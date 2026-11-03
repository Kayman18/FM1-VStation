#pragma once
#include <stdint.h>
/* FM1 signed 24-bit output boundary. Preserve the old multiplication/truncation
   exactly inside range; saturate only overflow, silence NaN. This is an FM1
   safety guard, not a recovered V-Station DSP operation. */
static inline int32_t pcm_output(float sample,float gain){
 float x=sample*8388352.f*gain;
 if(x!=x)return 0;
 if(x>8388607.f)return 8388607;
 if(x< -8388608.f)return -8388608;
 return (int32_t)x;
}
