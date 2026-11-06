/* Voice panic rebuild equivalence.
   synth_panic() resets a voice and then re-derives everything that depends on
   p[].  The light rebuild does that directly; -DPANIC_FORCE_FULL replays all
   170 parameter setters instead.  This test renders the same voice-stealing
   sequence for every parameter id, value and a spread of factory presets and
   prints one hash of all of it, so the two builds can be compared bit for bit.
   Run both ways and require identical output (tests/run.sh does this). */
#include "synth.h"
#include "preset.h"
#include "../app/factory_presets.h"
#include <stdio.h>
#include <string.h>
int main(void){unsigned v[SYNTH_PARAM_COUNT];uint8_t b[PRESET_BYTES];float l[128],r[128];
 uint64_t all=1469598103934665603ull;unsigned faults=0;
 static const unsigned vals[]={0,1,33,64,100,127};
 for(unsigned pr=18;pr<23 && pr<FACTORY_PRESET_COUNT;pr++)
 for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++)
 for(unsigned vi=0;vi<sizeof vals/sizeof*vals;vi++){
  uint64_t h=1469598103934665603ull;
  synth_init();
  for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++)v[i]=factory_preset_values[pr][i];
  v[127]=3;v[168]=8;preset_encode(v,b,sizeof b);preset_apply(b,sizeof b);
  synth_param(id,vals[vi]);
  synth_note(60,100);
  for(int k=0;k<40;k++)synth_render_stereo(l,r,128);
  synth_note(67,110);                 /* pool full -> steal -> synth_panic() */
  for(int k=0;k<60;k++){synth_render_stereo(l,r,128);
   for(int i=0;i<128;i++){uint32_t x,y;memcpy(&x,l+i,4);memcpy(&y,r+i,4);
    h^=x;h*=1099511628211ull;h^=y;h*=1099511628211ull;}}
  synth_param(id,vals[(vi+3)%(sizeof vals/sizeof*vals)]);   /* change it again */
  synth_note(64,90);
  for(int k=0;k<40;k++){synth_render_stereo(l,r,128);
   for(int i=0;i<128;i++){uint32_t x,y;memcpy(&x,l+i,4);memcpy(&y,r+i,4);
    h^=x;h*=1099511628211ull;h^=y;h*=1099511628211ull;}}
  faults+=synth_faults;
  all^=h;all*=1099511628211ull;}
 if(faults){printf("FAIL: %u synth faults\n",faults);return 1;}
 printf("PANIC-REBUILD %016llx\n",(unsigned long long)all);return 0;}
