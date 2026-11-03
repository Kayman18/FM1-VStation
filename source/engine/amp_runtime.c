#include "amp_runtime.h"
#include <string.h>
void amp_attack(float,float*,uint32_t*,uint32_t);
void amp_fall(float,float,float*,uint32_t*,uint32_t);
/* Gate subset traced in 0x43de0, 0x44062..0x440c0.
   Other envelope/LFO gates and voice allocator remain outside this module. */
void amp_gate(AmpRuntime *e,unsigned retrigger){
 if(!retrigger)return;
 if(e->mode==5 && e->smoothed<.001f){e->value=0;e->smoothed=0;}
 e->mode=0;
}
void amp_release(AmpRuntime *e){
 if(e->mode!=5 || e->smoothed>=.001f)e->mode=3;
}
/* Voice gain smoothing 0x379b2..0x379e8; engine control update follows audio. */
float amp_audio(AmpRuntime *e){
 if(e->mode==5 && e->smoothed<.001f)return 0;
 e->smoothed=e->output*.1f+e->smoothed*.9f;
 uint32_t bits;memcpy(&bits,&e->smoothed,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))e->smoothed=0;
 return e->smoothed;
}
void amp_control(AmpRuntime *e){
 if(e->tick==0){
  if(e->mode==0)amp_attack(e->attack,&e->value,&e->mode,1);
  else if(e->mode==1)amp_fall(e->decay,e->sustain,&e->value,&e->mode,2);
  else if(e->mode==2)e->value=e->sustain;
  else if(e->mode==3)amp_fall(e->release,0,&e->value,&e->mode,5);
  else if(e->mode==5)e->value=0;
  e->output=e->gain*e->value;
 }
 e->tick=(e->tick+1)&15;
}
