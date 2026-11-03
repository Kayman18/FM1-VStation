#include "synth.h"
#include "osc_drift.h"
#include "osc_tuning.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int32_t v0_test_drift(unsigned),v1_test_drift(unsigned);
static float a[4096],b[4096];
static void setup(unsigned amount){
 synth_init();synth_param(95,2);synth_param(125,amount);synth_param(14,64);
 synth_note(60,100);synth_note(67,100);
}
int main(void){
 for(unsigned v=0;v<128;v++)for(unsigned i=0;i<3;i++){
  float scalar=(float)((double)(v/127.f)*.9921875);
  scalar*=i==1?39.f:i==2?-17.f:0.f;scalar*=.00048828125f;
  assert(osc_drift_scale(i,v)==scalar);
  for(unsigned n=0;n<=126;n++){
   float pitch=osc_base_pitch_float(n,0,0,17,-3,0.f,0.f,0.f);
   float x=pitch/49152.f;x*=scalar;x*=4.f;x*=16777216.f;
   assert(osc_drift_increment(pitch,scalar)==(int32_t)x);
  }
 }
 setup(0);synth_render(a,4096);
 setup(127);assert(!v0_test_drift(0) && !v1_test_drift(0));
 assert(v0_test_drift(1)>0 && v0_test_drift(2)<0);
 assert(v1_test_drift(1)>v0_test_drift(1));
 int32_t before=v0_test_drift(1);synth_param(55,65);
 assert(v0_test_drift(1)>before); /* Fine pitch also updates drift. */
 synth_param(125,0);synth_param(55,64);synth_render(b,4096);
 assert(!memcmp(a,b,sizeof(a))); /* Edits before render do not reset phase. */
 setup(127);synth_render(a,4096);
 setup(127);for(unsigned i=0;i<4096;i+=16)synth_render(b+i,16);
 assert(!memcmp(a,b,sizeof(a)));
 synth_param(125,0);assert(!v0_test_drift(1) && !v1_test_drift(2));
 synth_param(125,999);synth_panic();synth_note(60,100);
 assert(v0_test_drift(1)>0); /* Saved value survives reset and is clamped. */
 synth_render(a,4096);for(unsigned i=0;i<4096;i++)assert(isfinite(a[i]));
 assert(!synth_faults);
 puts("PASS: drift scales, tuning, both voices, cache invalidation, zero restoration and block invariance");
}
