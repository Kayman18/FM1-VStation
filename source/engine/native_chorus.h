#pragma once
#include <stdint.h>
/* Portable native chorus/phaser. Two original 4096-sample delay lines; no
   allocation, resampling, reduced buffers or substitute effects algorithm.
   Phrase-start phase dispatch is handled by native_fx_phase.c. */
typedef struct {
 uint32_t write_index, type;
 float delay_scale, level, depth, feedback, centre, centre_min, centre_max;
 float wheel, wheel_depth, send, free_rate_squared;
 float hz, output, phase_value, phase_step, fade_step, fade;
 uint32_t phase, increment;
 float allpass[2][4], previous[2], bpm, period;
 uint32_t sync;
 float buffer[2][4096];
} NativeChorus;
void native_chorus_init(NativeChorus *s);
void native_chorus_reset(NativeChorus *s);
void native_chorus_level(NativeChorus *s,float normalized);
void native_chorus_type(NativeChorus *s,unsigned type);
void native_chorus_rate(NativeChorus *s,float normalized);
void native_chorus_sync(NativeChorus *s,unsigned native_index);
void native_chorus_depth(NativeChorus *s,float normalized);
void native_chorus_centre(NativeChorus *s,float normalized_signed);
void native_chorus_feedback(NativeChorus *s,float normalized_signed);
void native_chorus_wheel_depth(NativeChorus *s,float normalized_signed);
void native_chorus_wheel(NativeChorus *s,float normalized);
void native_chorus_tempo(NativeChorus *s,float bpm);
/* Takes the mono post-EQ/pre-pan signal; returns only the two wet channels. */
void native_chorus_step(NativeChorus *s,float input,float *left,float *right);
void native_chorus_process_add(NativeChorus *s,const float *input,float *left,float *right,unsigned n);
/* TEST37 performance mode: 22.05 kHz wet path, duplicated back to 44.1 kHz. */
void native_chorus_process_add_half(NativeChorus *s,const float *input,float *left,float *right,unsigned n);
