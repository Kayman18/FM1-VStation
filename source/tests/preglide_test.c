#include "synth.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int32_t v0_test_glide_offset(void),v1_test_glide_offset(void);
static float a[4096],b[4096];
int main(void){
 assert(synth_defaults[53]==1 && synth_defaults[115]==64);
 synth_init();synth_param(95,2);synth_note(60,20);synth_render(a,4096);
 synth_init();synth_param(95,2);synth_note(60,120);synth_render(b,4096);
 assert(!memcmp(a,b,sizeof(a))); /* Neutral velocity depths ignore key strength. */
 for(unsigned shape=1;shape<=2;shape++)for(unsigned v=52;v<=76;v++){
  synth_init();synth_param(95,2);synth_param(66,shape);synth_param(68,80);synth_param(115,v);
  synth_note(60,100);assert(v0_test_glide_offset()==((int)v-64)*8388608);
  synth_note(67,100);assert(v1_test_glide_offset()==((int)v-64)*8388608);
  synth_render(a,4096);
  int32_t after=v0_test_glide_offset(),start=((int)v-64)*8388608;
  if(start>0)assert(after>=0 && after<start);
  if(start<0)assert(after<=0 && after>start);
  synth_param(66,0);assert(v0_test_glide_offset()==0 && v1_test_glide_offset()==0);
 }
 synth_init();synth_param(95,2);synth_param(66,1);synth_param(68,0);synth_param(115,76);
 synth_note(60,100);assert(v0_test_glide_offset()==0);
 synth_param(68,80);synth_param(115,127);synth_note(67,100);
 assert(v1_test_glide_offset()==12*8388608);
 synth_param(116,0); /* Separate bypass cancels the active offset. */
 assert(v0_test_glide_offset()==0 && v1_test_glide_offset()==0);
 synth_note(67,100);assert(v1_test_glide_offset()==0);
 synth_param(116,1);synth_note(67,100);
 assert(v1_test_glide_offset()==12*8388608); /* Shape/time/preglide survived bypass. */
 puts("Preglide directions, first notes, both voices, limits and velocity default passed");
}
