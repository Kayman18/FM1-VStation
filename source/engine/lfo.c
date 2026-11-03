#include <stdint.h>
#include <string.h>
static int32_t signed_word(uint32_t x){int32_t y;memcpy(&y,&x,4);return y;}
static int32_t floor_shift(int32_t x,unsigned n){return x>=0?x/(1<<n):-1-(int32_t)((~(uint32_t)x)>>n);}
/* RVA 0x37520, all four waveform modes. Original word offsets retained.
   Mode 3 RNG state is word 12; original embedded vtable is unnecessary here. */
void lfo_step(uint32_t *s){
 unsigned mode=s[0]&255;int32_t out=signed_word(s[1]);
 if(mode==0){uint32_t a=s[3];if(a&0x80000000u)a=0u-a;out=floor_shift(signed_word(a-0x40000000u),16);}
 else if(mode==1){int32_t hi=(int32_t)(s[3]>>16);if(hi&32768)hi-=65536;out=floor_shift(hi*32768,16);}
 else if(mode==2)out=(int32_t)((s[3]>>16)&0x8000u);
 else if(mode==3 && (uint32_t)(s[3]-s[2])>0x80000000u){
  s[2]=s[3];s[12]=s[12]*1664525u+1013904223u;out=signed_word(s[12]);
 }
 s[3]+=s[4];s[6]+=s[5];if(s[6]>0x7fffffffu)s[6]=0x7fffffffu;
 /* Mode 3 requires wrapped multiplication, not signed C overflow. */
 out=signed_word((uint32_t)out*(s[6]>>16))/32768;s[1]=(uint32_t)out;
}
#ifndef VSTATION_TARGET
#include <math.h>
typedef void (__attribute__((ms_abi))*Step)(void*);
typedef void (__attribute__((ms_abi))*Setter)(void*,unsigned char,unsigned char);
void reference_lfo(void *fn,void *s){((Step)fn)(s);}
void reference_lfo_param(void *fn,void *s,unsigned char lfo,unsigned char v){((Setter)fn)(s,lfo,v);}
uint32_t lfo_rate(unsigned v,float sr){float hz=powf(2.f,((float)v-127.f)/12.f);hz*=125.f;if(v<31)hz*=v*.03125f;float x=hz*4294967296.f;x/=sr;return (uint32_t)(uint64_t)x;}
uint32_t lfo_fade(unsigned v,float sr){float x=powf(1000.f,v/127.f);x*=.009999999776482582f;x*=sr;x=1.f/x;x*=2147483648.f;return (uint32_t)(uint64_t)x;}
#endif
