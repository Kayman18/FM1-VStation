#include "osc_modulation.h"
#include <string.h>
static int32_t signed_bits(uint32_t bits){int32_t v;memcpy(&v,&bits,4);return v;}
/* RVA 0x37125..0x37179. Each product wraps before float conversion;
   retain two separate float products and their addition order. */
int32_t osc_pitch_mod(uint32_t env,uint32_t gain,int32_t env_depth,int32_t lfo,int32_t lfo_depth,int32_t bend){
 uint32_t level=((env>>16)*gain)>>15;
 float e=(float)signed_bits(level*(uint32_t)env_depth);e*=0.000030517578125f;
 float l=(float)signed_bits((uint32_t)lfo*(uint32_t)lfo_depth);l*=0.000030517578125f;
 float sum=e+l;sum+=(float)(bend/65536);return (int32_t)sum;
}
/* RVA 0x371d8..0x37210: unsigned wrap gives phase displacement. */
uint32_t osc_width_mod(uint32_t env,uint32_t gain,int32_t env_depth,int32_t lfo,int32_t lfo_depth){
 uint32_t level=((env>>16)*gain)>>15;
 return level*(uint32_t)env_depth+(uint32_t)lfo*(uint32_t)lfo_depth;
}
