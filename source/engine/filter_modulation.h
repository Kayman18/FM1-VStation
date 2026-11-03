#ifndef FILTER_MODULATION_H
#define FILTER_MODULATION_H
#include <stdint.h>
/* Original signed normalized depth (-0.5..+0.5), saturated internally. */
int32_t filter_mod_depth(float value);
int32_t filter_lfo_mod(int32_t value,int32_t depth);
int32_t filter_mod_depth_offsets(float base,float wheel,float breath,float pressure);
#endif
