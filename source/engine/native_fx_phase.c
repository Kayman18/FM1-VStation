#include "native_fx_phase.h"
#include <string.h>
void native_fx_phase_init(NativeFxPhase *s){memset(s,0,sizeof(*s));}
void native_fx_phase_clear(NativeFxPhase *s){memset(s->held,0,sizeof(s->held));s->count=0;s->pending=0;}
void native_fx_phase_set(NativeFxPhase *s,unsigned effect,unsigned value){if(effect<3)s->initial[effect]=(uint8_t)(value>3?3:value);}
void native_fx_phase_input(NativeFxPhase *s){if(!s->count)s->pending=1;}
void native_fx_phase_note_on(NativeFxPhase *s,unsigned note){if(note<128 && s->count<128){++s->held[note];++s->count;}}
void native_fx_phase_note_off(NativeFxPhase *s,unsigned note){if(note<128 && s->held[note]){--s->held[note];--s->count;}}
void native_fx_phase_dispatch(NativeFxPhase *s,NativeChorus *c,NativeEq *e,NativePan *p){
 if(!s->pending)return;
 unsigned v=s->initial[0];if(c && v)c->phase=(v-1u)<<21;
 v=s->initial[1];if(e && v)e->lfo[3]=(v-1u)<<30;
 v=s->initial[2];if(p && v)p->phase=(v-1u)<<21;
 s->pending=0;
}
