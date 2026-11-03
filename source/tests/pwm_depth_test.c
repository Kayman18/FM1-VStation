#include "synth.h"
#include "osc_modulation.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
extern int32_t v0_test_width_depth(unsigned,unsigned),v1_test_width_depth(unsigned,unsigned);
static int32_t original(unsigned v){float x=((int)v-64)/128.f;x*=2.f;x*=65536.f;x*=2.f;return (int32_t)x;}
int main(void){
 synth_init();synth_param(95,2);synth_note(48,100);synth_note(60,100);
 for(unsigned v=0;v<128;v++){
  synth_param(102,0);synth_param(23,v);
  for(unsigned i=0;i<3;i++){
   synth_param(39+i,v);synth_param(103+i,127-v);
   assert(v0_test_width_depth(i,1)==((int)v-64)*512);
   assert(v1_test_width_depth(i,0)==(int32_t)(v*32768/127));
  }
  synth_param(102,1);
  for(unsigned i=0;i<3;i++){
   assert(v0_test_width_depth(i,1)==original(v));
   assert(v1_test_width_depth(i,0)==original(127-v));
  }
 }
 synth_param(103,65);synth_param(104,63);synth_param(105,64);
 assert(v0_test_width_depth(0,0)==2048);
 assert(v0_test_width_depth(1,0)==-2048);
 assert(v0_test_width_depth(2,0)==0);
 synth_param(23,127);assert(v0_test_width_depth(2,0)==0);
 /* Equal/opposite LFO routes cancel modulo phase, including signed extremes. */
 for(int32_t l=-32768;l<=32768;l+=1024)
  assert(osc_width_mod(0,0,0,l,2048)+osc_width_mod(0,0,0,l,-2048)==0u);
 synth_panic();assert(v1_test_width_depth(0,0)==2048);
 synth_param(102,0);assert(v1_test_width_depth(2,0)==32768);
 puts("PWM depths: independent signed routes, legacy mode, both voices passed");
}
