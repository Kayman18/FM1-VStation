#include <math.h>
#ifndef VSTATION_TARGET
typedef void (__attribute__((ms_abi)) *Original)(void*,float);
void reference(void *fn, float *s, float x){((Original)fn)(s,x);}
#endif
/* Recovered two-pole sample step, RVA 0x26560. Offsets expressed as float slots.
   State layout retained only for comparison. Coefficient mapping is not yet ported. */
void portable(float *s,float x){
 x*=s[8];
 float y=x*s[13]/((fabsf(x)-0.9999924898147583f)*s[14]+1.0f);
 float q=s[10];
 if(q<.25f){q=fabsf(s[3])-.25f;if(q<s[10])q=s[10];if(q>1)q=1;}
 float old=s[3];float low=s[9]*old+s[4];s[4]=low;
 float high=y-low;high-=old*q;s[2]=high;s[3]=high*s[9]+old;
 q=s[10];if(q<.25f){q=fabsf(s[3])-.25f;if(q<s[10])q=s[10];if(q>1)q=1;}
 old=s[3];low=s[9]*old+s[4];s[4]=low;high=y-low;high-=old*q;s[2]=high;s[3]=high*s[9]+old;
}
static void stage(float *s,int off,float x){
 float q=s[10],old=s[off+1];
 if(q<.25f){q=fabsf(old)-.25f;if(q<s[10])q=s[10];if(q>1)q=1;}
 float low=s[9]*old+s[off+2];s[off+2]=low;
 float high=x-low;high-=old*q;s[off]=high;s[off+1]=high*s[9]+old;
}
void portable4(float *s,float x){
 x*=s[8];float y=x*s[13]/((fabsf(x)-0.9999924898147583f)*s[14]+1.0f);
 if(s[10]>=.25f){
  /* The Q guard in stage() only inspects s[10]; when it cannot fire the four
     stages are straight-line arithmetic.  Same operations in the same order
     with the state held in locals and stored once (bit-identical). */
  const float c=s[9],q=s[10];
  float oa=s[3],la=s[4],ob=s[17],lb=s[18],ha,hb;
  la=c*oa+la;ha=y-la;ha-=oa*q;oa=ha*c+oa;
  lb=c*ob+lb;hb=la-lb;hb-=ob*q;ob=hb*c+ob;
  la=c*oa+la;ha=y-la;ha-=oa*q;oa=ha*c+oa;
  lb=c*ob+lb;hb=la-lb;hb-=ob*q;ob=hb*c+ob;
  s[2]=ha;s[3]=oa;s[4]=la;s[16]=hb;s[17]=ob;s[18]=lb;
  return;
 }
 stage(s,2,y);stage(s,16,s[4]);stage(s,2,y);stage(s,16,s[4]);
}
void coefficients(float *s){
 float c=s[5],q=s[12]*0.707099974155426f;
 float t=c*q;t*=.5f;q*=1.0f-t;
 if(q<c*.125f)q=c*.125f;
 s[9]=c;q*=2.0f;s[10]=q;q*=2.0f;s[11]=q;
}
