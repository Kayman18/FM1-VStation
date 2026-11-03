#include "portamento.h"
#include <string.h>
extern const uint32_t portamento_samples_44100[128];
extern const int32_t portamento_exp_44100[128][128];
unsigned portamento_time_ms(unsigned time){
 if(time>127)time=127;
 return (portamento_samples_44100[time]*1000u+22050u)/44100u;
}
void portamento_configure(Portamento *p,unsigned linear,unsigned time){
 if(time>127)time=127;p->linear=linear?1:0;p->target=0;
 if(!time){p->offset=0;p->rate=0;return;}
 if(p->linear){
  p->rate=-(p->offset/(int32_t)portamento_samples_44100[time]);
  if(p->offset && !p->rate)p->rate=p->offset>0?-1:1;
 }else{
  uint32_t bits;memcpy(&bits,&p->offset,4);
  uint32_t magnitude=(bits&0x80000000u)?0u-bits:bits;
  /* Original rates index whole note intervals. For an interrupted glide,
     retain current pitch and round its residual interval upward. */
  unsigned interval=(magnitude+8388607u)/8388608u;
  if(interval>127)interval=127;
  p->rate=portamento_exp_44100[time][interval];
 }
}
/* Low 32 bits of unsigned product >>31, avoiding a target 64-bit multiply. */
static uint32_t product31(uint32_t a,uint32_t b){
 uint32_t a0=a&65535u,a1=a>>16,b0=b&65535u,b1=b>>16;
 uint32_t lo=a0*b0,t=a1*b0+(lo>>16),u=a0*b1+(t&65535u);
 uint32_t hi=a1*b1+(t>>16)+(u>>16);lo=(u<<16)|(lo&65535u);
 return (hi<<1)|(lo>>31);
}
/* RVA 0x379f9..0x37a59. Offset units are 1/65536 of a pitch-table step.
   Preserve signed multiply/shift and 32-bit wrap, including original edges. */
void portamento_step(Portamento *p){
 uint32_t a,b,r;memcpy(&a,&p->offset,4);memcpy(&b,&p->rate,4);
 if(!p->linear){
  r=product31(a,b);if(a&0x80000000u)r-=b<<1;if(b&0x80000000u)r-=a<<1;
  uint32_t magnitude=(r&0x80000000u)?0u-r:r;
  if(magnitude<2048u || magnitude==0x80000000u)r=0;
  memcpy(&p->offset,&r,4);
 }else{
  r=a+b;memcpy(&p->offset,&r,4);
  if(p->rate<0){if(p->offset<p->target)p->offset=p->target;}
  else if(p->offset>p->target)p->offset=p->target;
 }
}
