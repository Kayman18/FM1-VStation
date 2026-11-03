#include "synth.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
unsigned v0_test_return_gates(unsigned),v1_test_return_gates(unsigned);
uint32_t v0_test_lfo(unsigned,unsigned);
static float audio[512],whole[2048],split[2048];
static void switches(unsigned mask){
 for(unsigned i=0;i<3;i++)synth_param(62+i,(mask>>i)&1u);
 for(unsigned i=0;i<2;i++)synth_param(111+i,mask&(8u<<i)?2:1);
}
static void start(unsigned mode){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(126,mode);
 synth_param(21,64);synth_param(27,64);
 synth_note(60,100);synth_render(audio,512);
}
int main(void){
 assert(synth_defaults[126]==0);
 for(unsigned mode=0;mode<2;mode++)for(unsigned mask=0;mask<32;mask++){
  start(mode);switches(mask);synth_note(67,120);
  assert(v0_test_return_gates(0)==mask);
  synth_render(audio,512);
  uint32_t fade[2]={v0_test_lfo(0,6),v0_test_lfo(1,6)};
  switches(mask^31u);synth_note(67,0);
  unsigned expected=mode?mask:((mask^31u)&7u);
  assert(v0_test_return_gates(1)==expected);
  assert(v0_test_return_gates(0)==mask); /* Return does not overwrite latch. */
  for(unsigned i=0;i<2;i++)assert(v0_test_lfo(i,6)==(expected&(8u<<i)?0:fade[i]));
  assert(v1_test_return_gates(0)==0); /* Mono gate state is voice-local. */
  synth_panic();assert(v0_test_return_gates(0)==0 && v0_test_return_gates(1)==0);
 }
 start(1);switches(21);synth_note(67,110);synth_render(audio,512);
 synth_sustain(1);synth_note(67,0);switches(0);
 assert(v0_test_return_gates(0)==21);
 synth_sustain(0);assert(v0_test_return_gates(1)==21);
 /* Repeated note is a fresh note-on and replaces the saved gate flags. */
 synth_note(60,100);assert(v0_test_return_gates(0)==0);
 /* Releases of older, non-playing notes do not perform a return. */
 switches(31);synth_note(72,100);synth_render(audio,512);synth_note(60,0);
 assert(v0_test_return_gates(1)==31);
 start(1);switches(31);synth_note(67,110);synth_render(audio,512);synth_note(67,0);
 synth_render(whole,2048);
 start(1);switches(31);synth_note(67,110);synth_render(audio,512);synth_note(67,0);
 for(unsigned i=0;i<2048;i+=16)synth_render(split+i,16);
 assert(!memcmp(whole,split,sizeof(whole)) && !synth_faults);
 puts("PASS: live/latched note return, all 32 gate masks, LFO fades, sustain, repeat, panic and partition");
}
