#include "synth.h"
#include "amp_level.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static float plain[4096],scaled[4096],split[4096];
static void setup(unsigned voices, unsigned original){
 synth_init();synth_param(95,2);synth_param(95,voices);synth_param(120,52);
 synth_param(121,original);synth_note(60,100);
 if(voices>=2)synth_note(67,100);if(voices==3)synth_note(72,100);
}
int main(void){
 for(unsigned voices=1;voices<=3;voices++){
  setup(voices,0);synth_render(plain,4096);
  setup(voices,1);synth_render(scaled,4096);
  float gain=1.f,target=amp_level_curve(52);unsigned nonzero=0;
  for(unsigned i=0;i<4096;i++){
   gain=target*.01f+gain*.99f;
   assert(scaled[i]==plain[i]*gain);
   nonzero+=plain[i]!=0.f;
  }
  assert(nonzero>1000); /* Check real audio, not an accidentally silent path. */
  setup(voices,1);
  for(unsigned i=0;i<4096;){unsigned n=(i%37)+1;if(n>4096-i)n=4096-i;synth_render(split+i,n);i+=n;}
  assert(!memcmp(scaled,split,sizeof(scaled)));
  synth_midi_level(7,0);synth_render(split,4096);
  for(unsigned i=256;i<4096;i++)assert(split[i]==0.f);
  synth_panic();synth_render(split,4096);
  for(unsigned i=0;i<4096;i++)assert(split[i]==0.f);
  assert(!synth_faults);
 }
 puts("PASS: mono/poly amplitude dispatch, exact gain order, irregular block invariance, MIDI mute and panic");
}
