#ifndef AUDIO_HOST_DIAGNOSTICS_H
#define AUDIO_HOST_DIAGNOSTICS_H

#include <stdint.h>

void audio_diagnostics_init(void);
void audio_diagnostics_task(void);
void audio_diagnostics_fault(uint8_t fault_code, const char *message);

#endif
