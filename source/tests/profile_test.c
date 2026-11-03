#include "synth.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t now_us;
static uint32_t fake_clock(void){ now_us += 7; return now_us; }
int main(void){
 SynthProfile p;
 synth_init();
 synth_profile_config(fake_clock);
 synth_param(127,3); /* native poly */
 synth_param(137,1); synth_param(142,1); synth_param(151,1); synth_param(159,1); synth_param(167,1);
 synth_note(48,100); synth_note(55,96); synth_note(60,104);
 float l[128],r[128];
 synth_profile_block_begin();
 synth_render_stereo(l,r,128); synth_render_stereo(l,r,128);
 synth_profile_block_end();
 synth_profile_read(&p);
 assert(p.max_us[SYNTH_PROF_VOICES]>0);
 assert(p.max_us[SYNTH_PROF_EQ]>0);
 assert(p.max_us[SYNTH_PROF_PAN]>0);
 assert(p.max_us[SYNTH_PROF_DELAY]>0);
 assert(p.max_us[SYNTH_PROF_CHORUS]>0);
 assert(p.max_us[SYNTH_PROF_RVB_SETUP]>0);
 assert(p.max_us[SYNTH_PROF_RVB_MIX]>0);
 assert(p.max_us[SYNTH_PROF_MIDI_LEVEL]>0);
 synth_profile_reset(); synth_profile_read(&p);
 for(unsigned i=0;i<SYNTH_PROF_STAGE_COUNT;i++) assert(p.max_us[i]==0);
 puts("PASS: TEST26 per-half stage profiler accumulates and resets");
 return 0;
}
