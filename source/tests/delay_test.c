#include "native_delay.h"
#include "native_chorus.h"
#include "native_pan.h"
#include "native_eq.h"
#include "synth.h"
#include "midi_level.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const NativeDelay *synth_test_delay(void);
extern unsigned synth_test_delay_enabled(void);
#define N 8192
static float plain[N],left[N],right[N],split_l[N],split_r[N],ring[65536],other[65536];
static NativeDelay d,e,saved;static NativeChorus c;
static unsigned seed=0x18ab1739u;
static unsigned random_u(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void delay_setup(NativeDelay *s,float *buffer){
 native_delay_init(s,buffer);native_delay_level(s,65/127.f);native_delay_time(s,1/127.f);
 native_delay_feedback(s,93/127.f);native_delay_width(s,80/127.f);native_delay_ratio(s,3);
 native_delay_wheel_depth(s,-11/128.f);native_delay_wheel(s,99/127.f);
}
static void chorus_setup(NativeChorus *s,unsigned type){
 native_chorus_init(s);native_chorus_level(s,69/127.f);native_chorus_type(s,type);
 native_chorus_rate(s,97/127.f);native_chorus_depth(s,49/127.f);native_chorus_centre(s,12/128.f);
 native_chorus_feedback(s,-11/128.f);native_chorus_wheel_depth(s,17/128.f);native_chorus_wheel(s,99/127.f);
}
static void setup(unsigned mode,unsigned cap,unsigned type,unsigned effects,unsigned level){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 synth_param(128,85);synth_param(129,96);synth_param(130,80);synth_controller(0,99);synth_param(131,1);
 synth_param(120,52);synth_param(121,1);
 synth_param(132,101);synth_param(133,84);synth_param(134,63);synth_param(135,72);synth_param(137,1);
 synth_param(138,75);synth_param(139,95);synth_param(140,97);synth_param(142,effects);
 synth_param(143,69);synth_param(144,type);synth_param(145,97);synth_param(147,49);
 synth_param(148,76);synth_param(149,53);synth_param(150,81);synth_param(151,effects);
 synth_param(152,65);synth_param(153,1);synth_param(154,93);synth_param(156,80);synth_param(157,3);synth_param(158,53);synth_param(159,effects);
 if(level){synth_midi_level(7,83);synth_midi_level(11,107);}
 synth_note(48,100);synth_note(55,90);synth_note(60,110);
}
static void feed(unsigned source,uint32_t start,unsigned bpm){for(unsigned i=0;i<=24;i++)synth_clock_event(0xf8,source,start+(60000u*i+12*bpm)/(24*bpm));}
static unsigned length(unsigned bpm,unsigned div){NativeDelay p;native_delay_init(&p,other);native_delay_tempo(&p,(float)bpm);native_delay_sync(&p,div);return p.length;}
int main(void){
 /* Poisoned storage must be indistinguishable from initially cleared memory. */
 for(unsigned i=0;i<65536;i++){ring[i]=NAN;other[i]=0.f;}
 native_delay_init(&d,ring);assert(d.length==22050 && !d.valid_samples && !d.send && d.same==1.f);
 unsigned stress=0;
 for(unsigned ratio=0;ratio<13;ratio++)for(unsigned end=0;end<2;end++){
  native_delay_init(&d,ring);native_delay_level(&d,1.f);native_delay_feedback(&d,1.f);native_delay_time(&d,end?1.f:0.f);
  native_delay_ratio(&d,ratio);native_delay_width(&d,end?1.f:0.f);
  for(unsigned i=0;i<131072;i++){float l,r;native_delay_step(&d,i==0?.25f:0.f,&l,&r);assert(isfinite(l)&&isfinite(r));assert(fabsf(l)<1.f && fabsf(r)<1.f);++stress;}
  assert(d.valid_samples==65536);
 }
 /* First impulse locations, mute endpoints and stereo collapse. */
 for(unsigned ratio=0;ratio<13;ratio++){
  native_delay_init(&d,ring);native_delay_time(&d,1/127.f);native_delay_ratio(&d,ratio);native_delay_level(&d,1.f);native_delay_width(&d,1.f);
  unsigned first_l=999999,first_r=999999;
  for(unsigned i=0;i<1000;i++){float l,r;native_delay_step(&d,i==0?1.f:0.f,&l,&r);if(l && first_l==999999)first_l=i;if(r && first_r==999999)first_r=i;}
  assert(first_l==(ratio==12?999999:d.left_length));assert(first_r==(ratio==11?999999:d.right_length));
  native_delay_reset(&d);native_delay_width(&d,0.f);
  for(unsigned i=0;i<1000;i++){float l,r;native_delay_step(&d,i==0?1.f:0.f,&l,&r);assert(!memcmp(&l,&r,4));}
 }
 /* Logical reset and abrupt time/ratio edits cannot expose stale history. */
 delay_setup(&d,ring);for(unsigned i=0;i<100000;i++){float l,r;native_delay_step(&d,.3f,&l,&r);}
 native_delay_reset(&d);delay_setup(&e,other);
 for(unsigned i=0;i<200000;i++){
  if(i%997==0){float t=(random_u()%128)/127.f;unsigned ratio=random_u()%13;native_delay_time(&d,t);native_delay_time(&e,t);native_delay_ratio(&d,ratio);native_delay_ratio(&e,ratio);}
  float dl,dr,el,er,x=((int)(random_u()%10000)-5000)*.00001f;native_delay_step(&d,x,&dl,&dr);native_delay_step(&e,x,&el,&er);assert(!memcmp(&dl,&el,4) && !memcmp(&dr,&er,4));
 }
 /* Send zero permits an existing feedback tail, bypass freezes it. */
 delay_setup(&d,ring);for(unsigned i=0;i<8192;i++){float l,r;native_delay_step(&d,sinf(i*.07f)*.2f,&l,&r);}
 native_delay_level(&d,0.f);native_delay_wheel_depth(&d,0.f);unsigned tail=0;
 for(unsigned i=0;i<8192;i++){float l,r;native_delay_step(&d,0.f,&l,&r);tail+=(l!=0.f||r!=0.f);}assert(tail);
 native_delay_reset(&d);for(unsigned i=0;i<131072;i++){float l,r;native_delay_step(&d,.5f,&l,&r);assert(l==0.f&&r==0.f);}
 /* Full parallel routing: EQ -> dry pan + delay; then chorus; MIDI once. */
 for(unsigned type=0;type<2;type++)for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  setup(mode,cap,type,0,0);synth_render(plain,N);setup(mode,cap,type,1,1);synth_render_stereo(left,right,N);
  NativePan p;native_pan_init(&p);native_pan_position(&p,11/128.f);native_pan_depth(&p,95/127.f);native_pan_rate(&p,97/127.f);
  delay_setup(&d,ring);chorus_setup(&c,type);
  for(unsigned i=0;i<N;i++){float dl,dr,cl,cr;native_pan_step(&p,plain[i],split_l+i,split_r+i);native_delay_step(&d,plain[i],&dl,&dr);split_l[i]+=dl;split_r[i]+=dr;native_chorus_step(&c,plain[i],&cl,&cr);split_l[i]=cl+split_l[i];split_r[i]=cr+split_r[i];}
  MidiLevel m;midi_level_init(&m);midi_level_set(&m,7,83);midi_level_set(&m,11,107);midi_level_apply_stereo(&m,split_l,split_r,N);
  assert(!memcmp(left,split_l,sizeof left) && !memcmp(right,split_r,sizeof right));
  setup(mode,cap,type,1,1);for(unsigned i=0;i<N;){unsigned n=1+random_u()%211;if(n>N-i)n=N-i;synth_render_stereo(split_l+i,split_r+i,n);i+=n;}
  assert(!memcmp(left,split_l,sizeof left) && !memcmp(right,split_r,sizeof right));assert(!synth_faults);
  saved=*synth_test_delay();synth_render_stereo(split_l,split_r,0);assert(!memcmp(&saved,synth_test_delay(),sizeof saved));
  synth_param(159,0);synth_render_stereo(split_l,split_r,512);assert(!memcmp(&saved,synth_test_delay(),sizeof saved));
  synth_controller(1,61);synth_controller(2,18);assert(!memcmp(&saved,synth_test_delay(),sizeof saved));
  synth_param(159,1);synth_reset_controllers();assert(!synth_test_delay()->wheel && synth_test_delay()->send==synth_test_delay()->level);
  synth_panic();assert(!synth_test_delay()->valid_samples && !synth_test_delay()->write_index && !synth_test_delay()->low);
  synth_render_stereo(split_l,split_r,512);for(unsigned i=0;i<512;i++)assert(split_l[i]==0.f && split_r[i]==0.f);
 }
 synth_init();assert(!synth_test_delay_enabled());synth_param(152,1000);assert(synth_test_delay()->level==1.f);
 synth_param(153,1000);assert(synth_test_delay()->free_seconds==127.f*350.f/65536.f);
 synth_param(154,1000);assert(synth_test_delay()->feedback==127/128.f);synth_param(155,1000);assert(synth_test_delay()->period==8.f);
 synth_param(156,1000);assert(!synth_test_delay()->cross);synth_param(157,1000);assert(synth_test_delay()->ratio==12);synth_param(159,1000);assert(synth_test_delay_enabled());
 saved=*synth_test_delay();synth_param(SYNTH_PARAM_COUNT,0);assert(!memcmp(&saved,synth_test_delay(),sizeof saved));
 synth_param(158,127);synth_controller(0,127);assert(synth_test_delay()->send==1.f);synth_param(152,0);synth_param(158,0);assert(!synth_test_delay()->send);
 unsigned values[SYNTH_PARAM_COUNT];memcpy(values,synth_defaults,sizeof values);uint8_t snapshot[PRESET_BYTES];assert(preset_encode(values,snapshot,sizeof snapshot));assert(preset_apply(snapshot,sizeof snapshot));
 assert(!synth_test_delay_enabled() && !synth_test_delay()->send && !synth_test_delay()->sync && !synth_test_delay()->valid_samples);
 synth_param(153,91);unsigned free_length=synth_test_delay()->length;synth_param(155,10);synth_param(97,100);synth_param(100,1);synth_param(159,1);
 feed(2,0,120);assert(synth_test_delay()->length==length(100,10));feed(1,1000,120);assert(synth_test_delay()->length==length(120,10));
 synth_param(97,90);assert(synth_test_delay()->length==length(120,10));synth_render_stereo(split_l,split_r,256);unsigned write=synth_test_delay()->write_index;
 synth_note(60,100);synth_clock_event(0xfa,1,1600);assert(synth_test_delay()->write_index==write);
 feed(1,1700,120);synth_clock_poll(2801);assert(synth_test_delay()->length==length(90,10));
 synth_param(100,2);feed(2,3000,150);assert(synth_test_delay()->length==length(150,10));
 synth_panic();assert(synth_test_delay()->length==length(150,10));synth_clock_event(0xfc,2,3600);assert(synth_test_delay()->length==length(90,10));
 synth_param(100,0);synth_param(155,0);assert(synth_test_delay()->length==free_length);
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned div=0;div<=19;div++){
  synth_param(97,bpm);synth_param(155,div);assert(synth_test_delay()->length>=4 && synth_test_delay()->length<=65535);
  synth_render_stereo(split_l,split_r,32);for(unsigned i=0;i<32;i++)assert(isfinite(split_l[i])&&isfinite(split_r[i]));
 }
 printf("PASS: delay %u extreme stress samples, all 13 impulse tap ratios, logical resets/time changes, send/tails, exact FX chain, 18 type/mode/cap cases, partition/bypass/reset/Init, clocks and 4020 tempo/division cases; state %zu bytes plus 262144 ring bytes\n",stress,sizeof d);
}
