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
  AUDIO_DIAG_FAULT_SETUP_REJECTED = 1,
  AUDIO_DIAG_FAULT_EP_OPEN = 2,
  AUDIO_DIAG_FAULT_CAPTURE_START = 3,
  AUDIO_DIAG_FAULT_PLAYBACK_START = 4,
  AUDIO_DIAG_FAULT_CAPTURE_XFER = 5,
  AUDIO_DIAG_FAULT_PLAYBACK_XFER = 6,
  AUDIO_DIAG_FAULT_STOP = 7,
  AUDIO_DIAG_FAULT_ISO_SUBMIT = 8,
  AUDIO_DIAG_FAULT_INIT = 9,
};

enum {
  AUDIO_DIAG_PULSE_MS = 100,
  AUDIO_DIAG_PULSE_PERIOD_MS = 250,
  AUDIO_DIAG_GROUP_GAP_MS = 1000,
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

static inline void audio_diag_playback_complete(audio_diag_state_t *s) {
  if (s->playback_completions < 2) ++s->playback_completions;
  if (s->playback_completions == 2) {
    audio_diag_advance(s, AUDIO_DIAG_PLAYBACK_COMPLETING);
  }
}

static inline uint32_t audio_diag_led_cycle_ms(const audio_diag_state_t *s) {
  uint32_t const stage_span = (uint32_t)s->stage * AUDIO_DIAG_PULSE_PERIOD_MS;
  if (s->fault_code == AUDIO_DIAG_FAULT_NONE) {
    return stage_span + AUDIO_DIAG_CYCLE_GAP_MS;
  }
  return stage_span + AUDIO_DIAG_GROUP_GAP_MS +
         (uint32_t)s->fault_code * AUDIO_DIAG_PULSE_PERIOD_MS +
         AUDIO_DIAG_CYCLE_GAP_MS;
}

/*
 * Normal: [stage-count burst] [long gap].
 * Fault:  [stage-count burst] [1 s gap] [fault-code burst] [long gap].
 * Pulses are 100 ms on every 250 ms.
 */
static inline bool audio_diag_led_on(const audio_diag_state_t *s, uint32_t elapsed_ms) {
  uint32_t phase = elapsed_ms % audio_diag_led_cycle_ms(s);
  uint32_t const stage_span = (uint32_t)s->stage * AUDIO_DIAG_PULSE_PERIOD_MS;

  if (phase < stage_span) {
    return phase % AUDIO_DIAG_PULSE_PERIOD_MS < AUDIO_DIAG_PULSE_MS;
  }

  if (s->fault_code == AUDIO_DIAG_FAULT_NONE) return false;

  phase -= stage_span;
  if (phase < AUDIO_DIAG_GROUP_GAP_MS) return false;
  phase -= AUDIO_DIAG_GROUP_GAP_MS;

  uint32_t const fault_span =
      (uint32_t)s->fault_code * AUDIO_DIAG_PULSE_PERIOD_MS;
  return phase < fault_span &&
         phase % AUDIO_DIAG_PULSE_PERIOD_MS < AUDIO_DIAG_PULSE_MS;
}

#endif
