#pragma once
#include <stdint.h>
/* Native object layout retained for isolated instruction comparisons.
   +0x58/+0x5c hold no foreign vtable in the portable implementation. */
typedef struct {
 uint32_t reserved;
 float feed, feedback, depth, direct_gain, low_gain, modulation_extent;
 int32_t frequency, modulation_scale;
 float low, free_rate_squared;
 uint32_t reserved2;
 uint32_t lfo[14];
 float bpm, period;
 uint8_t sync, initial_position;
 uint16_t reserved3;
} NativeEq;
extern const float native_eq_table_44100[128];
extern const float native_eq_periods[35];
void native_eq_init(NativeEq *s);
void native_eq_depth(NativeEq *s,float normalized_signed);
void native_eq_frequency(NativeEq *s,unsigned index);
void native_eq_modulation(NativeEq *s,float normalized);
void native_eq_rate(NativeEq *s,float normalized);
void native_eq_sync(NativeEq *s,unsigned native_index);
void native_eq_tempo(NativeEq *s,float bpm);
void native_eq_reset(NativeEq *s);
float native_eq_step(NativeEq *s,float input);
void native_eq_process(NativeEq *s,float *samples,unsigned n);
