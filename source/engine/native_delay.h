#pragma once
#include <stdint.h>
/* Original 65536-float ring at 44.1 kHz, integer stereo taps and filtered
   feedback. Storage is supplied by the caller; no allocation or resampling. */
#define NATIVE_DELAY_SAMPLES 65536u
typedef struct {
 float *buffer;
 uint32_t valid_samples;
 uint16_t write_index,feedback_index,left_index,right_index;
 uint16_t length,left_length,right_length;
 uint8_t ratio,sync;
 float level,wheel,wheel_depth,send,seconds,smoothed,feedback,low;
 float left_gain,right_gain,bpm,period,free_seconds,same,cross;
} NativeDelay;
void native_delay_init(NativeDelay *s,float buffer[NATIVE_DELAY_SAMPLES]);
void native_delay_reset(NativeDelay *s);
void native_delay_level(NativeDelay *s,float normalized);
void native_delay_time(NativeDelay *s,float normalized);
void native_delay_feedback(NativeDelay *s,float normalized);
void native_delay_sync(NativeDelay *s,unsigned native_index);
void native_delay_width(NativeDelay *s,float normalized);
void native_delay_ratio(NativeDelay *s,unsigned native_index);
void native_delay_wheel_depth(NativeDelay *s,float normalized_signed);
void native_delay_wheel(NativeDelay *s,float normalized);
void native_delay_tempo(NativeDelay *s,float bpm);
void native_delay_step(NativeDelay *s,float input,float *left,float *right);
void native_delay_process_add(NativeDelay *s,const float *input,float *left,float *right,unsigned n);
