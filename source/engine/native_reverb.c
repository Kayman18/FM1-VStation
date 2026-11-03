#include "native_reverb.h"
#include <math.h>
#include <string.h>
const uint16_t native_reverb_lengths[6][NATIVE_REVERB_LINES]={
 {1464,1100,826,621,465,350,585,1025,1076,732,878,1054},
 {1464,1126,865,665,551,392,732,1538,1040,1025,1098,1171},
 {1757,1321,993,745,559,420,585,1523,2343,1464,644,2332},
 {1757,1352,1038,798,613,471,732,1904,2929,1831,805,2915},
 {1611,1211,909,682,512,385,1025,2666,4101,2563,1127,4081},
 {1904,1431,1075,807,606,455,1464,3808,5859,3662,1611,5830}
};
const uint16_t native_reverb_capacities[NATIVE_REVERB_LINES]={1904,1431,1075,807,613,471,1464,3808,5859,3662,1611,5830};
static const float decay_scale[6]={.4300000071525574f,.4490000009536743f,.5080000162124634f,.546999990940094f,.6050000190734863f,.6639999747276306f};
static const float decay_offset[6]={.23399999737739563f,.25f,.25f,.25f,.25f,.25f};
#include "native_reverb_decay_table.inc"
static void update_send(NativeReverb *s){
 float x=s->wheel*s->wheel_depth;x*=2.f;x+=s->level;
 s->send=x<0.f?0.f:x>1.f?1.f:x;
}
void native_reverb_level(NativeReverb *s,float x){s->level=x;update_send(s);}
void native_reverb_wheel_depth(NativeReverb *s,float x){s->wheel_depth=x;update_send(s);}
void native_reverb_wheel(NativeReverb *s,float x){s->wheel=x;update_send(s);}
void native_reverb_type(NativeReverb *s,unsigned type){
 if(type>5)type=5;s->type=(uint8_t)type;
 for(unsigned i=0;i<NATIVE_REVERB_LINES;i++){
  unsigned cap=native_reverb_capacities[i],w=s->write[i],len=native_reverb_lengths[type][i];
  s->length[i]=(uint16_t)len;s->read[i]=(uint16_t)(w>=len?w-len:w+cap-len);
 }
}
void native_reverb_decay(NativeReverb *s,float x){
 x*=127.f;x*=0x1p-7f;x*=decay_scale[s->type];x+=decay_offset[s->type];s->decay=x;
 float coefficient=sqrtf(x);coefficient+=1.f;coefficient*=.35f;
 for(unsigned i=0;i<10;i++)s->coefficient[i]=coefficient;
}
void native_reverb_decay_index(NativeReverb *s,unsigned value){
 if(value>127)value=127;s->decay=native_reverb_decay_grid[s->type][value];
 float coefficient=native_reverb_coefficient_grid[s->type][value];
 for(unsigned i=0;i<10;i++)s->coefficient[i]=coefficient;
}
void native_reverb_reset(NativeReverb *s){
 for(unsigned i=0;i<NATIVE_REVERB_LINES;i++){
  s->write[i]=0;s->valid[i]=0;s->read[i]=(uint16_t)(native_reverb_capacities[i]-s->length[i]);
 }
 s->warm_samples=0;s->feedback_state[0]=s->feedback_state[1]=0.f;
}
void native_reverb_init(NativeReverb *s,float *const history[NATIVE_REVERB_LINES]){
 memset(s,0,sizeof(*s));for(unsigned i=0;i<NATIVE_REVERB_LINES;i++)s->history[i]=history[i];
 /* Exact 10 kHz / 44.1 kHz one-pole coefficients produced by native ctor. */
 s->low_a[0]=s->low_a[1]=0x1.2bbe9ap-2f;
 s->low_b[0]=s->low_b[1]=0x1.6a20b4p-1f;
 native_reverb_type(s,0);
 /* Constructor default is exactly decay=.5 through the recovered float path.
    Spell out its resulting state so the target does not require libm sqrtf. */
 s->decay=0x1.ca0e56p-2f;
 for(unsigned i=0;i<10;i++)s->coefficient[i]=0x1.2b0d74p-1f;
 native_reverb_reset(s);
}
static inline float read_line_slow(const NativeReverb *s,unsigned line){
 if(s->valid[line]<s->length[line])return 0.f;
 return s->history[line][s->read[line]];
}
static inline __attribute__((always_inline)) void advance_heads(NativeReverb *s,unsigned line){
 unsigned cap=native_reverb_capacities[line];
 unsigned w=s->write[line]+1u;if(w==cap)w=0;s->write[line]=(uint16_t)w;
 unsigned r=s->read[line]+1u;if(r==cap)r=0;s->read[line]=(uint16_t)r;
}
static inline void write_line_slow(NativeReverb *s,unsigned line,float x){
 s->history[line][s->write[line]]=x;advance_heads(s,line);
 if(s->valid[line]<native_reverb_capacities[line])s->valid[line]++;
}
static inline float allpass_slow(NativeReverb *s,unsigned line,float x,float coefficient){
 float old=read_line_slow(s,line);float v=x-old*coefficient;write_line_slow(s,line,v);v*=.7f;v+=old;return v;
}
static inline float delay_slow(NativeReverb *s,unsigned line,float x){float old=read_line_slow(s,line);write_line_slow(s,line,x);return old;}
static inline __attribute__((always_inline)) float read_line_fast(const NativeReverb *s,unsigned line){return s->history[line][s->read[line]];}
static inline __attribute__((always_inline)) void write_line_fast(NativeReverb *s,unsigned line,float x){s->history[line][s->write[line]]=x;advance_heads(s,line);}
static inline __attribute__((always_inline)) float allpass_fast(NativeReverb *s,unsigned line,float x,float coefficient){
 float old=read_line_fast(s,line);float v=x-old*coefficient;write_line_fast(s,line,v);v*=.7f;v+=old;return v;
}
static inline __attribute__((always_inline)) float delay_fast(NativeReverb *s,unsigned line,float x){float old=read_line_fast(s,line);write_line_fast(s,line,x);return old;}
#define RV_BODY(AP,DELAY) do { \
 float x=input*s->send;x+=0.f; \
 x=AP(s,0,x,s->coefficient[0]); x=AP(s,1,x,s->coefficient[1]); \
 x=AP(s,2,x,s->coefficient[2]); x=AP(s,3,x,s->coefficient[3]); \
 x=AP(s,4,x,s->coefficient[4]); x=AP(s,5,x,s->coefficient[5]); \
 float a=s->decay*s->feedback_state[1];a+=x; \
 float b=s->decay*s->feedback_state[0];b+=x; \
 a=AP(s,6,a,s->coefficient[6]); a=AP(s,7,a,s->coefficient[7]); \
 float delayed_a=DELAY(s,8,a); \
 b=AP(s,9,b,s->coefficient[8]); b=AP(s,10,b,s->coefficient[9]); \
 float delayed_b=DELAY(s,11,b); \
 float l=delayed_b*s->low_b[0]; float t=s->low_a[0]*s->feedback_state[0]; \
 l+=t;s->feedback_state[0]=l;l+=a;*left=l; \
 float r=s->low_a[1]*s->feedback_state[1];t=delayed_a*s->low_b[1]; \
 r+=t;s->feedback_state[1]=r;r+=b;*right=r; \
} while(0)
#ifndef PLAT_HOST
#define RV_RAM_FAST __attribute__((noinline,used))
#else
#define RV_RAM_FAST __attribute__((noinline))
#endif
static inline void step_slow(NativeReverb *s,float input,float *left,float *right){RV_BODY(allpass_slow,delay_slow);++s->warm_samples;}
static inline void step_fast(NativeReverb *s,float input,float *left,float *right){RV_BODY(allpass_fast,delay_fast);}
typedef void (*ReverbFastBlockFn)(NativeReverb *,const float *,float *,float *,unsigned);
/* TEST30: keep the steady-state reverb loop in XIP. Hardware profiling showed
   the native voice renderer dominates the deadline, so scarce executable SRAM
   is reserved for the shared voice hot loop instead. */
static RV_RAM_FAST void process_fast_ram(NativeReverb *s,const float *src,float *dst_l,float *dst_r,unsigned n){
 for(unsigned i=0;i<n;i++){float input=src[i];float *left=dst_l+i,*right=dst_r+i;RV_BODY(allpass_fast,delay_fast);}
}
static ReverbFastBlockFn volatile process_fast_ram_ptr=process_fast_ram;
void native_reverb_step(NativeReverb *s,float input,float *left,float *right){
 if(s->warm_samples>=5859u)step_fast(s,input,left,right);else step_slow(s,input,left,right);
}
void native_reverb_process(NativeReverb *s,const float *input,float *left,float *right,unsigned n){
 unsigned i=0;
 while(i<n && s->warm_samples<5859u){float x=input[i];step_slow(s,x,left+i,right+i);++i;}
 if(i<n){ReverbFastBlockFn fn=process_fast_ram_ptr;fn(s,input+i,left+i,right+i,n-i);}
}
#undef RV_BODY
