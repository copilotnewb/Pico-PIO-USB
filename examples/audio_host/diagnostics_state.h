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

typedef struct {
  uint8_t stage;
  uint8_t playback_completions;
  bool fault;
} audio_diag_state_t;

static inline void audio_diag_reset(audio_diag_state_t *s) {
  *s = (audio_diag_state_t){.stage = AUDIO_DIAG_BOOT};
}

static inline void audio_diag_advance(audio_diag_state_t *s, uint8_t stage) {
  if (stage > s->stage && stage <= AUDIO_DIAG_PLAYBACK_COMPLETING) {
    s->stage = stage;
  }
}

static inline void audio_diag_playback_complete(audio_diag_state_t *s) {
  if (s->playback_completions < 2) ++s->playback_completions;
  if (s->playback_completions == 2) {
    audio_diag_advance(s, AUDIO_DIAG_PLAYBACK_COMPLETING);
  }
}

/* 100 ms pulses, 250 ms spacing, then a 1250 ms gap. Nine pulses = fault. */
static inline bool audio_diag_led_on(const audio_diag_state_t *s, uint32_t elapsed_ms) {
  const uint32_t pulses = s->fault ? 9u : s->stage;
  const uint32_t phase = elapsed_ms % (pulses * 250u + 1250u);
  return phase < pulses * 250u && phase % 250u < 100u;
}

#endif
