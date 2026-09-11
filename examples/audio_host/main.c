/*
 * Standalone Pico-PIO-USB Audio Host test with attach-to-stream diagnostics.
 * Upstream TinyUSB still owns audio parsing, configuration and test-tone logic.
 */
#include <stdio.h>

#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pio_usb.h"
#include "tusb.h"
#include "app.h"
#include "diagnostics.h"

#ifndef AUDIO_HOST_TINYUSB_REV
#define AUDIO_HOST_TINYUSB_REV "unknown"
#endif

static void initialization_failed(const char *message) {
  audio_diagnostics_fault(message);
  while (true) {
    audio_diagnostics_task();
    tight_loop_contents();
  }
}

int main(void) {
  // PIO USB requires a system clock that is an integer multiple of 12 MHz.
  set_sys_clock_khz(120000, true);
  stdio_init_all();
  sleep_ms(10);
  audio_diagnostics_init();

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  printf("Pico-PIO-USB audio diagnostics; TinyUSB=" AUDIO_HOST_TINYUSB_REV "\r\n");
  printf("[diag] clock=%lu D+=GP%u D-=GP%u rhport=1 EP_SIZE=%u\r\n",
         (unsigned long)clock_get_hz(clk_sys), pio_cfg.pin_dp,
         pio_cfg.pinout == PIO_USB_PINOUT_DPDM ? pio_cfg.pin_dp + 1 : pio_cfg.pin_dp - 1,
         PIO_USB_EP_SIZE);
  if (!tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg)) {
    initialization_failed("tuh_configure failed");
  }
  if (!tuh_init(1)) {
    initialization_failed("tuh_init failed");
  }
  printf("[diag] host initialized; connect a USB audio device on GP14/15\r\n");

  while (true) {
    audio_diagnostics_task();
    tuh_task();
    audio_app_task();
    defer_queue_task();
    // Diagnostics replaces only the upstream heartbeat, not audio behavior.
  }
}
