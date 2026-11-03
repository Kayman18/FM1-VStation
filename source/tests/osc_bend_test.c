#include "synth.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
int32_t v0_test_bend_offset(unsigned),v1_test_bend_offset(unsigned);
static float a[4096],b[4096];
static void setup(void){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(117,1);synth_param(118,13);synth_param(119,25);
 synth_param(66,1);synth_param(68,80);synth_note(48,100);synth_pitch_bend(9001);synth_note(60,100);
}
int main(void){
 static const unsigned bends[]={0,1,4096,8191,8192,8193,12288,16383};
 synth_init();synth_param(95,2);synth_param(42,7);
 for(unsigned enc=0;enc<=25;enc++)for(unsigned b=0;b<8;b++){
  for(unsigned i=0;i<3;i++)synth_param(117+i,enc);
  synth_pitch_bend(bends[b]);
  int range=enc?(int)enc-13:7;
  int32_t expected=((int)bends[b]-8192)*range*1024;
  for(unsigned i=0;i<3;i++)assert(v0_test_bend_offset(i)==expected && v1_test_bend_offset(i)==expected);
 }
 synth_param(117,0);synth_param(118,1);synth_param(119,13);synth_pitch_bend(16383);
 synth_param(42,24);
 assert(v0_test_bend_offset(0)==8191*24*1024);
 assert(v0_test_bend_offset(1)==-8191*12*1024 && v0_test_bend_offset(2)==0);
 synth_reset_controllers();for(unsigned i=0;i<3;i++)assert(v0_test_bend_offset(i)==0);
 setup();synth_render(a,4096);
 setup();for(unsigned i=0;i<4096;i++)synth_render(b+i,1);
 assert(!memcmp(a,b,sizeof(a)));
 synth_panic();for(unsigned i=0;i<3;i++)assert(v0_test_bend_offset(i)==0 && v1_test_bend_offset(i)==0);
 puts("Independent oscillator bend ranges, shared fallback, reset and glide partition passed");
}
