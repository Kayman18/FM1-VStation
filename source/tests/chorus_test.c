#include "native_chorus.h"
#include "native_pan.h"
#include "synth.h"
#include "midi_level.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const NativeChorus *synth_test_chorus(void);
extern unsigned synth_test_chorus_enabled(void);
static float plain[8192],left[8192],right[8192],split_l[8192],split_r[8192];
static NativeChorus c,saved;
static uint32_t seed=0x17ab1793u;
static unsigned random_u(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void chorus_setup(NativeChorus *s,unsigned type){
 native_chorus_init(s);native_chorus_level(s,69/127.f);native_chorus_type(s,type);
 native_chorus_rate(s,97/127.f);native_chorus_depth(s,49/127.f);
 native_chorus_centre(s,12/128.f);native_chorus_feedback(s,-11/128.f);
 native_chorus_wheel_depth(s,17/128.f);native_chorus_wheel(s,99/127.f);
}
static void setup(unsigned mode,unsigned cap,unsigned type,unsigned enabled,unsigned pan_enabled,unsigned level){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 synth_param(128,85);synth_param(129,96);synth_param(130,80);synth_controller(0,99);synth_param(131,1);
 synth_param(120,52);synth_param(121,1);
 synth_param(132,101);synth_param(133,84);synth_param(134,63);synth_param(135,72);synth_param(137,1);
 synth_param(138,75);synth_param(139,95);synth_param(140,97);synth_param(142,pan_enabled);
 synth_param(143,69);synth_param(144,type);synth_param(145,97);synth_param(147,49);
 synth_param(148,76);synth_param(149,53);synth_param(150,81);synth_param(151,enabled);
 if(level){synth_midi_level(7,83);synth_midi_level(11,107);}
 synth_note(48,100);synth_note(55,90);synth_note(60,110);
}
static void feed(unsigned source,uint32_t start,unsigned bpm){
 for(unsigned i=0;i<=24;i++)synth_clock_event(0xf8,source,start+(60000u*i+12*bpm)/(24*bpm));
}
static uint32_t rate(unsigned bpm,unsigned div){NativeChorus p;native_chorus_init(&p);native_chorus_tempo(&p,(float)bpm);native_chorus_sync(&p,div);return p.increment;}
int main(void){
 native_chorus_init(&c);assert(!c.send && !c.increment && c.fade==1.f && c.centre==.5f*(127/128.f));
 /* Depth setters deliberately retain the original ordered clamp, even when its
    bounds cross; delay positions still remain within one wrap of the ring. */
 for(unsigned type=0;type<2;type++)for(unsigned depth=0;depth<128;depth++)for(unsigned end=0;end<2;end++){
  native_chorus_init(&c);native_chorus_type(&c,type);native_chorus_level(&c,1.f);
  native_chorus_depth(&c,depth/127.f);native_chorus_centre(&c,end?63/128.f:-.5f);
  native_chorus_feedback(&c,end?63/128.f:-.5f);native_chorus_rate(&c,1.f);
  for(unsigned i=0;i<4096;i++){float l,r;native_chorus_step(&c,i==0?.25f:0.f,&l,&r);assert(isfinite(l)&&isfinite(r));assert(fabsf(l)<16.f && fabsf(r)<16.f);assert(c.write_index<4096);}
 }
 for(unsigned type=0;type<2;type++){
  chorus_setup(&c,type);native_chorus_level(&c,0.f);native_chorus_wheel_depth(&c,0.f);
  for(unsigned i=0;i<8192;i++){float l,r;native_chorus_step(&c,.5f,&l,&r);assert(l==0.f && r==0.f);}
  native_chorus_level(&c,1.f);
  for(unsigned i=0;i<8192;i++){float l,r;native_chorus_step(&c,sinf(i*.07f)*.2f,&l,&r);assert(isfinite(l)&&isfinite(r));}
  native_chorus_level(&c,0.f);unsigned tail=0;
  for(unsigned i=0;i<512;i++){float l,r;native_chorus_step(&c,0.f,&l,&r);tail+=(l!=0.f || r!=0.f);}
  assert(tail); /* Send zero stops new input, not an existing wet tail. */
  native_chorus_reset(&c);assert(c.fade==1.f && !c.phase && !c.write_index);
  for(unsigned i=0;i<8192;i++){float l,r;native_chorus_step(&c,.5f,&l,&r);assert(l==0.f && r==0.f);}
 }
 /* Component reconstruction of the full application chain. Wet is fed from
    post-EQ mono, in parallel with pan; MIDI gain acts once after wet addition. */
 for(unsigned type=0;type<2;type++)for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  setup(mode,cap,type,0,0,0);synth_render(plain,8192);
  setup(mode,cap,type,1,1,1);synth_render_stereo(left,right,8192);
  NativePan p;native_pan_init(&p);native_pan_position(&p,11/128.f);native_pan_depth(&p,95/127.f);native_pan_rate(&p,97/127.f);
  chorus_setup(&c,type);
  for(unsigned i=0;i<8192;i++){float wl,wr;native_pan_step(&p,plain[i],split_l+i,split_r+i);native_chorus_step(&c,plain[i],&wl,&wr);split_l[i]+=wl;split_r[i]+=wr;}
  MidiLevel m;midi_level_init(&m);midi_level_set(&m,7,83);midi_level_set(&m,11,107);midi_level_apply_stereo(&m,split_l,split_r,8192);
  assert(!memcmp(left,split_l,sizeof left) && !memcmp(right,split_r,sizeof right));
  setup(mode,cap,type,1,1,1);for(unsigned i=0;i<8192;){unsigned n=1+random_u()%211;if(n>8192-i)n=8192-i;synth_render_stereo(split_l+i,split_r+i,n);i+=n;}
  assert(!memcmp(left,split_l,sizeof left) && !memcmp(right,split_r,sizeof right));assert(!synth_faults);
  saved=*synth_test_chorus();synth_render_stereo(split_l,split_r,0);assert(!memcmp(&saved,synth_test_chorus(),sizeof saved));
  synth_param(151,0);synth_render_stereo(split_l,split_r,512);assert(!memcmp(&saved,synth_test_chorus(),sizeof saved));
  synth_controller(1,61);synth_controller(2,18);assert(!memcmp(&saved,synth_test_chorus(),sizeof saved));
  synth_param(151,1);synth_reset_controllers();assert(!synth_test_chorus()->wheel && synth_test_chorus()->send==synth_test_chorus()->level);
  synth_panic();assert(!synth_test_chorus()->phase && !synth_test_chorus()->write_index && synth_test_chorus()->fade==1.f);
  synth_render_stereo(split_l,split_r,512);for(unsigned i=0;i<512;i++)assert(split_l[i]==0.f && split_r[i]==0.f);
 }
 synth_init();assert(!synth_test_chorus_enabled());synth_param(143,1000);assert(synth_test_chorus()->level==1.f);
 synth_param(144,1000);assert(synth_test_chorus()->type==1);synth_param(145,1000);assert(synth_test_chorus()->free_rate_squared==1.f);
 synth_param(146,1000);assert(synth_test_chorus()->period==48.f);synth_param(151,1000);assert(synth_test_chorus_enabled());
 saved=*synth_test_chorus();synth_param(SYNTH_PARAM_COUNT,0);assert(!memcmp(&saved,synth_test_chorus(),sizeof saved));
 synth_param(150,127);synth_controller(0,127);assert(synth_test_chorus()->send==1.f);synth_param(143,0);synth_param(150,0);assert(synth_test_chorus()->send==0.f);
 unsigned values[SYNTH_PARAM_COUNT];memcpy(values,synth_defaults,sizeof values);uint8_t snapshot[PRESET_BYTES];assert(preset_encode(values,snapshot,sizeof snapshot));assert(preset_apply(snapshot,sizeof snapshot));
 assert(!synth_test_chorus_enabled() && !synth_test_chorus()->send && !synth_test_chorus()->depth && !synth_test_chorus()->sync);
 synth_param(145,91);uint32_t free_rate=synth_test_chorus()->increment;synth_param(146,10);synth_param(97,100);synth_param(100,1);synth_param(151,1);
 feed(2,0,120);assert(synth_test_chorus()->increment==rate(100,10));feed(1,1000,120);assert(synth_test_chorus()->increment==rate(120,10));
 synth_param(97,90);assert(synth_test_chorus()->increment==rate(120,10));synth_render_stereo(split_l,split_r,256);uint32_t phase=synth_test_chorus()->phase;
 synth_note(60,100);synth_clock_event(0xfa,1,1600);assert(synth_test_chorus()->phase==phase);
 feed(1,1700,120);synth_clock_poll(2801);assert(synth_test_chorus()->increment==rate(90,10));
 synth_param(100,2);feed(2,3000,150);assert(synth_test_chorus()->increment==rate(150,10));
 synth_panic();assert(synth_test_chorus()->increment==rate(150,10));synth_clock_event(0xfc,2,3600);assert(synth_test_chorus()->increment==rate(90,10));
 synth_param(100,0);synth_param(146,0);assert(synth_test_chorus()->increment==free_rate);
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned div=0;div<=34;div++){
  synth_param(97,bpm);synth_param(146,div);synth_render_stereo(split_l,split_r,32);for(unsigned i=0;i<32;i++)assert(isfinite(split_l[i])&&isfinite(split_r[i]));
 }
 printf("PASS: chorus/phaser extremes, 2097152 stress samples, zero send/tails, exact parallel FX chain, 18 type/mode/cap cases, partition/bypass/reset/Init, selected clocks and 7035 tempo/division cases; component %zu bytes\n",sizeof c);
}
