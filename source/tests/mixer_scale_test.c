#include "synth.h"
#include <assert.h>
#include <stdio.h>
extern float v0_test_mix(unsigned),v1_test_mix(unsigned);
int main(void){
 unsigned ids[]={3,4,5,43,12};synth_init();synth_param(95,2);
 for(unsigned v=0;v<128;v++){
  synth_param(107,0);
  for(unsigned i=0;i<5;i++){
   synth_param(ids[i],v);
   float legacy=(i==3 && !v)?-.0001f:v/127.f;
   assert(v0_test_mix(i)==legacy && v1_test_mix(i)==legacy);
  }
  assert(v0_test_mix(5)==.25f);
  synth_param(107,1);
  for(unsigned i=0;i<5;i++)assert(v0_test_mix(i)==(v/127.f)*.25f && v1_test_mix(i)==(v/127.f)*.25f);
  assert(v1_test_mix(5)==1.f);
 }
 synth_param(43,0);assert(v0_test_mix(3)==0.f);
 synth_panic();assert(v1_test_mix(3)==0.f && v1_test_mix(5)==1.f);
 synth_param(107,0);assert(v1_test_mix(3)==-.0001f);
 puts("mixer scale: all levels, both voices, mode changes and zero-noise policy passed");
}
