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
  assert(s.stage == AUDIO_DIAG_DEVICE_DESC);
  audio_diag_advance(&s, AUDIO_DIAG_AUDIO_MOUNTED);
  assert(s.stage == AUDIO_DIAG_AUDIO_MOUNTED);

  const uint8_t set_interface[8] = {0x01, 0x0b, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00};
  const uint8_t set_rate[8]      = {0x22, 0x01, 0x00, 0x01, 0x02, 0x00, 0x03, 0x00};
  const uint8_t other[8]         = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
  assert(audio_diag_playback_start_detail(set_interface) == AUDIO_DIAG_PLAYBACK_CTL_SET_INTERFACE);
  assert(audio_diag_playback_start_detail(set_rate) == AUDIO_DIAG_PLAYBACK_CTL_SET_RATE);
  assert(audio_diag_playback_start_detail(other) == AUDIO_DIAG_PLAYBACK_CTL_OTHER);

  // Normal state: one burst containing the stage count.
  s.stage = 6;
  uint32_t cycle = audio_diag_led_cycle_ms(&s);
  assert(count_edges(&s, 0, cycle) == 6);

  // Fault state: still exactly one burst, containing only the focused fault code.
  s.stage = 6;
  audio_diag_fault(&s, 2);
  cycle = audio_diag_led_cycle_ms(&s);
  assert(count_edges(&s, 0, cycle) == 2);

  // Preserve the first/root fault if the demo auto-retries later.
  audio_diag_fault(&s, 3);
  assert(s.fault_code == 2);

  puts("audio diagnostics state tests: PASS");
  return 0;
}
