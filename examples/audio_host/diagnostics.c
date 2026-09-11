/* Observers only: no USB requests or changes to the PIO transfer engine. */
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"
#include "tusb.h"
#include "host/hcd.h"
#include "bsp/board_api.h"
#include "diagnostics.h"
#include "diagnostics_state.h"

/* CMake renames only the upstream application's callback definitions. */
void audio_example_mount_cb(uint8_t idx);
void audio_example_umount_cb(uint8_t idx);
void audio_example_capture_cb(uint8_t idx, uint8_t stream_idx, uint16_t bytes);
void audio_example_playback_cb(uint8_t idx, uint8_t stream_idx, uint16_t bytes);
void audio_example_event_cb(uint8_t idx, uint8_t stream_idx,
                            tuh_audio_event_t event, tusb_xfer_result_t result);

static audio_diag_state_t state;
static uint32_t led_epoch_ms, report_ms, previous_frames;
static uint32_t capture_count, playback_count, failure_count;
static uint32_t reset_start_count, reset_end_count, iso_out_submitted;
static uint16_t iso_out_mask[CFG_TUH_DEVICE_MAX + 1];
static uint32_t observed_generation;
static uint8_t printed_stage;
static volatile uint32_t link_generation, attach_count, remove_count, xfer_event_count;
static volatile bool link_attached;

/* The HCD hook can run in the 1 ms IRQ. Never print or drive LEDs here. */
void tuh_event_hook_cb(uint8_t rhport, uint32_t eventid, bool in_isr) {
  (void)in_isr;
  /* TinyUSB currently fills rhport=0 for transfer events, even on PIO port 1.
     This example has only one host controller. Filter only link events. */
  if (eventid == HCD_EVENT_XFER_COMPLETE) {
    ++xfer_event_count;
    return;
  }
  if (rhport != 1) return;
  if (eventid == HCD_EVENT_DEVICE_ATTACH) {
    ++attach_count;
    link_attached = true;
    ++link_generation;
  } else if (eventid == HCD_EVENT_DEVICE_REMOVE) {
    ++remove_count;
    link_attached = false;
    ++link_generation;
  }
}

/* All remaining state is main-loop-owned. Reset stale milestones on replug. */
static void sync_link(void) {
  const uint32_t irq = save_and_disable_interrupts();
  const uint32_t generation = link_generation;
  const bool attached = link_attached;
  restore_interrupts(irq);
  if (generation == observed_generation) return;
  observed_generation = generation;
  audio_diag_reset(&state);
  if (attached) audio_diag_advance(&state, AUDIO_DIAG_ATTACH);
  memset(iso_out_mask, 0, sizeof(iso_out_mask));
  capture_count = playback_count = failure_count = iso_out_submitted = 0;
  reset_start_count = reset_end_count = 0;
  led_epoch_ms = tusb_time_millis_api();
  printf("[diag] HCD %s on rhport 1; generation=%lu\r\n",
         attached ? "attach" : "remove", (unsigned long)generation);
}

void audio_diagnostics_init(void) {
  audio_diag_reset(&state);
  report_ms = led_epoch_ms = tusb_time_millis_api();
  board_led_write(false);
}

void audio_diagnostics_fault(const char *message) {
  sync_link();
  state.fault = true;
  ++failure_count;
  printf("[diag] ERROR: %s (last stage=%u)\r\n", message, state.stage);
}

void tuh_enum_descriptor_device_cb(uint8_t daddr, const tusb_desc_device_t *desc) {
  sync_link();
  audio_diag_advance(&state, AUDIO_DIAG_DEVICE_DESC);
  printf("[diag] device addr=%u VID:PID=%04x:%04x EP0=%u\r\n", daddr,
         tu_le16toh(desc->idVendor), tu_le16toh(desc->idProduct), desc->bMaxPacketSize0);
}

