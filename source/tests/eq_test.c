#include "native_eq.h"
#include "synth.h"
#include "midi_level.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const NativeEq *synth_test_eq(void);
extern unsigned synth_test_eq_enabled(void);
static float plain[8192],wet[8192],split[8192];
static uint32_t seed=0x15ab1793u;
static unsigned random_u(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void setup(unsigned mode,unsigned cap,unsigned enabled,unsigned level){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 synth_param(128,85);synth_param(129,96);synth_param(130,80);synth_controller(0,99);synth_param(131,1);
 synth_param(120,52);synth_param(121,1);
 synth_param(132,101);synth_param(133,84);synth_param(134,63);synth_param(135,72);synth_param(137,enabled);
 if(level){synth_midi_level(7,83);synth_midi_level(11,107);}
 synth_note(48,100);synth_note(55,90);synth_note(60,110);
}
static void feed(unsigned source,uint32_t start,unsigned bpm){
 for(unsigned i=0;i<=24;i++)synth_clock_event(0xf8,source,start+(60000u*i+12*bpm)/(24*bpm));
}
static uint32_t rate(unsigned bpm,unsigned div){NativeEq e;native_eq_init(&e);native_eq_tempo(&e,(float)bpm);native_eq_sync(&e,div);return e.lfo[4];}
int main(void){
 NativeEq e,saved;native_eq_init(&e);
 assert(sizeof e==116 && e.frequency==78 && e.direct_gain==1.f && e.low_gain==0.f);
 assert(e.lfo[4]==0 && !e.sync && !e.initial_position);
 for(unsigned i=0;i<128;i++)assert(native_eq_table_44100[i]>0.f && native_eq_table_44100[i]<1.f);
 for(unsigned i=1;i<128;i++)assert(native_eq_table_44100[i]<native_eq_table_44100[i-1]);
 for(unsigned frequency=0;frequency<128;frequency++){
  native_eq_frequency(&e,frequency);native_eq_modulation(&e,1.f);native_eq_rate(&e,1.f);
  for(unsigned i=0;i<512;i++){float x=((int)(random_u()%65537)-32768)/16384.f;assert(native_eq_step(&e,x)==x);}
 }
 for(unsigned depth=0;depth<128;depth+=3)for(unsigned frequency=0;frequency<128;frequency+=5){
  native_eq_init(&e);native_eq_depth(&e,((int)depth-64)/128.f);native_eq_frequency(&e,frequency);native_eq_modulation(&e,(random_u()%128)/127.f);native_eq_rate(&e,(random_u()%128)/127.f);
  for(unsigned i=0;i<256;i++){float x=((int)(random_u()%65537)-32768)/16384.f;float y=native_eq_step(&e,x);assert(isfinite(y)&&fabsf(y)<=8.f);assert(fabsf(e.low)<=2.f);}
 }
 for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  setup(mode,cap,0,0);synth_render(plain,8192);
  setup(mode,cap,1,1);synth_render(wet,8192);
  native_eq_init(&e);native_eq_depth(&e,37/128.f);native_eq_frequency(&e,84);native_eq_modulation(&e,63/127.f);native_eq_rate(&e,72/127.f);
  for(unsigned i=0;i<8192;i++)split[i]=native_eq_step(&e,plain[i]);
  MidiLevel m;midi_level_init(&m);midi_level_set(&m,7,83);midi_level_set(&m,11,107);midi_level_apply(&m,split,8192);
  assert(!memcmp(wet,split,sizeof wet));
  setup(mode,cap,1,1);for(unsigned i=0;i<8192;){unsigned n=1+random_u()%211;if(n>8192-i)n=8192-i;synth_render(split+i,n);i+=n;}
  assert(!memcmp(wet,split,sizeof wet));assert(!synth_faults);
  saved=*synth_test_eq();synth_param(137,0);synth_render(split,512);assert(!memcmp(&saved,synth_test_eq(),sizeof saved));
  synth_param(137,1);synth_reset_controllers();assert(!memcmp(&saved,synth_test_eq(),sizeof saved));
  synth_panic();assert(!synth_test_eq()->low && !synth_test_eq()->lfo[3] && !synth_test_eq()->lfo[6]);
  synth_render(split,512);for(unsigned i=0;i<512;i++)assert(split[i]==0.f);
 }
 synth_init();assert(!synth_test_eq_enabled());
 synth_param(132,1000);assert(synth_test_eq()->depth==126/128.f);
 synth_param(133,1000);assert(synth_test_eq()->frequency==127);
 synth_param(134,1000);assert(synth_test_eq()->modulation_scale==160);
 synth_param(135,1000);assert(synth_test_eq()->free_rate_squared==1.f);
 synth_param(136,1000);assert(synth_test_eq()->period==48.f);
 synth_param(137,1000);assert(synth_test_eq_enabled());
 saved=*synth_test_eq();synth_param(SYNTH_PARAM_COUNT,0);assert(!memcmp(&saved,synth_test_eq(),sizeof saved));
 unsigned values[SYNTH_PARAM_COUNT];memcpy(values,synth_defaults,sizeof values);uint8_t snapshot[PRESET_BYTES];assert(preset_encode(values,snapshot,sizeof snapshot));assert(preset_apply(snapshot,sizeof snapshot));
 assert(!synth_test_eq_enabled() && !synth_test_eq()->depth && !synth_test_eq()->sync && synth_test_eq()->frequency==78);
 /* Native EQ divisions follow the selected FM1 clock without imposing any new
    note/transport phase resets. Source loss returns to the retained BPM. */
 synth_param(135,91);uint32_t free_rate=synth_test_eq()->lfo[4];synth_param(136,10);synth_param(97,100);synth_param(100,1);synth_param(137,1);
 feed(2,0,120);assert(synth_test_eq()->lfo[4]==rate(100,10));
 feed(1,1000,120);assert(synth_test_eq()->lfo[4]==rate(120,10));
 synth_param(97,90);assert(synth_test_eq()->lfo[4]==rate(120,10));
 synth_note(60,100);synth_render(split,256);uint32_t phase=synth_test_eq()->lfo[3];
 synth_clock_event(0xfa,1,1600);assert(synth_test_eq()->lfo[3]==phase);
 feed(1,1700,120);synth_clock_poll(2801);assert(synth_test_eq()->lfo[4]==rate(90,10));
 synth_param(100,2);feed(2,3000,150);assert(synth_test_eq()->lfo[4]==rate(150,10));
 synth_panic();assert(synth_test_eq()->lfo[4]==rate(150,10));
 synth_clock_event(0xfc,2,3600);assert(synth_test_eq()->lfo[4]==rate(90,10));
 synth_param(100,0);synth_param(136,0);assert(synth_test_eq()->lfo[4]==free_rate);
 /* Every native division at all supported internal tempos is finite. */
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned div=0;div<=34;div++){
  synth_param(97,bpm);synth_param(136,div);synth_render(split,32);for(unsigned i=0;i<32;i++)assert(isfinite(split[i]));
 }
 puts("PASS: EQ neutral/bounds, exact distortion-master-EQ-MIDI ordering, nine voice policies, block invariance, bypass/panic/Init, selected-clock/fallback/free-rate and 7035 tempo/division cases");
}
