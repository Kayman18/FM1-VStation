#include "mod_envelope.h"
#include <string.h>
/* Low 32 bits of ((uint64_t)a*b)>>31, using only 16x16 multiplies.
   Preserve wraparound even for out-of-range original state words. */
static uint32_t mul_shift31(uint32_t a,uint32_t b){
 uint32_t a0=a&65535u,a1=a>>16,b0=b&65535u,b1=b>>16;
 uint32_t lo=a0*b0,t=a1*b0+(lo>>16);
 uint32_t u=a0*b1+(t&65535u);
 uint32_t hi=a1*b1+(t>>16)+(u>>16);
 lo=(u<<16)|(lo&65535u);
 return (hi<<1)|(lo>>31);
}
/* RVA 0x37360; scheduler 0x37820..0x37843 selects one of eight states
   each audio sample. FM1 advances its single instance every eight samples. */
void mod_envelope_step(ModEnvelope *e){
 uint32_t *s=e->word,mode=s[0]&255u,v=s[1],target=s[13];
 if(mode==0){v+=s[9];if(v>=0x7fffffffu){v=0x7fffffffu;mode=1;}}
 else if(mode==1){if(v-target<=16u){v=target;mode=2;}
  else v=mul_shift31(s[11],v-target)+target;}
 else if(mode==2)v=target;
 else if(mode==3 || mode==4){if(v<=16u){v=0;mode=5;}
  else v=mul_shift31(s[mode==3?15:17],v);}
 s[0]=(s[0]&0xffffff00u)|mode;s[1]=v;
}
/* RVA 0x37429..0x37447: preserve x86 32-bit multiplication wrap,
   unsigned first shift, then signed division truncated toward zero. */
int32_t mod_envelope_filter(const ModEnvelope *e,int32_t depth){
 uint32_t level=((e->word[1]>>16)*e->word[3])>>15;
 uint32_t bits=level*(uint32_t)depth;int32_t product;
 memcpy(&product,&bits,sizeof(product));return product/32768;
}

extern const uint32_t mod_attack_44100[128],mod_decay_44100[128];
void mod_envelope_init(ModEnvelope *e){memset(e,0,sizeof(*e));e->word[0]=5;e->word[3]=32767;}
void mod_envelope_params(ModEnvelope *e,unsigned a,unsigned d,unsigned s,unsigned r){
 if(a>127)a=127;if(d>127)d=127;if(s>127)s=127;if(r>127)r=127;
 e->word[9]=mod_attack_44100[a];e->word[11]=mod_decay_44100[d];e->word[15]=mod_decay_44100[r];
 /* Sustain setter 0x2bef0..0x2bf24: linear normalized value, clamp Q31. */
 float v=s/127.f;v*=2147483648.f;
 e->word[13]=s==127?0x7fffffffu:(uint32_t)v;
}
void mod_envelope_gate(ModEnvelope *e,unsigned cold){
 /* 0x44082/0x440ca: active retrigger preserves level; cold gate clears it.
    FM1 last-note priority chooses when to request a gate. */
 e->word[0]&=0xffffff00u;if(cold){e->word[1]=0;e->word[2]=0;}
 e->word[18]=(e->word[18]&0xffffff00u)|1u;
}
void mod_envelope_release(ModEnvelope *e){e->word[0]=(e->word[0]&0xffffff00u)|3u;}
