#include "amp_level.h"
/* Original 0x46d00: single-unison target=10^((dB-6)/20).
   Table avoids pow and software doubles on the target. */
static const float curve[25]={0x1.01d3f40000000p-3f,0x1.2149a40000000p-3f,0x1.44960c0000000p-3f,0x1.6c310e0000000p-3f,0x1.98a1360000000p-3f,0x1.ca7d780000000p-3f,0x1.0137980000000p-2f,0x1.209a380000000p-2f,0x1.43d1360000000p-2f,0x1.6b54340000000p-2f,0x1.97a9680000000p-2f,0x1.c9676c0000000p-2f,0x1.009b9c0000000p-1f,0x1.1feb340000000p-1f,0x1.430cd80000000p-1f,0x1.6a77de0000000p-1f,0x1.96b2300000000p-1f,0x1.c8520a0000000p-1f,0x1.0000000000000p+0f,0x1.1f3c9a0000000p+0f,0x1.4248f00000000p+0f,0x1.699c100000000p+0f,0x1.95bb900000000p+0f,0x1.c73d520000000p+0f,0x1.fec9840000000p+0f};
static const float low_curve[7]={0x1.ccab860000000p-5f,0x1.0270aa0000000p-4f,0x1.21f9800000000p-4f,0x1.455b5a0000000p-4f,0x1.6d0e720000000p-4f,0x1.99999a0000000p-4f,0x1.cb942a0000000p-4f};
static unsigned level=64,original,unison_count=1;
static float target=1.f,current=1.f;
float amp_level_curve(unsigned value){if(value<52)value=52;if(value>76)value=76;return curve[value-52];}
static float corrected_curve(void){unsigned v=level-(unison_count-1);return v<52?low_curve[v-45]:curve[v-52];}
static void update_target(void){target=original?corrected_curve():1.f;}
void amp_level_init(void){level=64;original=0;unison_count=1;target=current=1.f;}
void amp_level_param(unsigned id,unsigned value){
 if(id==120){if(value<52)value=52;if(value>76)value=76;level=value;}
 else if(id==121)original=!!value;else return;
 update_target();
}
void amp_level_unison(unsigned count){if(count<1)count=1;if(count>8)count=8;unison_count=count;update_target();
}
void amp_level_reset(void){current=target;}
void amp_level_apply(float *out,unsigned n){
 if(target==1.f && current==1.f)return;
 for(unsigned i=0;i<n;i++){
  /* Original smoothing coefficients at0x3784d..0x37883. */
  current=target*.01f+current*.99f;
  if(!original && current>.99999f && current<1.00001f)current=1.f;
  out[i]*=current;
 }
}
