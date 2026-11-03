#include <stdint.h>
#ifndef VSTATION_TARGET
typedef void (__attribute__((ms_abi))* Stage)(void*,float*,uint32_t*);
void reference_stage(void *fn,void *s,float *v,uint32_t *mode){((Stage)fn)(s,v,mode);}
#endif
/* Recovered RVA 0x25fe0 attack and 0x26040 falling stage.
   Voice-list cleanup callback is deliberately external to this numeric step. */
void amp_attack(float rate,float *v,uint32_t *mode,uint32_t next){*v+=rate;if(*v>=1.0f){*v=1.0f;*mode=next;}}
void amp_fall(float rate,float target,float *v,uint32_t *mode,uint32_t next){
 float delta=*v-target;
 if(delta<=.001f){*v=target;*mode=target==0.0f?5:next;}
 else {*v=delta*rate+target;}
}
