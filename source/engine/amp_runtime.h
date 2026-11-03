#ifndef AMP_RUNTIME_H
#define AMP_RUNTIME_H
#include <stdint.h>
typedef struct {
 uint32_t mode,tick;
 float value,output,smoothed,gain,attack,decay,release,sustain;
} AmpRuntime;
void amp_gate(AmpRuntime*,unsigned retrigger);
void amp_release(AmpRuntime*);
float amp_audio(AmpRuntime*);
void amp_control(AmpRuntime*);
#endif
