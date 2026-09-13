/* Hardware-independent, testable diagnostic progress and LED encoding. */
#ifndef AUDIO_HOST_DIAGNOSTICS_STATE_H
#define AUDIO_HOST_DIAGNOSTICS_STATE_H

#include <stdbool.h>
#include <stdint.h>

enum {
  AUDIO_DIAG_BOOT = 1,
  AUDIO_DIAG_ATTACH,
  AUDIO_DIAG_DEVICE_DESC,
  AUDIO_DIAG_CONFIG_DESC,
  AUDIO_DIAG_AUDIO_PARSED,
  AUDIO_DIAG_AUDIO_MOUNTED,
  AUDIO_DIAG_PLAYBACK_STARTED,
  AUDIO_DIAG_PLAYBACK_COMPLETING
};

enum {
  AUDIO_DIAG_FAULT_NONE = 0,
  // Focused one-burst codes for the current hub playback-start investigation.
  AUDIO_DIAG_FAULT_PLAYBACK_SET_INTERFACE = 1,
  AUDIO_DIAG_FAULT_PLAYBACK_SET_RATE = 2,
  AUDIO_DIAG_FAULT_PLAYBACK_START_OTHER = 3,
  // Generic fallbacks retained for unexpected failures.
  AUDIO_DIAG_FAULT_SETUP_REJECTED = 4,
  AUDIO_DIAG_FAULT_EP_OPEN = 5,
  AUDIO_DIAG_FAULT_CAPTURE_START = 6,
  AUDIO_DIAG_FAULT_CAPTURE_XFER = 7,
  AUDIO_DIAG_FAULT_PLAYBACK_XFER = 8,
  AUDIO_DIAG_FAULT_STOP = 9,
  AUDIO_DIAG_FAULT_ISO_SUBMIT = 10,
  AUDIO_DIAG_FAULT_INIT = 11,
};

enum {
  AUDIO_DIAG_PLAYBACK_CTL_OTHER = 0,
  AUDIO_DIAG_PLAYBACK_CTL_SET_INTERFACE = 1,
  AUDIO_DIAG_PLAYBACK_CTL_SET_RATE = 2,
};

enum {
  AUDIO_DIAG_PULSE_MS = 100,
  AUDIO_DIAG_PULSE_PERIOD_MS = 250,
  AUDIO_DIAG_CYCLE_GAP_MS = 1500,
};

typedef struct {
  uint8_t stage;
  uint8_t playback_completions;
  uint8_t fault_code;
} audio_diag_state_t;

static inline void audio_diag_reset(audio_diag_state_t *s) {
  *s = (audio_diag_state_t){.stage = AUDIO_DIAG_BOOT};
}

static inline void audio_diag_advance(audio_diag_state_t *s, uint8_t stage) {
  if (stage > s->stage && stage <= AUDIO_DIAG_PLAYBACK_COMPLETING) {
    s->stage = stage;
  }
}

/* Latch the first/root fault. The demo may automatically retry a failed stream
 * and produce secondary failures that should not replace the initial cause. */
static inline void audio_diag_fault(audio_diag_state_t *s, uint8_t fault_code) {
  if (s->fault_code == AUDIO_DIAG_FAULT_NONE &&
      fault_code != AUDIO_DIAG_FAULT_NONE) {
    s->fault_code = fault_code;
  }
}

/* Classify the control request most likely responsible for a UAC1 playback
 * start failure. SET_INTERFACE activates the selected AS alternate setting.
 * UAC1 SET_CUR(SAMPLING_FREQ) targets the endpoint after activation. */
static inline uint8_t audio_diag_playback_start_detail(const uint8_t setup[8]) {
  if (setup[0] == 0x01u && setup[1] == 0x0bu) {
    return AUDIO_DIAG_PLAYBACK_CTL_SET_INTERFACE;
  }
  if (setup[0] == 0x22u && setup[1] == 0x01u &&
      setup[2] == 0x00u && setup[3] == 0x01u &&
      setup[6] == 0x03u && setup[7] == 0x00u) {
    return AUDIO_DIAG_PLAYBACK_CTL_SET_RATE;
  }
  return AUDIO_DIAG_PLAYBACK_CTL_OTHER;
}

static inline void audio_diag_playback_complete(audio_diag_state_t *s) {
  if (s->playback_completions < 2) ++s->playback_completions;
  if (s->playback_completions == 2) {
    audio_diag_advance(s, AUDIO_DIAG_PLAYBACK_COMPLETING);
  }
}

/* One burst only. Before any failure it shows the progress stage. Once a fault
 * is latched it shows only the fault code, followed by the long cycle gap. */
static inline uint8_t audio_diag_led_pulse_count(const audio_diag_state_t *s) {
  return s->fault_code != AUDIO_DIAG_FAULT_NONE ? s->fault_code : s->stage;
}

static inline uint32_t audio_diag_led_cycle_ms(const audio_diag_state_t *s) {
  return (uint32_t)audio_diag_led_pulse_count(s) * AUDIO_DIAG_PULSE_PERIOD_MS +
         AUDIO_DIAG_CYCLE_GAP_MS;
}

static inline bool audio_diag_led_on(const audio_diag_state_t *s, uint32_t elapsed_ms) {
  uint32_t const phase = elapsed_ms % audio_diag_led_cycle_ms(s);
  uint32_t const burst_span =
      (uint32_t)audio_diag_led_pulse_count(s) * AUDIO_DIAG_PULSE_PERIOD_MS;
  return phase < burst_span &&
         phase % AUDIO_DIAG_PULSE_PERIOD_MS < AUDIO_DIAG_PULSE_MS;
}

#endif
