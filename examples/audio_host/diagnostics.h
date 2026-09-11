#ifndef AUDIO_HOST_DIAGNOSTICS_H
#define AUDIO_HOST_DIAGNOSTICS_H

void audio_diagnostics_init(void);
void audio_diagnostics_task(void);
void audio_diagnostics_fault(const char *message);

#endif
