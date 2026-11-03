#include "synth.h"
#include "velocity.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
unsigned v0_test_native_state(unsigned,unsigned),v1_test_native_state(unsigned,unsigned);
unsigned v0_test_return_gates(unsigned);
float v0_test_native_amp_gain(void);
int32_t v0_test_glide_offset(void);
static float block[512],whole[2048],split[2048];
static unsigned state(unsigned field){return v0_test_native_state(field,0);}
static void switches(unsigned mask){
 for(unsigned i=0;i<3;i++)synth_param(62+i,(mask>>i)&1u);
 for(unsigned i=0;i<2;i++)synth_param(111+i,mask&(8u<<i)?2:1);
}
static void init(unsigned mode){synth_init();synth_param(127,mode);}
static void list(unsigned count,const unsigned *notes,const unsigned *velocities){
 assert(state(0)==count);
 for(unsigned i=0;i<count;i++){
  assert(v0_test_native_state(3,i)==notes[i]);
  assert(v0_test_native_state(4,i)==velocities[i]);
 }
 assert(v0_test_native_state(3,count)==128);
}
int main(void){
 assert(synth_defaults[95]==3 && synth_defaults[127]==0);
 for(unsigned mode=1;mode<=2;mode++){
  init(mode);synth_param(49,127);synth_note(60,30);synth_note(60,100);
  const unsigned nn[]={60,60},vv[]={30,100};list(2,nn,vv);
  float gain=v0_test_native_amp_gain();
  synth_note(60,0);list(1,nn,vv+1);assert(v0_test_native_amp_gain()==gain);
  assert(state(1)==0 && state(5)!=3 && synth_voice_count()==1);
  synth_note(60,0);assert(state(0)==0 && state(1)==1 && state(5)==3);
  synth_note(60,0);assert(state(0)==0); /* Unknown release is ignored. */
  init(mode);synth_param(49,127);synth_note(48,30);synth_render(block,512);
  synth_note(60,100);synth_render(block,512);synth_note(60,0);
  assert(state(0)==1 && state(6)==48);
  assert(v0_test_native_amp_gain()==velocity_gain(30,63/128.f,0));
  assert(v1_test_native_state(0,0)==0); /* Configured cap3 is not polyphony in native mono. */
  /* All 32 retained gate combinations, including edits before return. */
  for(unsigned mask=0;mask<32;mask++){
   init(mode);switches(mask);synth_note(48,100);
   assert(v0_test_return_gates(1)==31); /* Empty physical list always gates. */
   synth_render(block,512);synth_note(60,110);
   assert(v0_test_return_gates(0)==mask && v0_test_return_gates(1)==mask);
   switches(mask^31u);synth_note(60,0);
   assert(v0_test_return_gates(1)==mask); /* Native return is latched even p126=0. */
  }
  /* Pedal keeps sound, not held-note membership or overlap. */
  init(mode);switches(0);synth_sustain(1);synth_note(60,100);synth_render(block,512);
  synth_note(60,0);assert(state(0)==0 && state(1)==1 && state(5)!=3);
  synth_note(67,100);assert(state(0)==1 && !state(1));
  assert(v0_test_return_gates(1)==31);
  synth_note(67,0);synth_sustain(0);assert(state(5)==3 && state(8)==3);
  /* Pedal-up cannot release a physically held tail. */
  init(mode);synth_sustain(1);synth_note(60,100);synth_note(67,100);
  synth_note(67,0);assert(state(0)==1 && state(6)==60);
  synth_sustain(0);assert(state(5)!=3);
  /* CC123-style release clears all membership even while sustaining. */
  synth_sustain(1);synth_all_notes_off();assert(state(0)==0 && state(1)==1 && state(5)!=3);
  synth_reset_controllers();assert(!state(7) && state(5)==3);
  /* A native fresh gate re-derives glide from last note, not live pitch. */
  init(mode);synth_param(66,1);synth_param(68,90);synth_param(67,mode==1?1:0);
  synth_note(48,100);assert(v0_test_glide_offset()==0);
  synth_note(60,100);assert(v0_test_glide_offset()==-12*8388608);
  synth_render(block,512);assert(v0_test_glide_offset()!=-12*8388608);
  synth_note(67,100);assert(v0_test_glide_offset()==-7*8388608);
  synth_note(67,0);assert(v0_test_glide_offset()==0 && state(6)==60);
  synth_all_notes_off();synth_note(72,100);
  assert(v0_test_glide_offset()==(mode==1?-12*8388608:0));
  synth_panic();synth_param(115,76);synth_note(60,100);
  assert(v0_test_glide_offset()==12*8388608); /* Preglide wins even without overlap. */
  synth_note(67,100);synth_note(67,0);assert(v0_test_glide_offset()==0);
  /* Bounded duplicate ledger; rejecting overflow must not corrupt old entries. */
  init(mode);for(unsigned i=0;i<128;i++)synth_note(60,1+i%127);
  assert(state(0)==128);synth_note(60,127);assert(state(0)==128 && synth_note_overflows()==1);
  for(unsigned i=0;i<128;i++){assert(v0_test_native_state(4,0)==1+i%127);synth_note(60,0);}
  assert(state(0)==0);synth_note(60,0);assert(state(0)==0);
  synth_panic();assert(!synth_note_overflows());
  /* Rendering partition must not alter the policy or sound. */
  init(mode);synth_note(48,100);synth_render(block,512);synth_note(60,90);synth_note(60,0);synth_render(whole,2048);
  init(mode);synth_note(48,100);synth_render(block,512);synth_note(60,90);synth_note(60,0);
  for(unsigned i=0;i<2048;i+=16)synth_render(split+i,16);
  assert(!memcmp(whole,split,sizeof whole) && !synth_faults);
 }
 /* Changing mode kills old voices/pending steals; returning to FM1 retains cap3. */
 synth_init();synth_note(48,100);synth_note(60,100);synth_note(72,100);assert(synth_voice_count()==3);
 synth_param(127,1);assert(synth_voice_count()==0);synth_note(60,100);assert(synth_voice_count()==1);
 synth_param(127,0);assert(synth_voice_count()==0);
 synth_note(48,100);synth_note(60,100);synth_note(72,100);assert(synth_voice_count()==3);
 puts("PASS: recovered mono/mono-AG gates, duplicate FIFO, velocity, sustain separation, return, glide, capacity, mode switching and partition");
}
