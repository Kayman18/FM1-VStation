#include "synth.h"
#include "filter_modulation.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
extern int32_t v0_test_pitch_env(unsigned),v1_test_pitch_env(unsigned);
/* Golden Q7 depths from original table RVA 0x115b40 at packed byte positions. */
static const int32_t golden[128]={-16256,-15360,-14720,-13824,-13184,-12288,-11648,-10752,-10112,-9728,-9216,-8576,-8192,-7680,-7040,-6656,-6144,-6016,-5760,-5504,-5120,-4864,-4608,-4480,-4224,-3968,-3584,-3328,-3072,-2944,-2688,-2432,-2048,-1792,-1536,-1408,-1280,-1152,-1024,-896,-768,-704,-640,-576,-512,-448,-384,-320,-256,-224,-192,-160,-128,-96,-72,-56,-44,-32,-24,-18,-12,-8,-5,-2,0,2,5,8,12,18,24,32,44,56,72,96,128,160,192,224,256,320,384,448,512,576,640,704,768,896,1024,1152,1280,1408,1536,1792,2048,2432,2688,2944,3072,3328,3584,3968,4224,4480,4608,4864,5120,5504,5760,6016,6144,6656,7040,7680,8192,8576,9216,9728,10112,10752,11648,12288,13184,13824,14720,15360};
int main(void){
 synth_init();synth_param(95,2);
 /* Mode changes must update both voices, including during held notes. */
 synth_note(48,100);synth_note(60,100);
 for(unsigned value=0;value<128;value++){
  synth_param(36,value);synth_param(37,127-value);synth_param(38,64);
  synth_param(101,0);
  assert(v0_test_pitch_env(0)==((int)value-64)*24);
  assert(v1_test_pitch_env(1)==(63-(int)value)*24);
  synth_param(101,1);
  assert(v0_test_pitch_env(0)==golden[value]);
  assert(v1_test_pitch_env(1)==golden[127-value]);
  assert(v0_test_pitch_env(2)==0 && v1_test_pitch_env(2)==0);
 }
 synth_param(36,65);assert(v0_test_pitch_env(0)==2);
 synth_param(36,0);assert(v0_test_pitch_env(0)==-16256);
 synth_panic();assert(v0_test_pitch_env(0)==-16256);
 synth_param(101,0);assert(v0_test_pitch_env(0)==-1536);
 puts("pitch envelope curve: both voices, mode edits and panic passed");
}
