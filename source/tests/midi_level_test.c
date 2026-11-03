#include "synth.h"
#include "midi_level.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
unsigned synth_test_pending(unsigned);
static float a[512],b[512];
static void drain_tail(void){for(unsigned i=0;i<10000;i++)synth_render(a,256);assert(!synth_voice_count());}
int main(void){
 MidiLevel x,y;midi_level_init(&x);for(unsigned i=0;i<512;i++)a[i]=b[i]=1.f;
 midi_level_apply(&x,a,512);assert(!memcmp(a,b,sizeof(a)));
 midi_level_set(&x,7,0);y=x;midi_level_apply(&x,a,512);
 for(unsigned i=0;i<512;i+=32)midi_level_apply(&y,b+i,32);
 assert(!memcmp(a,b,sizeof(a)));assert(a[0]==255.f/256.f && a[255]==0 && a[511]==0);
 midi_level_set(&x,7,127);for(unsigned i=0;i<512;i++)a[i]=1;
 midi_level_apply(&x,a,17);unsigned remaining=x.remaining;
 midi_level_set(&x,7,127);assert(x.remaining==remaining);
 float previous=x.current;midi_level_set(&x,11,64);assert(x.current==previous);
 midi_level_apply(&x,a,256);assert(x.current==64.f/127.f && !x.remaining);
 midi_level_set(&x,99,0);assert(x.target==64.f/127.f);
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);synth_render(a,256);
 synth_midi_level(7,0);synth_render(a,256);synth_render(a,256);
 for(unsigned i=0;i<256;i++)assert(a[i]==0);assert(synth_voice_count()==2);
 synth_reset_controllers();synth_render(a,256);
 for(unsigned i=0;i<256;i++)assert(a[i]==0); /* CC121 preserves CC7 volume. */
 synth_all_notes_off();assert(synth_voice_count()==2);drain_tail();
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);synth_sustain(1);synth_render(a,256);
 synth_all_notes_off();synth_render(a,256);assert(synth_voice_count()==2);
 synth_sustain(0);drain_tail();
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);synth_render(a,256);
 synth_note(72,100);assert(synth_test_pending(0)==72);synth_all_notes_off();
 assert(synth_test_pending(0)==128);drain_tail();
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(60,100);synth_note(67,100);synth_render(a,256);
 synth_all_notes_off();assert(synth_voice_count()==1);drain_tail();
 assert(!synth_faults);
 puts("PASS: unity bypass, ramp/block invariance, repeated/retargeted controls, mute, controller reset, release-all/sustain/pending cancellation");
}
