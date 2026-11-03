#include "native_eq.h"
#include "native_delay.h"
#include "native_chorus.h"
#include "native_reverb.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rng=0x2468ace1u;
static float sample(void){rng=rng*1664525u+1013904223u;int32_t x=(int32_t)(rng>>8)-0x7fffff;return (float)x*(1.0f/8388608.0f)*.2f;}
static void samef(float a,float b){assert(!memcmp(&a,&b,4));}
static void histories(float *storage,float *out[NATIVE_REVERB_LINES]){unsigned off=0;for(unsigned i=0;i<NATIVE_REVERB_LINES;i++){out[i]=storage+off;off+=native_reverb_capacities[i];}assert(off==28535);}
int main(void){enum{N=128,BLOCKS=160};
 float in[N],a[N],b[N],la[N],ra[N],lb[N],rb[N];
 /* EQ: block path vs public per-sample path. */
 NativeEq e1,e2;native_eq_init(&e1);native_eq_init(&e2);native_eq_depth(&e1,.25f);native_eq_depth(&e2,.25f);native_eq_frequency(&e1,43);native_eq_frequency(&e2,43);native_eq_modulation(&e1,.63f);native_eq_modulation(&e2,.63f);native_eq_rate(&e1,.42f);native_eq_rate(&e2,.42f);
 for(unsigned q=0;q<BLOCKS;q++){for(unsigned i=0;i<N;i++)a[i]=b[i]=sample();native_eq_process(&e1,a,N);for(unsigned i=0;i<N;i++)b[i]=native_eq_step(&e2,b[i]);assert(!memcmp(a,b,sizeof a));}
 /* Delay: process-add preserves exact dry + wet arithmetic. */
 static float db1[65536],db2[65536];NativeDelay d1,d2;native_delay_init(&d1,db1);native_delay_init(&d2,db2);native_delay_level(&d1,.55f);native_delay_level(&d2,.55f);native_delay_time(&d1,.44f);native_delay_time(&d2,.44f);native_delay_feedback(&d1,.39f);native_delay_feedback(&d2,.39f);native_delay_width(&d1,.77f);native_delay_width(&d2,.77f);native_delay_ratio(&d1,4);native_delay_ratio(&d2,4);
 for(unsigned q=0;q<BLOCKS;q++){for(unsigned i=0;i<N;i++){in[i]=sample();la[i]=lb[i]=in[i];ra[i]=rb[i]=in[i];}native_delay_process_add(&d1,in,la,ra,N);for(unsigned i=0;i<N;i++){float l,r;native_delay_step(&d2,in[i],&l,&r);lb[i]+=l;rb[i]+=r;}assert(!memcmp(la,lb,sizeof la)&&!memcmp(ra,rb,sizeof ra));}
 /* Chorus/phaser block-add vs per-sample. */
 NativeChorus c1,c2;native_chorus_init(&c1);native_chorus_init(&c2);native_chorus_level(&c1,.8f);native_chorus_level(&c2,.8f);native_chorus_rate(&c1,.31f);native_chorus_rate(&c2,.31f);native_chorus_depth(&c1,.47f);native_chorus_depth(&c2,.47f);native_chorus_centre(&c1,.1f);native_chorus_centre(&c2,.1f);native_chorus_feedback(&c1,.12f);native_chorus_feedback(&c2,.12f);
 for(unsigned q=0;q<BLOCKS;q++){if(q==80){native_chorus_type(&c1,1);native_chorus_type(&c2,1);}for(unsigned i=0;i<N;i++){in[i]=sample();la[i]=lb[i]=in[i];ra[i]=rb[i]=in[i];}native_chorus_process_add(&c1,in,la,ra,N);for(unsigned i=0;i<N;i++){float l,r;native_chorus_step(&c2,in[i],&l,&r);lb[i]+=l;rb[i]+=r;}assert(!memcmp(la,lb,sizeof la)&&!memcmp(ra,rb,sizeof ra));}
 /* Reverb: block processor matches public step path before and after warm-up, including room changes. */
 static float rs1[28535],rs2[28535];float *rh1[12],*rh2[12];histories(rs1,rh1);histories(rs2,rh2);NativeReverb v1,v2;native_reverb_init(&v1,rh1);native_reverb_init(&v2,rh2);native_reverb_level(&v1,.7f);native_reverb_level(&v2,.7f);native_reverb_decay_index(&v1,53);native_reverb_decay_index(&v2,53);
 for(unsigned q=0;q<BLOCKS;q++){if(q==48||q==96){unsigned room=q==48?5:2;native_reverb_type(&v1,room);native_reverb_type(&v2,room);native_reverb_decay_index(&v1,71);native_reverb_decay_index(&v2,71);}for(unsigned i=0;i<N;i++)in[i]=sample();native_reverb_process(&v1,in,la,ra,N);for(unsigned i=0;i<N;i++)native_reverb_step(&v2,in[i],lb+i,rb+i);assert(!memcmp(la,lb,sizeof la)&&!memcmp(ra,rb,sizeof ra));}
 printf("PASS: TEST24 EQ/delay/chorus/reverb block processors are bit exact vs per-sample paths across %u frames\n",BLOCKS*N);
 return 0;
}
