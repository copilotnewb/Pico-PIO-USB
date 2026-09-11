#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "diagnostics_state.h"

int main(void) {
  audio_diag_state_t s;
  audio_diag_reset(&s);
  assert(s.stage == AUDIO_DIAG_BOOT && !s.fault);
  audio_diag_advance(&s, AUDIO_DIAG_DEVICE_DESC);
  assert(s.stage == AUDIO_DIAG_DEVICE_DESC);
  audio_diag_advance(&s, AUDIO_DIAG_ATTACH);
  assert(s.stage == AUDIO_DIAG_DEVICE_DESC); // delayed observations do not downgrade evidence
  audio_diag_advance(&s, AUDIO_DIAG_AUDIO_MOUNTED);
  assert(s.stage == AUDIO_DIAG_AUDIO_MOUNTED);
  audio_diag_playback_complete(&s);
  assert(s.stage != AUDIO_DIAG_PLAYBACK_COMPLETING); // one completion is not a running stream
  audio_diag_playback_complete(&s);
  assert(s.stage == AUDIO_DIAG_PLAYBACK_COMPLETING);
  for (unsigned i = 0; i < 1000; ++i) audio_diag_playback_complete(&s);
  assert(s.playback_completions == 2); // saturate, never overflow back to zero
  s.fault = true;
  audio_diag_reset(&s);
  assert(s.stage == AUDIO_DIAG_BOOT && !s.fault && s.playback_completions == 0);

  for (unsigned pulses = 1; pulses <= 9; ++pulses) {
    s.stage = (uint8_t)(pulses <= 8 ? pulses : 1);
    s.fault = pulses == 9;
    const uint32_t cycle = pulses * 250u + 1250u;
    unsigned rising_edges = 0;
    bool previous = false;
    for (uint32_t ms = 0; ms < cycle; ++ms) {
      const bool on = audio_diag_led_on(&s, ms);
      if (on && !previous) ++rising_edges;
      previous = on;
      if (ms >= pulses * 250u) assert(!on); // visible gap between groups
      assert(on == audio_diag_led_on(&s, ms + cycle));
    }
    assert(rising_edges == pulses);
  }
  puts("audio diagnostics state tests: PASS");
  return 0;
}
