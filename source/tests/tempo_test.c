#include "synth.h"
#include "lfo_tempo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
uint32_t v0_test_lfo(unsigned,unsigned),v1_test_lfo(unsigned,unsigned);
static float a[4096],b[4096];
int main(void){
 static const unsigned num[]={16,8,4,2,3,1,2,3,1,1,1,1,1,1,3,4,8,3,16,6,32,12,40,18,56,20,64,24,28,30,32,36,42,48};
 static const unsigned den[]={1,1,1,1,2,1,3,4,2,3,4,8,12,6,8,3,3,1,3,1,3,1,3,1,3,1,3,1,1,1,1,1,1,1};
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned d=1;d<=LFO_TEMPO_DIVISIONS;d++){
  double expected=4294967296.0*16*bpm*den[d-1]/(44100.0*60*num[d-1]);
  assert(fabs(lfo_tempo_increment(bpm,d,0)-expected)<=expected*2e-7+1);
 }
 assert(lfo_tempo_increment(120,0,123456)==123456);
 assert(lfo_tempo_increment(0,6,0)==lfo_tempo_increment(40,6,0));
 assert(lfo_tempo_increment(999,6,0)==lfo_tempo_increment(240,6,0));
 synth_init();synth_param(95,2);synth_param(24,64);synth_note(60,100);synth_note(67,100);synth_render(a,4096);
 synth_init();synth_param(95,2);synth_param(24,64);synth_param(97,200);synth_note(60,100);synth_note(67,100);synth_render(b,4096);
 assert(!memcmp(a,b,sizeof(a))); /* FREE is unchanged by tempo. */
 uint32_t phase=v0_test_lfo(0,3),free_rate=v0_test_lfo(0,4);
 synth_param(98,6);synth_param(99,9);
 assert(v0_test_lfo(0,3)==phase); /* Division edit preserves phase. */
 for(unsigned i=0;i<2;i++){
  unsigned d=i?9:6;
  assert(v0_test_lfo(i,4)==lfo_tempo_increment(200,d,0));
  assert(v1_test_lfo(i,4)==v0_test_lfo(i,4));
 }
 synth_param(20,1);assert(v0_test_lfo(0,4)==lfo_tempo_increment(200,6,0));
 synth_param(97,120);assert(v0_test_lfo(0,3)==phase);
 synth_panic();assert(v0_test_lfo(0,4)==lfo_tempo_increment(120,6,0));
 synth_note(60,100);synth_note(67,100);synth_render(a,256);synth_note(72,100);synth_render(a,64);
 assert(v0_test_lfo(0,4)==lfo_tempo_increment(120,6,0));
 synth_param(98,0);assert(v0_test_lfo(0,4)!=free_rate); /* Latest free knob retained. */
 synth_param(20,synth_defaults[20]);assert(v0_test_lfo(0,4)==free_rate);
 puts("PASS: tempo rates, clamping, FREE equivalence, phase continuity, independent divisions, panic and steal restoration");
}
