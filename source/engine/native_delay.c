#include "native_delay.h"
#include "native_eq.h"
#include <string.h>
static void update_send(NativeDelay *s){
 float x=s->wheel_depth*s->wheel;x*=2.f;x+=s->level;
 s->send=x<0.f?0.f:x>1.f?1.f:x;
}
void native_delay_ratio(NativeDelay *s,unsigned index){
 static const float factors[13][2]={
  {1.f,1.f},{1.f,.75f},{.75f,1.f},{1.f,0x1.555556p-1f},
  {0x1.555556p-1f,1.f},{1.f,.5f},{.5f,1.f},
  {1.f,0x1.555556p-2f},{0x1.555556p-2f,1.f},
  {1.f,.25f},{.25f,1.f},{1.f,1.f},{1.f,1.f}};
 if(index>12)index=12;s->ratio=index;
 s->left_gain=index==12?0.f:1.f;s->right_gain=index==11?0.f:1.f;
 s->left_length=(uint16_t)((float)s->length*factors[index][0]);
 s->right_length=(uint16_t)((float)s->length*factors[index][1]);
 s->left_index=(uint16_t)(s->write_index-s->left_length);
 s->right_index=(uint16_t)(s->write_index-s->right_length);
}
static void update_taps(NativeDelay *s){
 float samples=s->seconds*44100.f;
 if(samples<4.f)samples=4.f;else if(samples>65535.f)samples=65535.f;
 s->length=(uint16_t)samples;
 s->feedback_index=(uint16_t)(s->write_index-s->length);
 native_delay_ratio(s,s->ratio);
}
void native_delay_level(NativeDelay *s,float x){s->level=x;update_send(s);}
void native_delay_wheel_depth(NativeDelay *s,float x){s->wheel_depth=x;update_send(s);}
void native_delay_wheel(NativeDelay *s,float x){s->wheel=x;update_send(s);}
void native_delay_time(NativeDelay *s,float x){
 s->free_seconds=((x*127.f)*350.f)*0x1p-16f;
 if(!s->sync){s->seconds=s->free_seconds;update_taps(s);}
}
void native_delay_feedback(NativeDelay *s,float x){s->feedback=(x*127.f)*0x1p-7f;}
void native_delay_width(NativeDelay *s,float x){s->cross=(1.f-x)*.5f;s->same=1.f-s->cross;}
void native_delay_tempo(NativeDelay *s,float bpm){
 if(bpm>0.f){s->bpm=bpm;if(s->sync){s->seconds=(60.f/bpm)*s->period;update_taps(s);}}
}
void native_delay_sync(NativeDelay *s,unsigned index){
 if(index>19)index=19;s->sync=!!index;
 if(!index){s->seconds=s->free_seconds;update_taps(s);}
 s->period=native_eq_periods[index];native_delay_tempo(s,s->bpm);
}
void native_delay_reset(NativeDelay *s){
 /* Explicit FM1 panic/Init policy, not original patch-change behaviour. */
 s->valid_samples=0; /* lazy zero: never read history preceding this reset */
 s->write_index=0;s->low=0.f;s->smoothed=0.f;update_taps(s);
}
void native_delay_init(NativeDelay *s,float buffer[NATIVE_DELAY_SAMPLES]){
 memset(s,0,sizeof(*s));s->buffer=buffer;s->bpm=120.f;s->period=1.f;
 s->seconds=s->free_seconds=.5f;s->same=1.f;native_delay_reset(s);
}
static inline void native_delay_step_inline(NativeDelay *s,float input,float *left,float *right){
 float smooth=s->send*.125f+s->smoothed*.875f;
 uint32_t bits;memcpy(&bits,&smooth,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))smooth=0.f;
 s->smoothed=smooth;
 float previous=s->low*0x1.0f87bp-3f;
 float tap=s->valid_samples>=s->length?s->buffer[s->feedback_index]:0.f;
 s->feedback_index++;s->low=tap*0x1.bc1e14p-1f+previous;
 float send=input*smooth;send+=0.f;
 s->buffer[s->write_index++]=send+s->feedback*s->low;
 float l=(s->valid_samples>=s->left_length?s->buffer[s->left_index]:0.f)*s->left_gain;
 float r=(s->valid_samples>=s->right_length?s->buffer[s->right_index]:0.f)*s->right_gain;
 s->left_index++;s->right_index++;
 if(s->valid_samples<NATIVE_DELAY_SAMPLES)s->valid_samples++;
 *left=s->same*l+s->cross*r;*right=s->same*r+s->cross*l;
}
void native_delay_step(NativeDelay *s,float input,float *left,float *right){native_delay_step_inline(s,input,left,right);}
void native_delay_process_add(NativeDelay *s,const float *input,float *left,float *right,unsigned n){
 for(unsigned i=0;i<n;i++){float l,r;native_delay_step_inline(s,input[i],&l,&r);left[i]+=l;right[i]+=r;}
}
