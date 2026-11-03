#include "native_reverb.h"
#include "native_delay.h"
#include "native_chorus.h"
#include "native_pan.h"
#include "synth.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const NativeReverb *synth_test_reverb(void);
extern unsigned synth_test_reverb_enabled(void);
#define N 8192
static float plain[N],left[N],right[N],split_l[N],split_r[N];
static float delay_ring[65536];
static float rv_a[28535],rv_b[28535];
static NativeReverb rv,rv2,saved;static NativeDelay d;static NativeChorus c;static NativePan p;
static unsigned seed=0x22a5beefu;
static unsigned random_u(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void history_ptrs(float *storage,float *out[NATIVE_REVERB_LINES]){
 unsigned offset=0;for(unsigned i=0;i<NATIVE_REVERB_LINES;i++){out[i]=storage+offset;offset+=native_reverb_capacities[i];}assert(offset==28535);
}
static void rv_setup(NativeReverb *s,float *storage,unsigned type){
 float *h[NATIVE_REVERB_LINES];history_ptrs(storage,h);native_reverb_init(s,h);
 native_reverb_type(s,type);native_reverb_decay_index(s,87);native_reverb_level(s,67/127.f);
 native_reverb_wheel_depth(s,17/128.f);native_reverb_wheel(s,99/127.f);
}
static void delay_setup(NativeDelay *s){native_delay_init(s,delay_ring);native_delay_level(s,65/127.f);native_delay_time(s,1/127.f);native_delay_feedback(s,93/127.f);native_delay_width(s,80/127.f);native_delay_ratio(s,3);native_delay_wheel_depth(s,-11/128.f);native_delay_wheel(s,99/127.f);}
static void chorus_setup(NativeChorus *s,unsigned type){native_chorus_init(s);native_chorus_level(s,69/127.f);native_chorus_type(s,type);native_chorus_rate(s,97/127.f);native_chorus_depth(s,49/127.f);native_chorus_centre(s,12/128.f);native_chorus_feedback(s,-11/128.f);native_chorus_wheel_depth(s,17/128.f);native_chorus_wheel(s,99/127.f);}
static void pan_setup(NativePan *s){native_pan_init(s);native_pan_position(s,11/128.f);native_pan_depth(s,95/127.f);native_pan_rate(s,97/127.f);}
static void fixture(unsigned mode,unsigned cap,unsigned chorus_type,unsigned effects,unsigned room){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 synth_param(138,75);synth_param(139,95);synth_param(140,97);synth_param(142,effects);
 synth_param(143,69);synth_param(144,chorus_type);synth_param(145,97);synth_param(147,49);synth_param(148,76);synth_param(149,53);synth_param(150,81);synth_param(151,effects);
 synth_param(152,65);synth_param(153,1);synth_param(154,93);synth_param(156,80);synth_param(157,3);synth_param(158,53);synth_param(159,effects);
 synth_param(163,67);synth_param(164,room);synth_param(165,87);synth_param(166,81);synth_param(167,effects);synth_controller(0,99);
 synth_note(48,100);synth_note(55,90);synth_note(60,110);
}
int main(void){
 /* Stale backing storage is unreadable until each compact line has valid history. */
 for(unsigned i=0;i<28535;i++){rv_a[i]=NAN;rv_b[i]=0.f;}float *ha[NATIVE_REVERB_LINES],*hb[NATIVE_REVERB_LINES];history_ptrs(rv_a,ha);history_ptrs(rv_b,hb);
 native_reverb_init(&rv,ha);native_reverb_init(&rv2,hb);native_reverb_level(&rv,1.f);native_reverb_level(&rv2,1.f);
 for(unsigned i=0;i<20000;i++){float al,ar,bl,br,x=i==0?.25f:0.f;native_reverb_step(&rv,x,&al,&ar);native_reverb_step(&rv2,x,&bl,&br);assert(!memcmp(&al,&bl,4)&&!memcmp(&ar,&br,4));}
 /* Reset is constant-time and changing rooms afterwards cannot expose old samples. */
 native_reverb_reset(&rv);native_reverb_reset(&rv2);for(unsigned t=0;t<6;t++){native_reverb_type(&rv,t);native_reverb_type(&rv2,t);native_reverb_decay_index(&rv,127);native_reverb_decay_index(&rv2,127);for(unsigned i=0;i<12000;i++){float al,ar,bl,br,x=(i%997==0)?.2f:0.f;native_reverb_step(&rv,x,&al,&ar);native_reverb_step(&rv2,x,&bl,&br);assert(!memcmp(&al,&bl,4)&&!memcmp(&ar,&br,4));assert(isfinite(al)&&isfinite(ar));}}
 /* Send zero stops new input but an existing native tail continues. */
 rv_setup(&rv,rv_a,5);for(unsigned i=0;i<16000;i++){float l,r;native_reverb_step(&rv,i==0?.5f:0.f,&l,&r);}native_reverb_level(&rv,0.f);native_reverb_wheel_depth(&rv,0.f);unsigned tail=0;for(unsigned i=0;i<24000;i++){float l,r;native_reverb_step(&rv,0.f,&l,&r);tail+=(l!=0.f||r!=0.f);}assert(tail);
 /* Full routing: post-EQ/pre-pan mono feeds dry pan, delay, chorus and reverb in parallel. */
 for(unsigned room=0;room<6;room++)for(unsigned type=0;type<2;type++)for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  fixture(mode,cap,type,0,room);synth_render(plain,N);
  fixture(mode,cap,type,1,room);synth_render_stereo(left,right,N);
  pan_setup(&p);delay_setup(&d);chorus_setup(&c,type);memset(rv_b,0,sizeof rv_b);rv_setup(&rv2,rv_b,room);
  for(unsigned i=0;i<N;i++){float dl,dr,cl,cr,rl,rr;native_pan_step(&p,plain[i],split_l+i,split_r+i);native_delay_step(&d,plain[i],&dl,&dr);split_l[i]+=dl;split_r[i]+=dr;native_chorus_step(&c,plain[i],&cl,&cr);split_l[i]=cl+split_l[i];split_r[i]=cr+split_r[i];native_reverb_step(&rv2,plain[i],&rl,&rr);split_l[i]+=rl;split_r[i]+=rr;}
  assert(!memcmp(left,split_l,sizeof left)&&!memcmp(right,split_r,sizeof right));
  fixture(mode,cap,type,1,room);for(unsigned i=0;i<N;){unsigned n=1+random_u()%211;if(n>N-i)n=N-i;synth_render_stereo(split_l+i,split_r+i,n);i+=n;}assert(!memcmp(left,split_l,sizeof left)&&!memcmp(right,split_r,sizeof right));assert(!synth_faults);
 }
 synth_init();assert(!synth_test_reverb_enabled());assert(!synth_test_reverb()->send);synth_param(163,1000);assert(synth_test_reverb()->level==1.f);synth_param(164,1000);assert(synth_test_reverb()->type==5);synth_param(165,1000);assert(synth_test_reverb()->decay>0.f);synth_param(166,127);synth_controller(0,127);assert(synth_test_reverb()->send==1.f);synth_param(167,1000);assert(synth_test_reverb_enabled());
 synth_note(60,100);synth_render_stereo(left,right,1024);saved=*synth_test_reverb();synth_param(167,0);synth_render_stereo(left,right,1024);assert(!memcmp(&saved,synth_test_reverb(),sizeof saved));
 synth_param(167,1);synth_reset_controllers();assert(!synth_test_reverb()->wheel);synth_panic();for(unsigned i=0;i<NATIVE_REVERB_LINES;i++)assert(!synth_test_reverb()->write[i]&&!synth_test_reverb()->valid[i]);assert(!synth_test_reverb()->feedback_state[0]&&!synth_test_reverb()->feedback_state[1]);
 unsigned values[SYNTH_PARAM_COUNT];memcpy(values,synth_defaults,sizeof values);uint8_t snapshot[PRESET_BYTES];assert(preset_encode(values,snapshot,sizeof snapshot));synth_param(167,1);assert(preset_apply(snapshot,sizeof snapshot));assert(!synth_test_reverb_enabled()&&!synth_test_reverb()->send);
 printf("PASS: exact compact reverb histories, six room types, logical reset/type changes, tails, 108 FX-chain mode/type/room cases, partition invariance, wheel/bypass/panic/Init; %u history bytes\n",28535u*4u);
 return 0;
}