bool tuh_enum_descriptor_configuration_cb(uint8_t daddr, uint8_t index,
                                           const tusb_desc_configuration_t *desc) {
  sync_link();
  audio_diag_advance(&state, AUDIO_DIAG_CONFIG_DESC);
  printf("[diag] config addr=%u index=%u bytes=%u interfaces=%u\r\n", daddr, index,
         tu_le16toh(desc->wTotalLength), desc->bNumInterfaces);
  return true; /* Same policy as TinyUSB's default callback. */
}

void tuh_audio_descriptor_cb(uint8_t idx, const tuh_audio_descriptor_cb_t *desc) {
  (void)desc;
  sync_link();
  audio_diag_advance(&state, AUDIO_DIAG_AUDIO_PARSED);
  printf("[diag] audioh_open accepted idx=%u streams=%u\r\n", idx, tuh_audio_stream_count(idx));
}

void tuh_mount_cb(uint8_t daddr) {
  printf("[diag] USB enumeration complete addr=%u (not proof of audio mount)\r\n", daddr);
}

void tuh_umount_cb(uint8_t daddr) {
  sync_link();
  printf("[diag] USB unmounted addr=%u\r\n", daddr);
}

void tuh_audio_mount_cb(uint8_t idx) {
  sync_link();
  audio_diag_advance(&state, AUDIO_DIAG_AUDIO_MOUNTED);
  printf("[diag] audio mount idx=%u\r\n", idx);
  audio_example_mount_cb(idx);
}

void tuh_audio_umount_cb(uint8_t idx) {
  sync_link();
  audio_example_umount_cb(idx);
}

void tuh_audio_capture_cb(uint8_t idx, uint8_t stream_idx, uint16_t bytes) {
  ++capture_count;
  audio_example_capture_cb(idx, stream_idx, bytes);
}

void tuh_audio_playback_cb(uint8_t idx, uint8_t stream_idx, uint16_t bytes) {
  ++playback_count;
  audio_diag_playback_complete(&state);
  audio_example_playback_cb(idx, stream_idx, bytes);
}

void tuh_audio_event_cb(uint8_t idx, uint8_t stream_idx,
                        tuh_audio_event_t event, tusb_xfer_result_t result) {
  sync_link();
  if (result != XFER_RESULT_SUCCESS) {
    state.fault = true;
    ++failure_count;
  } else if (event == TUH_AUDIO_EVENT_START_COMPLETE &&
             tuh_audio_stream_direction(idx, stream_idx) == TUH_AUDIO_STREAM_PLAYBACK) {
    state.fault = false;
    audio_diag_advance(&state, AUDIO_DIAG_PLAYBACK_STARTED);
  }
  audio_example_event_cb(idx, stream_idx, event, result);
}

/* Linker wrappers forward each call exactly once, preserving the result. */
void __real_hcd_port_reset(uint8_t rhport);
void __wrap_hcd_port_reset(uint8_t rhport) {
  sync_link();
  ++reset_start_count;
  printf("[diag] bus reset start rhport=%u\r\n", rhport);
  __real_hcd_port_reset(rhport);
}

void __real_hcd_port_reset_end(uint8_t rhport);
void __wrap_hcd_port_reset_end(uint8_t rhport) {
  __real_hcd_port_reset_end(rhport);
  ++reset_end_count;
  printf("[diag] bus reset end rhport=%u\r\n", rhport);
}

bool __real_hcd_setup_send(uint8_t rhport, uint8_t daddr, const uint8_t setup[8]);
bool __wrap_hcd_setup_send(uint8_t rhport, uint8_t daddr, const uint8_t setup[8]) {
  printf("[diag] SETUP addr=%u type=%02x req=%02x value=%04x index=%04x len=%u\r\n",
         daddr, setup[0], setup[1], setup[2] | ((unsigned)setup[3] << 8),
         setup[4] | ((unsigned)setup[5] << 8), setup[6] | ((unsigned)setup[7] << 8));
  const bool accepted = __real_hcd_setup_send(rhport, daddr, setup);
  if (!accepted) audio_diagnostics_fault("HCD rejected SETUP submission");
  return accepted;
}

