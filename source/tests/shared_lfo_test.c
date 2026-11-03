#include "synth.h"
#include "shared_lfo.h"
#include "voice_internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
const uint32_t *v0_test_selected_lfo(unsigned),*v1_test_selected_lfo(unsigned);
static float a[4096],b[4096];
static void setup(unsigned voices,unsigned mask){
 synth_init();synth_param(95,2);synth_param(95,voices);
 synth_param(24,80);synth_param(23,60);synth_param(19,3);
 synth_param(113,mask&1);synth_param(114,mask&2);
 synth_note(48,100);synth_note(60,90);if(voices==3)synth_note(67,80);
}
int main(void){
 for(unsigned voices=1;voices<=3;voices++)for(unsigned mask=1;mask<4;mask++){
  setup(voices,mask);synth_render(a,4096);
  uint32_t state[2][14];memcpy(state,shared_lfo_state,sizeof(state));
  setup(voices,mask);
  unsigned pos=0;while(pos<4096){unsigned n=(pos*17+3)%47+1;if(n>4096-pos)n=4096-pos;synth_render(b+pos,n);pos+=n;}
  assert(!memcmp(a,b,sizeof(a)));
  assert(!memcmp(state,shared_lfo_state,sizeof(state)));
  for(unsigned i=0;i<2;i++)if(mask&(1u<<i)){
   assert(v0_test_selected_lfo(i)==shared_lfo_state[i]);
   assert(v1_test_selected_lfo(i)==shared_lfo_state[i]);
  }
 }
 setup(2,1);
 uint32_t phase=shared_lfo_state[0][3],rate=shared_lfo_state[0][4];
 synth_render(a,33);
 assert(shared_lfo_state[0][3]==phase+3u*rate); /* Not once per voice. */
 phase=shared_lfo_state[0][3];synth_param(20,75);
 assert(shared_lfo_state[0][3]==phase); /* Rate edit preserves phase. */
 assert(shared_lfo_state[0][4]==v0_private_lfo(0)[4]);
 uint32_t private_state[14];memcpy(private_state,v1_private_lfo(0),sizeof(private_state));
 synth_param(113,0);
 assert(v1_test_selected_lfo(0)==v1_private_lfo(0));
 assert(!memcmp(private_state,v1_private_lfo(0),sizeof(private_state)));
 synth_param(113,1);assert(!memcmp(shared_lfo_state[0],v0_private_lfo(0),sizeof(private_state)));
 synth_note(67,100);synth_render(a,64); /* Voice steal must not panic the shared clock. */
 assert(shared_lfo_mask==1);
 synth_panic();assert(shared_lfo_state[0][3]==0 && shared_lfo_mask==1);
 assert(!synth_faults);
 puts("Shared LFO ownership, clock, switching and partition checks passed");
}
