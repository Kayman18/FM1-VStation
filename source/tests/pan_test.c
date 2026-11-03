#include "native_pan.h"
#include "synth.h"
#include "midi_level.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const NativePan *synth_test_pan(void);
extern unsigned synth_test_pan_enabled(void);
static float plain[8192],left[8192],right[8192],split_l[8192],split_r[8192];
static uint32_t seed=0x16ab1793u;
static unsigned random_u(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void setup(unsigned mode,unsigned cap,unsigned enabled,unsigned level){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 synth_param(128,85);synth_param(129,96);synth_param(130,80);synth_controller(0,99);synth_param(131,1);
 synth_param(120,52);synth_param(121,1);
 synth_param(132,101);synth_param(133,84);synth_param(134,63);synth_param(135,72);synth_param(137,1);
 synth_param(138,75);synth_param(139,95);synth_param(140,97);synth_param(142,enabled);
 if(level){synth_midi_level(7,83);synth_midi_level(11,107);}
 synth_note(48,100);synth_note(55,90);synth_note(60,110);
}
static void feed(unsigned source,uint32_t start,unsigned bpm){
 for(unsigned i=0;i<=24;i++)synth_clock_event(0xf8,source,start+(60000u*i+12*bpm)/(24*bpm));
}
static uint32_t rate(unsigned bpm,unsigned div){NativePan p;native_pan_init(&p);native_pan_tempo(&p,(float)bpm);native_pan_sync(&p,div);return p.increment;}
int main(void){
 NativePan p,saved;native_pan_init(&p);assert(sizeof p==60 && !p.position && !p.depth && !p.increment && p.fade==1.f);
 for(unsigned rate=0;rate<128;rate++){
  native_pan_init(&p);native_pan_rate(&p,rate/127.f);
  for(unsigned i=0;i<1024;i++){float x=((int)(random_u()%65537)-32768)/16384.f,l,r;native_pan_step(&p,x,&l,&r);assert(l==x && r==x);}
 }
 for(unsigned pos=0;pos<128;pos+=3)for(unsigned depth=0;depth<128;depth+=5){
  native_pan_init(&p);native_pan_position(&p,((int)pos-64)/128.f);native_pan_depth(&p,depth/127.f);native_pan_rate(&p,(random_u()%128)/127.f);
  for(unsigned i=0;i<256;i++){float x=((int)(random_u()%65537)-32768)/16384.f,l,r;native_pan_step(&p,x,&l,&r);assert(isfinite(l)&&isfinite(r));assert(fabsf(l)<=4.f && fabsf(r)<=4.f);assert(fabsf((l+r)*.5f-x)<1e-6f);assert(fabsf(p.smoothed)<=1.f);}
 }
 native_pan_init(&p);native_pan_position(&p,-.5f);float l,r;
 for(unsigned i=0;i<1024;i++)native_pan_step(&p,1.f,&l,&r);
 assert(l>1.999f && r<.001f);
 native_pan_position(&p,63/128.f);for(unsigned i=0;i<1024;i++)native_pan_step(&p,1.f,&l,&r);
 assert(r>1.984f && l<.016f); /* native +63 is intentionally short of +1 */
 for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  setup(mode,cap,0,0);synth_render(plain,8192);
  setup(mode,cap,1,1);synth_render_stereo(left,right,8192);
  native_pan_init(&p);native_pan_position(&p,11/128.f);native_pan_depth(&p,95/127.f);native_pan_rate(&p,97/127.f);
  for(unsigned i=0;i<8192;i++)native_pan_step(&p,plain[i],split_l+i,split_r+i);
  MidiLevel m;midi_level_init(&m);midi_level_set(&m,7,83);midi_level_set(&m,11,107);midi_level_apply_stereo(&m,split_l,split_r,8192);
  assert(!memcmp(left,split_l,sizeof left) && !memcmp(right,split_r,sizeof right));
  setup(mode,cap,1,1);for(unsigned i=0;i<8192;){unsigned n=1+random_u()%211;if(n>8192-i)n=8192-i;synth_render_stereo(split_l+i,split_r+i,n);i+=n;}
  assert(!memcmp(left,split_l,sizeof left) && !memcmp(right,split_r,sizeof right));assert(!synth_faults);
  saved=*synth_test_pan();synth_render_stereo(split_l,split_r,0);assert(!memcmp(&saved,synth_test_pan(),sizeof saved));
  synth_param(142,0);synth_render_stereo(split_l,split_r,512);assert(!memcmp(&saved,synth_test_pan(),sizeof saved));assert(!memcmp(split_l,split_r,512*sizeof(float)));
  synth_param(142,1);synth_reset_controllers();assert(!memcmp(&saved,synth_test_pan(),sizeof saved));
  synth_panic();assert(!synth_test_pan()->phase && !synth_test_pan()->smoothed && synth_test_pan()->fade==1.f);
  synth_render_stereo(split_l,split_r,512);for(unsigned i=0;i<512;i++)assert(split_l[i]==0.f && split_r[i]==0.f);
 }
 /* Stereo gain must advance once per frame, and match the old mono ramp. */
 MidiLevel mono,stereo;midi_level_init(&mono);midi_level_init(&stereo);
 for(unsigned v=0;v<128;v++){
  midi_level_set(&mono,7,v);midi_level_set(&stereo,7,v);
  for(unsigned i=0;i<400;i++)plain[i]=split_l[i]=split_r[i]=(i%13-6.f)/7.f;
  midi_level_apply(&mono,plain,400);midi_level_apply_stereo(&stereo,split_l,split_r,400);
  assert(!memcmp(plain,split_l,400*4) && !memcmp(plain,split_r,400*4));assert(!memcmp(&mono,&stereo,sizeof mono));
 }
 synth_init();assert(!synth_test_pan_enabled());synth_param(138,1000);assert(synth_test_pan()->position==126/128.f);
 synth_param(139,1000);assert(synth_test_pan()->depth==2.f);synth_param(140,1000);assert(synth_test_pan()->free_rate_squared==1.f);
 synth_param(141,1000);assert(synth_test_pan()->period==48.f);synth_param(142,1000);assert(synth_test_pan_enabled());
 saved=*synth_test_pan();synth_param(SYNTH_PARAM_COUNT,0);assert(!memcmp(&saved,synth_test_pan(),sizeof saved));
 unsigned values[SYNTH_PARAM_COUNT];memcpy(values,synth_defaults,sizeof values);uint8_t snapshot[PRESET_BYTES];assert(preset_encode(values,snapshot,sizeof snapshot));assert(preset_apply(snapshot,sizeof snapshot));
 assert(!synth_test_pan_enabled() && !synth_test_pan()->position && !synth_test_pan()->depth && !synth_test_pan()->sync);
 synth_param(140,91);uint32_t free_rate=synth_test_pan()->increment;synth_param(141,10);synth_param(97,100);synth_param(100,1);synth_param(142,1);
 feed(2,0,120);assert(synth_test_pan()->increment==rate(100,10));feed(1,1000,120);assert(synth_test_pan()->increment==rate(120,10));
 synth_param(97,90);assert(synth_test_pan()->increment==rate(120,10));synth_render_stereo(split_l,split_r,256);uint32_t phase=synth_test_pan()->phase;
 synth_note(60,100);synth_clock_event(0xfa,1,1600);assert(synth_test_pan()->phase==phase);
 feed(1,1700,120);synth_clock_poll(2801);assert(synth_test_pan()->increment==rate(90,10));
 synth_param(100,2);feed(2,3000,150);assert(synth_test_pan()->increment==rate(150,10));
 synth_panic();assert(synth_test_pan()->increment==rate(150,10));synth_clock_event(0xfc,2,3600);assert(synth_test_pan()->increment==rate(90,10));
 synth_param(100,0);synth_param(141,0);assert(synth_test_pan()->increment==free_rate);
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned div=0;div<=34;div++){
  synth_param(97,bpm);synth_param(141,div);synth_render_stereo(split_l,split_r,32);for(unsigned i=0;i<32;i++)assert(isfinite(split_l[i])&&isfinite(split_r[i]));
 }
 puts("PASS: pan identity/bounds/stereo direction, exact FX chain order, nine policies, block invariance, stereo gain ramps, bypass/reset/Init, selected-clock/fallback and 7035 tempo/division cases");
}
