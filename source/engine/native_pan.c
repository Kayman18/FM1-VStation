#include "native_pan.h"
#include "native_eq.h" /* Pan uses the same native 35-entry beat-period table. */
#include <string.h>
static void update_rate(NativePan *s){
 float hz=s->free_rate_squared*10.f;
 if(s->sync && s->bpm>0.f){hz=s->bpm*0x1.b4e81cp-10f;hz/=s->period;hz*=10.f;}
 s->hz=hz;s->phase_step=hz/44100.f;
 s->increment=(uint32_t)((hz*8388608.f)/44100.f);
}
void native_pan_position(NativePan *s,float x){s->position=x*2.f;}
void native_pan_depth(NativePan *s,float x){s->depth=x*2.f;}
void native_pan_rate(NativePan *s,float x){s->free_rate_squared=x*x;if(!s->sync)update_rate(s);}
void native_pan_sync(NativePan *s,unsigned index){if(index>34)index=34;s->sync=!!index;s->period=native_eq_periods[index];update_rate(s);}
void native_pan_tempo(NativePan *s,float bpm){if(bpm>0.f){s->bpm=bpm;if(s->sync)update_rate(s);}}
void native_pan_reset(NativePan *s){
 /* FM1 panic/Init policy: restore free phase and clear pan smoothing. This
    does not claim to reproduce the original note-trigger dispatcher. */
 s->phase=0;s->phase_value=0.f;s->output=0.f;s->fade=1.f;s->smoothed=0.f;
}
void native_pan_init(NativePan *s){
 memset(s,0,sizeof(*s));s->bpm=120.f;s->period=1.f;
 s->fade_step=1.f/(.01f*44100.f);native_pan_reset(s);native_pan_rate(s,0.f);
}
void native_pan_step(NativePan *s,float input,float *left,float *right){
 /* Original float triangle: evaluate OLD phase, then wrap the 23-bit phase
    through a float mantissa. Preserve operation order and separate rounding. */
 float x=s->phase_value;
 s->output=(x>=.5f?.75f-x:x-.25f)*2.f;
 s->phase+=s->increment;
 uint32_t bits=(s->phase&0x7fffffu)+0x3f800000u;
 memcpy(&s->phase_value,&bits,4);s->phase_value-=1.f;
 s->fade+=s->fade_step;if(s->fade>1.f)s->fade=1.f;
 s->output*=s->fade;
 float target=s->output*s->depth;target+=s->position;
 if(target< -1.f)target=-1.f;else if(target>1.f)target=1.f;
 s->smoothed=s->smoothed*.875f+target*.125f;
 memcpy(&bits,&s->smoothed,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))s->smoothed=0.f;
 *left=(1.f-s->smoothed)*input;*right=(s->smoothed+1.f)*input;
}
