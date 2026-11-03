#pragma once
#include <stdint.h>
/* Portable subset of the original pan object. Free-running phase only;
   original note-dependent initial-position dispatch is not implemented. */
typedef struct {
 float position,depth,hz,output,phase_value,phase_step,fade_step,fade;
 float bpm,period,free_rate_squared,smoothed;
 uint32_t phase,increment,sync;
} NativePan;
void native_pan_init(NativePan *s);
void native_pan_position(NativePan *s,float normalized_signed);
void native_pan_depth(NativePan *s,float normalized);
void native_pan_rate(NativePan *s,float normalized);
void native_pan_sync(NativePan *s,unsigned native_index);
void native_pan_tempo(NativePan *s,float bpm);
void native_pan_reset(NativePan *s);
void native_pan_step(NativePan *s,float input,float *left,float *right);
