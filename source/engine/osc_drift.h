#pragma once
#include <stdint.h>
float osc_drift_scale(unsigned oscillator,unsigned value);
int32_t osc_drift_increment(float base_pitch,float scale);
