#include "synth.h"
#include <assert.h>
#include <stdio.h>
extern float v0_test_fm_amount(void),v1_test_fm_amount(void);
int main(void){
 synth_init();synth_param(95,2);synth_note(48,100);synth_note(60,100);
 for(unsigned v=0;v<128;v++){
  int delta=(int)v-64;
  float original=(float)(delta*delta)/4096.f;if(delta<0)original=-original;
  synth_param(106,0);synth_param(46,v);
  assert(v0_test_fm_amount()==original*4.f);
  assert(v1_test_fm_amount()==original*4.f);
  synth_param(106,1);
  assert(v0_test_fm_amount()==original && v1_test_fm_amount()==original);
  synth_panic();assert(v0_test_fm_amount()==original);
 }
 synth_param(46,64);assert(v0_test_fm_amount()==0.f);
 synth_param(46,0);assert(v1_test_fm_amount()==-1.f);
 synth_param(106,0);assert(v1_test_fm_amount()==-4.f);
 puts("FM envelope scaling: all depths, both voices, mode edits passed");
}
