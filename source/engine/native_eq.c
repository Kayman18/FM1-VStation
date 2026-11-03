#include "native_eq.h"
#include <string.h>
void lfo_step(uint32_t *state);
static void word_float(uint32_t *out,float value){memcpy(out,&value,4);}
static void set_hz(NativeEq *s,float hz){
 word_float(&s->lfo[8],hz);
 s->lfo[4]=(uint32_t)((hz*4294967296.f)/44100.f);
}
static void update_rate(NativeEq *s){
 float hz=s->free_rate_squared*10.f;
 if(s->sync && s->bpm>0.f){hz=s->bpm*0x1.b4e81cp-10f;hz/=s->period;hz*=10.f;}
 set_hz(s,hz);
}
void native_eq_depth(NativeEq *s,float x){s->depth=x*2.f;s->direct_gain=s->depth+1.f;s->low_gain=s->depth*2.f;}
void native_eq_frequency(NativeEq *s,unsigned index){
 if(index>127)index=127;s->frequency=(int32_t)index;
 s->feedback=native_eq_table_44100[index];s->feed=1.f-s->feedback;
}
void native_eq_modulation(NativeEq *s,float x){
 x*=2.f;x*=10.f;x*=128.f;x*=.0625f;
 s->modulation_extent=x;s->modulation_scale=(int32_t)x;
}
void native_eq_rate(NativeEq *s,float x){s->free_rate_squared=x*x;if(!s->sync)update_rate(s);}
void native_eq_sync(NativeEq *s,unsigned index){
 if(index>34)index=34;s->sync=!!index;s->period=native_eq_periods[index];update_rate(s);
}
void native_eq_tempo(NativeEq *s,float bpm){if(bpm>0.f){s->bpm=bpm;if(s->sync)update_rate(s);}}
void native_eq_reset(NativeEq *s){
 /* FM1 panic policy: clear filter history and restart the free-running
    triangle/fade. This is not the native initial-position/note-gate mapping. */
 s->low=0.f;s->lfo[1]=s->lfo[2]=s->lfo[3]=s->lfo[6]=0;
}
void native_eq_init(NativeEq *s){
 memset(s,0,sizeof(*s));
 word_float(&s->lfo[7],44100.f);word_float(&s->lfo[8],1.f);
 s->lfo[5]=(uint32_t)((1.f/(.01f*44100.f))*2147483648.f);
 s->bpm=120.f;s->period=1.f;
 native_eq_depth(s,0.f);native_eq_frequency(s,78);
 native_eq_modulation(s,0.f);native_eq_rate(s,0.f);
}
static inline float native_eq_step_inline(NativeEq *s,float input){
 /* Original audio-rate EQ LFO, NOT the voice LFO's /16 control cadence. */
 lfo_step(s->lfo);
 int32_t lfo;memcpy(&lfo,&s->lfo[1],4);
 int32_t index=s->frequency+(lfo*s->modulation_scale)/32768;
 if(index<0)index=0;else if(index>127)index=127;
 s->feedback=native_eq_table_44100[index];s->feed=1.f-s->feedback;
 float low=s->low*s->feedback;
 low+=s->feed*input;s->low=low;
 return input*s->direct_gain-low*s->low_gain;
}
float native_eq_step(NativeEq *s,float input){return native_eq_step_inline(s,input);}
void native_eq_process(NativeEq *s,float *samples,unsigned n){for(unsigned i=0;i<n;i++)samples[i]=native_eq_step_inline(s,samples[i]);}
