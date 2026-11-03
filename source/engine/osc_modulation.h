#ifndef OSC_MODULATION_H
#define OSC_MODULATION_H
#include <stdint.h>
int32_t osc_pitch_mod(uint32_t env,uint32_t gain,int32_t env_depth,int32_t lfo,int32_t lfo_depth,int32_t bend);
uint32_t osc_width_mod(uint32_t env,uint32_t gain,int32_t env_depth,int32_t lfo,int32_t lfo_depth);
#endif
