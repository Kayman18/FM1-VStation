#include "distortion.h"
#include "synth.h"
#include "amp_level.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const Distortion *synth_test_distortion(void);
extern unsigned synth_test_distortion_enabled(void);
static float plain[8192],wet[8192],split[8192];
static uint32_t seed=0x14ab1793u;
static unsigned random_u(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void setup(unsigned mode,unsigned cap,unsigned enable,unsigned amp){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 synth_param(128,85);synth_param(129,96);synth_param(130,80);synth_controller(0,99);
 synth_param(131,enable);synth_param(120,52);synth_param(121,amp);
 synth_note(48,100);synth_note(55,90);synth_note(60,110);
}
int main(void){
 Distortion d,e;distortion_init(&d);assert(d.amount==0 && d.wheel==0 && d.wheel_depth==0 && d.target==0 && d.smoothed==0);
 distortion_compensation(&d,1.f);assert(d.compensation==0x1.e8240cp-1f);
 for(int i=-4096;i<=4096;i++){float x=i/2048.f;assert(distortion_step(&d,x)==x);}
 for(unsigned amount=0;amount<128;amount+=7)for(unsigned comp=0;comp<128;comp+=7)for(unsigned depth=0;depth<128;depth+=7){
  distortion_init(&d);distortion_amount(&d,amount/127.f);distortion_compensation(&d,comp/127.f);distortion_depth(&d,((int)depth-64)/128.f);distortion_wheel(&d,(random_u()%128)/127.f);
  for(unsigned i=0;i<64;i++){float x=((int)(random_u()%65537)-32768)/32768.f;float y=distortion_step(&d,x);assert(isfinite(y)&&fabsf(y)<=1.000001f);assert(d.target>=0.f&&d.target<=1.f&&d.smoothed>=0.f&&d.smoothed<=1.f);}
 }
 for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  setup(mode,cap,0,0);synth_render(plain,8192);
  setup(mode,cap,1,1);synth_render(wet,8192);
  distortion_init(&d);distortion_amount(&d,85/127.f);distortion_compensation(&d,96/127.f);distortion_depth(&d,16/128.f);distortion_wheel(&d,99/127.f);
  unsigned audible=0;float gain=1.f,target=amp_level_curve(52);
  for(unsigned i=0;i<8192;i++){gain=target*.01f+gain*.99f;float want=distortion_step(&d,plain[i])*gain;assert(wet[i]==want);audible+=plain[i]!=0.f;}
  assert(audible>1000);
  setup(mode,cap,1,1);for(unsigned i=0;i<8192;){unsigned n=1+random_u()%211;if(n>8192-i)n=8192-i;synth_render(split+i,n);i+=n;}
  assert(!memcmp(wet,split,sizeof wet));assert(!synth_faults);
  /* Bypass freezes the native smoother, but controllers still update target. */
  e=*synth_test_distortion();synth_param(131,0);synth_render(split,512);assert(!memcmp(&e,synth_test_distortion(),sizeof e));
  synth_param(131,1);synth_reset_controllers();assert(synth_test_distortion()->wheel==0);assert(synth_test_distortion()->smoothed==e.smoothed);
  synth_controller(0,127);synth_panic();assert(synth_test_distortion()->wheel==0 && synth_test_distortion()->smoothed==0);
  synth_render(split,512);for(unsigned i=0;i<512;i++)assert(split[i]==0.f);
 }
 synth_init();assert(!synth_test_distortion_enabled());
 assert(synth_test_distortion()->amount==0 && synth_test_distortion()->wheel_depth==0 && synth_test_distortion()->compensation==0x1.e8240cp-1f);
 synth_param(128,1000);assert(synth_test_distortion()->amount==1.f);synth_param(130,1000);assert(synth_test_distortion()->wheel_depth==63/128.f);
 synth_param(129,1000);assert(synth_test_distortion()->compensation==0x1.e8240cp-1f);synth_param(131,1000);assert(synth_test_distortion_enabled()==1);
 synth_controller(0,1000);assert(synth_test_distortion()->wheel==1.f);
 e=*synth_test_distortion();synth_param(SYNTH_PARAM_COUNT,0);assert(!memcmp(&e,synth_test_distortion(),sizeof e));
 /* Full Init snapshot clears a live distortion setting. */
 unsigned values[SYNTH_PARAM_COUNT];memcpy(values,synth_defaults,sizeof values);uint8_t snapshot[PRESET_BYTES];assert(preset_encode(values,snapshot,sizeof snapshot));assert(preset_apply(snapshot,sizeof snapshot));assert(!synth_test_distortion_enabled());assert(synth_test_distortion()->amount==0);
 unsigned samples=0;
 for(unsigned event=0;event<12000;event++){
  unsigned id=random_u()%SYNTH_PARAM_COUNT,lo=preset_param_min(id),hi=preset_param_max(id);
  synth_param(id,lo+random_u()%(hi-lo+1));
  if(event%7==0)synth_note(24+random_u()%85,random_u()%128);
  if(event%11==0)synth_controller(0,random_u()%128);
  if(event%97==0)synth_panic();
  unsigned n=1+random_u()%128;synth_render(split,n);samples+=n;
  for(unsigned i=0;i<n;i++)assert(isfinite(split[i]));assert(!synth_faults);
 }
 printf("PASS: distortion component bounds/identity, exact mix-before-master order, nine voice policies, block invariance, bypass/reset/Init, 12000 events / %u stress samples\n",samples);
}
