#include "filter_modulation.h"
#include <string.h>
/* Original 0x469f0 / 0x46ba0 depth curve, table RVA 0x115b40.
   Called on parameter changes, not in the audio loop. */
static int32_t depth_curve(float x){
 static const float table[65]={0x0.0p+0f,0x1.0000000000000p-6f,0x1.4000000000000p-5f,0x1.0000000000000p-4f,0x1.8000000000000p-4f,0x1.2000000000000p-3f,0x1.8000000000000p-3f,0x1.0000000000000p-2f,0x1.6000000000000p-2f,0x1.c000000000000p-2f,0x1.2000000000000p-1f,0x1.8000000000000p-1f,0x1.0000000000000p+0f,0x1.4000000000000p+0f,0x1.8000000000000p+0f,0x1.c000000000000p+0f,0x1.0000000000000p+1f,0x1.4000000000000p+1f,0x1.8000000000000p+1f,0x1.c000000000000p+1f,0x1.0000000000000p+2f,0x1.2000000000000p+2f,0x1.4000000000000p+2f,0x1.6000000000000p+2f,0x1.8000000000000p+2f,0x1.c000000000000p+2f,0x1.0000000000000p+3f,0x1.2000000000000p+3f,0x1.4000000000000p+3f,0x1.6000000000000p+3f,0x1.8000000000000p+3f,0x1.c000000000000p+3f,0x1.0000000000000p+4f,0x1.3000000000000p+4f,0x1.5000000000000p+4f,0x1.7000000000000p+4f,0x1.8000000000000p+4f,0x1.a000000000000p+4f,0x1.c000000000000p+4f,0x1.f000000000000p+4f,0x1.0800000000000p+5f,0x1.1800000000000p+5f,0x1.2000000000000p+5f,0x1.3000000000000p+5f,0x1.4000000000000p+5f,0x1.5800000000000p+5f,0x1.6800000000000p+5f,0x1.7800000000000p+5f,0x1.8000000000000p+5f,0x1.a000000000000p+5f,0x1.b800000000000p+5f,0x1.e000000000000p+5f,0x1.0000000000000p+6f,0x1.0c00000000000p+6f,0x1.2000000000000p+6f,0x1.3000000000000p+6f,0x1.3c00000000000p+6f,0x1.5000000000000p+6f,0x1.6c00000000000p+6f,0x1.8000000000000p+6f,0x1.9c00000000000p+6f,0x1.b000000000000p+6f,0x1.cc00000000000p+6f,0x1.e000000000000p+6f,0x1.fc00000000000p+6f};
 if(x< -1.f)x=-1.f;if(x>1.f)x=1.f;
 float sign=1.f;if(x<0){sign=-1.f;x=-x;}
 x*=64.f;unsigned i=(unsigned)x;float f=x-(float)i;
 float next=table[i<64?i+1:64];
 float depth=table[i]*(1.f-f)+next*f;
 depth*=sign;depth*=128.f;return (int32_t)depth;
}
/* Original 0x37444..0x3745c: wrap multiplication, divide toward zero. */
int32_t filter_lfo_mod(int32_t value,int32_t depth){
 uint32_t bits=(uint32_t)value*(uint32_t)depth;int32_t product;
 memcpy(&product,&bits,4);return product/32768;
}

int32_t filter_mod_depth(float value){return depth_curve(value*2.f);}
/* Original LFO-depth sum: base is doubled before controller offsets enter.
   Preserve float addition order and clamp only after the complete sum. */
int32_t filter_mod_depth_offsets(float base,float wheel,float breath,float pressure){
 float x=base*2.f;x+=wheel;x+=pressure;x+=breath;
 return depth_curve(x);
}
