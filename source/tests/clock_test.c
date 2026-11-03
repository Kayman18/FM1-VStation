#include "synth.h"
#include "midi_clock.h"
#include "lfo_tempo.h"
#include <assert.h>
#include <stdio.h>
uint32_t v0_test_lfo(unsigned,unsigned),v1_test_lfo(unsigned,unsigned);
static void feed(unsigned source,uint32_t start,unsigned bpm){
 for(unsigned i=0;i<=24;i++)synth_clock_event(0xf8,source,start+(60000u*i+12*bpm)/(24*bpm));
}
int main(void){
 MidiClock c;
 for(unsigned bpm=40;bpm<=240;bpm++){
  midi_clock_reset(&c);
  for(unsigned i=0;i<=24;i++)midi_clock_event(&c,0xf8,0xffffff00u+(60000u*i+12*bpm)/(24*bpm));
  assert(c.locked && c.bpm_milli>=(bpm-1)*1000 && c.bpm_milli<=(bpm+1)*1000);
  midi_clock_poll(&c,c.last+501);assert(!c.locked && !c.seen);
 }
 synth_init();synth_param(95,2);synth_param(98,6);synth_param(97,100);synth_param(100,1);
 feed(2,0,120);assert(v0_test_lfo(0,4)==lfo_tempo_increment(100,6,0));
 feed(1,1000,120);assert(v0_test_lfo(0,4)==lfo_tempo_increment(120,6,0));
 assert(v1_test_lfo(0,4)==v0_test_lfo(0,4));
 synth_param(97,90);assert(v0_test_lfo(0,4)==lfo_tempo_increment(120,6,0));
 float buf[256];synth_note(60,100);synth_render(buf,256);
 unsigned active=synth_voice_count();uint32_t free_phase=v0_test_lfo(1,3);
 synth_clock_event(0xfa,1,1600);
 assert(synth_voice_count()==active && v0_test_lfo(1,3)==free_phase);
 assert(v0_test_lfo(0,3)==0x40000000u && v1_test_lfo(0,3)==0x40000000u);
 feed(1,1700,120);synth_render(buf,256);uint32_t phase=v0_test_lfo(0,3);
 synth_clock_event(0xfc,1,2250);assert(synth_voice_count()==active);
 assert(v0_test_lfo(0,4)==lfo_tempo_increment(90,6,0));
 feed(1,2300,120);assert(v0_test_lfo(0,4)==lfo_tempo_increment(90,6,0));
 synth_clock_event(0xfb,1,2900);assert(v0_test_lfo(0,3)==phase);
 feed(1,3000,120);synth_clock_poll(4001);
 assert(v0_test_lfo(0,4)==lfo_tempo_increment(90,6,0));
 feed(1,4100,120);synth_param(100,2);
 assert(v0_test_lfo(0,4)==lfo_tempo_increment(90,6,0));
 feed(2,4700,150);assert(v0_test_lfo(0,4)==lfo_tempo_increment(150,6,0));
 /* Voice reset during an active clock retains its fractional override. */
 synth_note(67,100);synth_render(buf,256);synth_note(72,100);synth_render(buf,64);
 assert(v0_test_lfo(0,4)==lfo_tempo_increment(150,6,0));
 synth_panic();assert(v0_test_lfo(0,4)==lfo_tempo_increment(150,6,0));
 synth_param(100,0);assert(v0_test_lfo(0,4)==lfo_tempo_increment(90,6,0));
 midi_clock_reset(&c);
 for(unsigned i=0;i<=96;i++)midi_clock_event(&c,0xf8,(uint32_t)((60000000ull*i+12*123456u)/(24*123456u)));
 assert(c.locked && c.count==4 && c.bpm_milli>123400 && c.bpm_milli<123520);
 assert(lfo_tempo_increment_milli(c.bpm_milli,6,0)!=lfo_tempo_increment(123,6,0));
 uint32_t start=c.last;
 for(unsigned i=1;i<=24;i++)midi_clock_event(&c,0xf8,start+(60000u*i+12*180)/(24*180));
 assert(c.locked && c.count==1 && c.bpm_milli>179000 && c.bpm_milli<181000);
 midi_clock_poll(&c,c.last+501);assert(!c.locked && c.count==0);
 puts("PASS: clock acquisition/range/wrap/timeout, source isolation, internal fallback, transport phase and held-note preservation");
}
