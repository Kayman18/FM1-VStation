#ifndef VSTATION_PORTAMENTO_H
#define VSTATION_PORTAMENTO_H
#include <stdint.h>
typedef struct {unsigned linear;int32_t offset,target,rate;} Portamento;
void portamento_step(Portamento *p);
/* Configure 44.1 kHz rate from recovered time/interval tables. */
void portamento_configure(Portamento *p,unsigned linear,unsigned time);
unsigned portamento_time_ms(unsigned time);
#endif
