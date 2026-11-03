#include "synth.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
unsigned synth_test_owner(unsigned),synth_test_pending(unsigned);
static float a[8192],b[8192],c[8192],buf[256];
static void render(float *p,unsigned n,unsigned block){while(n){unsigned k=n<block?n:block;synth_render(p,k);p+=k;n-=k;}}
static void scenario(float *out,unsigned block){
 synth_init();synth_param(95,2);synth_param(84,1);synth_param(86,80);synth_param(75,75);synth_param(66,2);synth_param(68,40);
 synth_note(60,100);synth_note(67,100);render(out,1024,block);
 synth_sustain(1);synth_note(60,0);synth_controller(0,90);synth_note(72,110);render(out+1024,1024,block);
 synth_note(74,120);render(out+2048,17,block);synth_note(76,110);synth_note(74,0);render(out+2065,1007,block);
 synth_note(76,0);synth_reset_controllers();synth_note(67,0);synth_note(72,0);render(out+3072,5120,block);
}
int main(void){
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(60,100);render(a,8192,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(67,100);render(b,8192,256);
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);assert(synth_voice_count()==2);render(c,8192,73);
 for(unsigned k=0;k<8192;k++){float gain=k<64?k/64.f:1.f;float expected=(a[k]*gain+b[k]*gain)*.5f;assert(c[k]==expected);}
 scenario(a,256);scenario(b,73);assert(!memcmp(a,b,sizeof(a)));
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);synth_render(buf,256);
 synth_note(72,100);assert(synth_test_owner(0)==72 && synth_test_pending(0)==72);
 synth_note(60,0);synth_render(buf,64);assert(synth_test_owner(0)==72 && synth_test_pending(0)==128);assert(synth_voice_count()==2);
 /* Released voice preferred over a held voice. */
 synth_note(67,0);synth_note(74,100);assert(synth_test_owner(1)==74);
 /* Note released during steal must never appear later. */
 synth_note(74,0);synth_render(buf,128);assert(synth_test_owner(1)==128);
 synth_note(76,100);assert(synth_test_owner(1)==76);synth_note(76,100);assert(synth_test_owner(0)==72);
 /* Pedal retains pending note, then releases it after its delayed gate. */
 synth_sustain(1);synth_note(79,100);synth_note(79,0);synth_render(buf,64);assert(synth_test_owner(0)==79);
 synth_sustain(0);synth_note(76,0);for(unsigned n=0;n<10000;n++)synth_render(buf,256);assert(synth_voice_count()==0);
 synth_init();synth_param(95,2);for(unsigned ch=0;ch<16;ch++)assert(synth_accept_channel(ch));
 for(unsigned channel=1;channel<=16;channel++){synth_param(96,channel);for(unsigned ch=0;ch<16;ch++)assert(synth_accept_channel(ch)==(ch+1==channel));}
 synth_note(60,100);synth_param(96,0);assert(synth_voice_count()==0);
 synth_note(60,100);synth_note(67,100);synth_param(95,1);assert(synth_voice_count()==0);synth_note(60,100);assert(synth_voice_count()==1);
 synth_param(95,2);assert(synth_voice_count()==0);
 unsigned seed=8;for(unsigned event=0;event<20000;event++){
  seed=seed*1664525u+1013904223u;unsigned note=24+(seed>>24)%72;
  if(event%9==0)synth_sustain((event/9)&1);else if(event%13==0)synth_reset_controllers();else synth_note(note,(seed&1)?100:0);
  synth_render(buf,1+(seed%256));assert(synth_voice_count()<=2 && synth_faults==0);
  for(unsigned k=0;k<1+(seed%256);k++)assert(isfinite(buf[k]) && fabsf(buf[k])<=1.f);
 }
 synth_panic();synth_render(buf,256);for(unsigned k=0;k<256;k++)assert(buf[k]==0.f);
 puts("PASS: independent two-voice sum; block invariance; steal/cancel/repeat/pedal/reset; channel filter; 20000 stress events; zero faults");
}