bool __real_hcd_edpt_open(uint8_t rhport, uint8_t daddr, const tusb_desc_endpoint_t *desc);
bool __wrap_hcd_edpt_open(uint8_t rhport, uint8_t daddr, const tusb_desc_endpoint_t *desc) {
  const bool accepted = __real_hcd_edpt_open(rhport, daddr, desc);
  printf("[diag] EP open addr=%u ep=%02x attr=%02x max=%u interval=%u %s\r\n",
         daddr, desc->bEndpointAddress, ((const uint8_t *)desc)[3], tu_edpt_packet_size(desc),
         desc->bInterval, accepted ? "OK" : "REJECTED");
  if (!accepted) audio_diagnostics_fault("HCD rejected endpoint");
  if (accepted && daddr <= CFG_TUH_DEVICE_MAX &&
      desc->bmAttributes.xfer == TUSB_XFER_ISOCHRONOUS &&
      tu_edpt_dir(desc->bEndpointAddress) == TUSB_DIR_OUT) {
    iso_out_mask[daddr] |= (uint16_t)(1u << tu_edpt_number(desc->bEndpointAddress));
  }
  return accepted;
}

bool __real_hcd_edpt_xfer(uint8_t rhport, uint8_t daddr, uint8_t ep, uint8_t *buffer, uint16_t len);
bool __wrap_hcd_edpt_xfer(uint8_t rhport, uint8_t daddr, uint8_t ep, uint8_t *buffer, uint16_t len) {
  const bool accepted = __real_hcd_edpt_xfer(rhport, daddr, ep, buffer, len);
  if (daddr <= CFG_TUH_DEVICE_MAX && tu_edpt_dir(ep) == TUSB_DIR_OUT &&
      (iso_out_mask[daddr] & (1u << tu_edpt_number(ep)))) {
    if (accepted) ++iso_out_submitted;
    else { state.fault = true; ++failure_count; }
  }
  return accepted;
}

void audio_diagnostics_task(void) {
  sync_link();
  const uint32_t now = tusb_time_millis_api();
  if (printed_stage != state.stage) {
    printed_stage = state.stage;
    led_epoch_ms = now;
    printf("[diag] last successful stage=%u\r\n", state.stage);
  }
  board_led_write(audio_diag_led_on(&state, now - led_epoch_ms));
  if (now - report_ms < 1000u) return;
  report_ms = now;

  /* On this example all PIO work runs on core 0. Keep this snapshot brief.
     Do not use bare gpio_get(): PIO USB intentionally inverts input signals. */
  root_port_t *root = PIO_USB_ROOT_PORT(0);
  const uint32_t irq = save_and_disable_interrupts();
  const unsigned line = root->initialized ? (unsigned)pio_usb_bus_get_line_state(root) : 4u;
  const bool initialized = root->initialized, connected = root->connected, suspended = root->suspended;
  const uint32_t frames = pio_usb_host_get_frame_number();
  const uint32_t attaches = attach_count, removes = remove_count, events = xfer_event_count;
  restore_interrupts(irq);
  static const char *const line_names[] = {"SE0", "FS", "LS", "SE1", "uninit"};
  printf("[diag] frame_delta=%lu line=%s init=%u conn=%u susp=%u attach=%lu remove=%lu xfer=%lu\r\n",
         (unsigned long)(frames - previous_frames), line_names[line], initialized, connected, suspended,
         (unsigned long)attaches, (unsigned long)removes, (unsigned long)events);
  printf("[diag] reset=%lu/%lu ISO_OUT=%lu MIC_CB=%lu SPK_CB=%lu FAIL=%lu stage=%u\r\n",
         (unsigned long)reset_start_count, (unsigned long)reset_end_count, (unsigned long)iso_out_submitted,
         (unsigned long)capture_count, (unsigned long)playback_count, (unsigned long)failure_count, state.stage);
  previous_frames = frames;
}
