#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "diagnostics_state.h"

static unsigned count_edges(const audio_diag_state_t *s, uint32_t start, uint32_t end) {
  unsigned edges = 0;
  bool previous = false;
  for (uint32_t ms = start; ms < end; ++ms) {
    bool const on = audio_diag_led_on(s, ms);
    if (on && !previous) ++edges;
    previous = on;
  }
  return edges;
}

int main(void) {
  audio_diag_state_t s;
  audio_diag_reset(&s);
  assert(s.stage == AUDIO_DIAG_BOOT && s.fault_code == AUDIO_DIAG_FAULT_NONE);
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

  audio_diag_fault(&s, AUDIO_DIAG_FAULT_EP_OPEN);
  assert(s.fault_code == AUDIO_DIAG_FAULT_EP_OPEN);
  audio_diag_reset(&s);
  assert(s.stage == AUDIO_DIAG_BOOT && s.fault_code == AUDIO_DIAG_FAULT_NONE &&
         s.playback_completions == 0);

  // Normal state: one burst containing the stage count.
  s.stage = 4;
  const uint32_t normal_cycle = audio_diag_led_cycle_ms(&s);
  assert(count_edges(&s, 0, normal_cycle) == 4);
  assert(audio_diag_led_on(&s, 0) == audio_diag_led_on(&s, normal_cycle));

  // Fault state: first burst is the last successful stage, second burst is fault code.
  s.stage = 4;
  audio_diag_fault(&s, AUDIO_DIAG_FAULT_SETUP_REJECTED);
  const uint32_t first_end = 4u * 250u;
  const uint32_t second_start = first_end + AUDIO_DIAG_GROUP_GAP_MS;
  const uint32_t second_end = second_start + AUDIO_DIAG_FAULT_SETUP_REJECTED * 250u;
  const uint32_t fault_cycle = audio_diag_led_cycle_ms(&s);
  assert(count_edges(&s, 0, first_end) == 4);
  assert(count_edges(&s, first_end, second_start) == 0);
  assert(count_edges(&s, second_start, second_end) == AUDIO_DIAG_FAULT_SETUP_REJECTED);
  assert(count_edges(&s, second_end, fault_cycle) == 0);
  assert(audio_diag_led_on(&s, 0) == audio_diag_led_on(&s, fault_cycle));

  puts("audio diagnostics state tests: PASS");
  return 0;
}
