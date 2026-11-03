#pragma once
#include <stdint.h>
/* Recovered native distortion object. Field order matches the six float
   words at the original +0x00..+0x14; no host pointers or allocations. */
typedef struct {
 float amount, wheel, wheel_depth, target, smoothed, compensation;
} Distortion;
void distortion_init(Distortion *s);
void distortion_amount(Distortion *s,float amount);
void distortion_compensation(Distortion *s,float normalized);
void distortion_depth(Distortion *s,float depth);
void distortion_wheel(Distortion *s,float normalized);
float distortion_step(Distortion *s,float input);
