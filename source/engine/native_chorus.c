#include "native_chorus.h"
#include "native_eq.h" /* Shared native beat-period table. */
#include <string.h>
static void update_send(NativeChorus *s){
 float x=s->wheel_depth*s->wheel;x*=2.f;x+=s->level;
 s->send=x<0.f?0.f:x>1.f?1.f:x;
}
static void update_rate(NativeChorus *s){
 float hz=s->free_rate_squared*10.f;
 if(s->sync && s->bpm>0.f){hz=s->bpm*0x1.b4e81cp-10f;hz/=s->period;hz*=10.f;}
 s->hz=hz;s->phase_step=hz/44100.f;
 s->increment=(uint32_t)((hz*8388608.f)/44100.f);
}
void native_chorus_level(NativeChorus *s,float x){s->level=x;update_send(s);}
void native_chorus_type(NativeChorus *s,unsigned type){s->type=!!type;}
void native_chorus_rate(NativeChorus *s,float x){s->free_rate_squared=x*x;if(!s->sync)update_rate(s);}
void native_chorus_sync(NativeChorus *s,unsigned index){if(index>34)index=34;s->sync=!!index;s->period=native_eq_periods[index];update_rate(s);}
void native_chorus_tempo(NativeChorus *s,float bpm){if(bpm>0.f){s->bpm=bpm;if(s->sync)update_rate(s);}}
void native_chorus_depth(NativeChorus *s,float x){
 s->depth=(x*127.f)*0x1p-8f;
 s->centre_min=x+0x1p-7f;s->centre_max=0x1.fcp-1f-x;
 /* Preserve the original setter order, even when the two bounds cross.
    Replacing this with an unordered min/max clamp changes factory behaviour. */
 if(s->centre_min>s->centre)s->centre=s->centre_min;
 else if(s->centre>s->centre_max)s->centre=s->centre_max;
}
void native_chorus_centre(NativeChorus *s,float x){
 x=(x+.5f)*0x1.fcp-1f;
 if(s->centre_min>x)x=s->centre_min;else if(x>s->centre_max)x=s->centre_max;
 s->centre=x;
}
void native_chorus_feedback(NativeChorus *s,float x){s->feedback=((x*2.f)*127.f)*0x1p-7f;}
void native_chorus_wheel_depth(NativeChorus *s,float x){s->wheel_depth=x;update_send(s);}
void native_chorus_wheel(NativeChorus *s,float x){s->wheel=x;update_send(s);}
void native_chorus_reset(NativeChorus *s){
 /* Explicit FM1 panic/Init policy, not native per-note reset behaviour. */
 memset(s->buffer,0,sizeof s->buffer);memset(s->allpass,0,sizeof s->allpass);
 memset(s->previous,0,sizeof s->previous);s->write_index=0;
 s->phase=0;s->phase_value=0.f;s->output=0.f;s->fade=1.f;
}
void native_chorus_init(NativeChorus *s){
 memset(s,0,sizeof(*s));
 s->delay_scale=0x1.572f78p+9f; /* native 44.1 kHz constant 0x442b97bc */
 s->centre=s->centre_min=0x1p-7f;s->centre_max=0x1.fcp-1f;
 s->bpm=120.f;s->period=1.f;s->fade_step=1.f/(.01f*44100.f);
 native_chorus_reset(s);native_chorus_rate(s,0.f);native_chorus_centre(s,0.f);
}
static float interpolate(const float *buffer,float index){
 if(index<0.f)index+=4096.f;else if(index>=4096.f)index+=-4096.f;
 unsigned i=(unsigned)index;float fraction=index-(float)i;
 float output=(1.f-fraction)*buffer[i]+fraction*buffer[(i+1)&4095];
 uint32_t bits;memcpy(&bits,&output,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))output=0.f;
 return output;
}
static float phaser_channel(float *state,float feedback,float input,float coefficient){
 float output=input+feedback;
 for(unsigned i=0;i<4;i++){
  float next=state[i]-coefficient*output;
  state[i]=next*coefficient+output;output=next;
 }
 return output;
}
static inline void native_chorus_step_inline(NativeChorus *s,float input,float *left,float *right){
 input*=s->send;input+=0.f;
 float phase=s->phase_value;
 s->output=(phase>=.5f?.75f-phase:phase-.25f)*2.f;
 s->phase+=s->increment;uint32_t bits=(s->phase&0x7fffffu)+0x3f800000u;
 memcpy(&s->phase_value,&bits,4);s->phase_value-=1.f;
 s->fade+=s->fade_step;if(s->fade>1.f)s->fade=1.f;s->output*=s->fade;
 float modulation=(s->output*s->depth)*2.f;
 if(s->type){
  *left=phaser_channel(s->allpass[0],s->feedback*s->previous[0],input,modulation+.5f);
  *right=phaser_channel(s->allpass[1],s->feedback*s->previous[1],input,.5f-modulation);
  s->previous[0]=*left;s->previous[1]=*right;
 }else{
  float index=(float)s->write_index;
  float lpos=(s->centre+modulation)*s->delay_scale+index;
  float rpos=index+(s->centre-modulation)*s->delay_scale;
  *left=interpolate(s->buffer[0],lpos);*right=interpolate(s->buffer[1],rpos);
  s->buffer[0][s->write_index]=*left*s->feedback+input;
  s->buffer[1][s->write_index]=*right*s->feedback+input;
  s->write_index=(s->write_index-1)&4095;
 }
}
void native_chorus_step(NativeChorus *s,float input,float *left,float *right){native_chorus_step_inline(s,input,left,right);}
void native_chorus_process_add(NativeChorus *s,const float *input,float *left,float *right,unsigned n){
 for(unsigned i=0;i<n;i++){float l,r;native_chorus_step_inline(s,input[i],&l,&r);left[i]+=l;right[i]+=r;}
}
void native_chorus_process_add_half(NativeChorus *s,const float *input,float *left,float *right,unsigned n){
 /* One wet-state update per two output samples.  Double LFO/fade advance and
    halve the chorus delay span so modulation rate and nominal delay time stay
    close to the full-rate path.  Wet output is zero-order held for the second
    sample of each pair. */
 uint32_t inc=s->increment;float scale=s->delay_scale,fade_step=s->fade_step;
 s->increment=inc*2u;s->delay_scale=scale*.5f;s->fade_step=fade_step*2.f;
 for(unsigned i=0;i<n;i+=2){
  float x=input[i];if(i+1<n)x=(x+input[i+1])*.5f;float l,r;native_chorus_step_inline(s,x,&l,&r);
  left[i]+=l;right[i]+=r;if(i+1<n){left[i+1]+=l;right[i+1]+=r;}
 }
 s->increment=inc;s->delay_scale=scale;s->fade_step=fade_step;
}
