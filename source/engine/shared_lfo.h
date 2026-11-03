#pragma once
#include <stdint.h>
extern uint32_t shared_lfo_state[2][14];
extern unsigned shared_lfo_mask;
void shared_lfo_init(void);
void shared_lfo_panic(void);
void shared_lfo_config(unsigned i,const uint32_t *source);
void shared_lfo_enable(unsigned i,unsigned enabled,const uint32_t *source);
/* Called once by the renderer, never by an individual voice. */
unsigned shared_lfo_slice(unsigned requested);
void shared_lfo_advance(unsigned samples);
unsigned shared_lfo_boundary(void);
