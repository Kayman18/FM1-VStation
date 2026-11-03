#include "synth.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
uint32_t v0_test_lfo(unsigned,unsigned);
uint32_t v1_test_lfo(unsigned,unsigned);
static float audio[4096];
int main(void){
 for(unsigned shared=0;shared<2;shared++)
 for(unsigned a=0;a<3;a++)for(unsigned b=0;b<3;b++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(65,shared);
  synth_param(111,a);synth_param(112,b);
  synth_param(21,100);synth_param(27,100);
  synth_param(91,1);synth_param(92,1);
  synth_note(60,100);synth_render(audio,4096);
  uint32_t fades[2]={v0_test_lfo(0,6),v0_test_lfo(1,6)};
  uint32_t phases[2]={v0_test_lfo(0,3),v0_test_lfo(1,3)};
  assert(fades[0] && fades[1]);
  synth_note(64,100);
  for(unsigned i=0;i<2;i++){
   unsigned mode=i?b:a,reset=mode?mode==2:shared;
   assert(v0_test_lfo(i,6)==(reset?0:fades[i]));
   if(reset)assert(v0_test_lfo(i,3)==0x40000000u);
   if(!reset)assert(v0_test_lfo(i,3)==phases[i]);
  }
  synth_render(audio,4096);
  fades[0]=v0_test_lfo(0,6);fades[1]=v0_test_lfo(1,6);
  synth_note(64,0); /* Return to held note must not restart either fade. */
  assert(v0_test_lfo(0,6)==fades[0] && v0_test_lfo(1,6)==fades[1]);
  synth_note(60,0);synth_note(67,100); /* New phrase always gates. */
  assert(v0_test_lfo(0,6)==0 && v0_test_lfo(1,6)==0);
 }
 /* Pedal-held note remains an overlap under the current FM1 policy. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(111,1);synth_param(112,2);
 synth_param(21,100);synth_param(27,100);
 synth_note(60,100);synth_render(audio,4096);synth_sustain(1);synth_note(60,0);
 uint32_t fade=v0_test_lfo(0,6),phase=v0_test_lfo(1,3);
 synth_note(64,100);
 assert(v0_test_lfo(0,6)==fade && v0_test_lfo(1,6)==0);
 assert(v0_test_lfo(1,3)==phase); /* Fade reset alone must not reset phase. */
 /* Both physical voices receive the overrides; new allocations gate. */
 synth_init();synth_param(95,2);synth_param(111,1);synth_param(112,2);
 synth_param(21,100);synth_param(27,100);
 synth_note(60,100);synth_render(audio,4096);synth_note(64,100);
 assert(v1_test_lfo(0,6)==0 && v1_test_lfo(1,6)==0);
 puts("LFO independent/shared retrigger cases passed");
}
