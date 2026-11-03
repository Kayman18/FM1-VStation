#include "shared_lfo.h"
#include <string.h>
void lfo_step(uint32_t *);
uint32_t shared_lfo_state[2][14];
unsigned shared_lfo_mask;
static unsigned tick;
void shared_lfo_init(void){memset(shared_lfo_state,0,sizeof(shared_lfo_state));shared_lfo_mask=0;tick=0;}
void shared_lfo_config(unsigned i,const uint32_t *source){
 if(i>=2)return;
 shared_lfo_state[i][0]=source[0];shared_lfo_state[i][4]=source[4];shared_lfo_state[i][5]=source[5];
}
void shared_lfo_enable(unsigned i,unsigned enabled,const uint32_t *source){
 if(i>=2)return;unsigned bit=1u<<i;
 if(enabled){if(!(shared_lfo_mask&bit))memcpy(shared_lfo_state[i],source,14*sizeof(uint32_t));shared_lfo_mask|=bit;}
 else shared_lfo_mask&=~bit;
}
void shared_lfo_panic(void){
 for(unsigned i=0;i<2;i++){
  uint32_t config[14]={0};config[0]=shared_lfo_state[i][0];config[4]=shared_lfo_state[i][4];config[5]=shared_lfo_state[i][5];
  memcpy(shared_lfo_state[i],config,sizeof(config));
 }tick=0;
}
unsigned shared_lfo_slice(unsigned requested){
 if(!requested)return 0;
 if(tick==0)for(unsigned i=0;i<2;i++)if(shared_lfo_mask&(1u<<i))lfo_step(shared_lfo_state[i]);
 unsigned left=16-tick;return requested<left?requested:left;
}
void shared_lfo_advance(unsigned samples){tick=(tick+samples)&15u;}
unsigned shared_lfo_boundary(void){return tick==0;}
