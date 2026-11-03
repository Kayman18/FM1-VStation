#include "synth.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
unsigned synth_test_owner(unsigned),synth_test_pending(unsigned);
unsigned v2_test_pressure(void);
int32_t v2_test_bend_offset(unsigned),v2_test_drift(unsigned);
static float mono[3][4096],poly[4096],split[4096],buf[256];
static void chord(void){synth_init();synth_note(60,100);synth_note(64,100);synth_note(67,100);}
int main(void){
 assert(synth_defaults[95]==3);
 unsigned notes[3]={60,64,67};
 for(unsigned i=0;i<3;i++){synth_init();synth_param(95,1);synth_note(notes[i],100);synth_render(mono[i],4096);}
 chord();assert(synth_voice_count()==3);synth_render(poly,4096);
 for(unsigned k=0;k<4096;k++){
  float gain=k<64?k/64.f:1.f,sum=0;
  for(unsigned i=0;i<3;i++)sum+=mono[i][k]*gain;
  assert(poly[k]==sum*(1.f/3.f));
 }
 chord();for(unsigned i=0;i<4096;i+=16)synth_render(split+i,16);
 assert(!memcmp(poly,split,sizeof(poly)));
 synth_note_pressure(67,31);assert(v2_test_pressure()==31);
 synth_param(119,1);synth_pitch_bend(16383);assert(v2_test_bend_offset(2)<0);
 synth_param(125,127);assert(v2_test_drift(1)>0);
 /* Fourth note steals oldest; released third slot takes priority next. */
 synth_note(72,100);assert(synth_test_pending(0)==72);synth_note(60,0);
 synth_render(buf,64);assert(synth_test_owner(0)==72 && synth_test_pending(0)==128);
 synth_note(67,0);synth_note(74,100);assert(synth_test_pending(2)==74);
 synth_note(74,0);synth_render(buf,128);assert(synth_test_owner(2)==128);
 synth_param(95,2);assert(synth_voice_count()==0);synth_note(60,100);synth_note(64,100);synth_note(67,100);
 assert(synth_voice_count()==2 && synth_test_owner(2)==128);
 synth_param(95,99);assert(!synth_voice_count());chord();
 unsigned seed=11;
 for(unsigned event=0;event<20000;event++){
  seed=seed*1664525u+1013904223u;
  if(event%17==0)synth_sustain((event/17)&1);
  else if(event%23==0)synth_reset_controllers();
  else synth_note(24+(seed>>24)%72,seed&1?100:0);
  unsigned n=1+seed%256;synth_render(buf,n);
  assert(synth_voice_count()<=3 && !synth_faults);
  for(unsigned i=0;i<n;i++)assert(isfinite(buf[i]) && fabsf(buf[i])<=1.f);
 }
 synth_panic();synth_render(buf,256);for(unsigned i=0;i<256;i++)assert(buf[i]==0.f);
 puts("PASS: three-voice sum, partition, pressure/bend/drift, steal/cancel, limit changes, 20000 stress events");
}
