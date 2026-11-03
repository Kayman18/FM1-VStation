#pragma once
#include <stdint.h>
#define NATIVE_REVERB_LINES 12u
/* Largest native delay length reached by each line across all six room types.
   Independent compact histories are exact because native write heads advance in
   lockstep and no other head reaches a line's live history before its read. */
extern const uint16_t native_reverb_capacities[NATIVE_REVERB_LINES];
extern const uint16_t native_reverb_lengths[6][NATIVE_REVERB_LINES];
typedef struct {
 float *history[NATIVE_REVERB_LINES];
 uint16_t write[NATIVE_REVERB_LINES];
 uint16_t read[NATIVE_REVERB_LINES];
 uint16_t length[NATIVE_REVERB_LINES];
 uint16_t valid[NATIVE_REVERB_LINES];
 uint16_t warm_samples;
 uint8_t type;
 float level,wheel,wheel_depth,send,decay;
 float coefficient[10];
 float feedback_state[2];
 float low_a[2],low_b[2];
} NativeReverb;
void native_reverb_init(NativeReverb *s,float *const history[NATIVE_REVERB_LINES]);
void native_reverb_decay_index(NativeReverb *s,unsigned value);
void native_reverb_reset(NativeReverb *s);
void native_reverb_level(NativeReverb *s,float x);
void native_reverb_type(NativeReverb *s,unsigned type);
void native_reverb_decay(NativeReverb *s,float x);
void native_reverb_wheel_depth(NativeReverb *s,float x);
void native_reverb_wheel(NativeReverb *s,float x);
void native_reverb_step(NativeReverb *s,float input,float *left,float *right);
void native_reverb_process(NativeReverb *s,const float *input,float *left,float *right,unsigned n);
