#include <stdint.h>
void coefficients(float*);
/* Base cutoff smoothing/interpolation in RVA 0x37410. Modulation upstream of
   index addition is not included. Run once every 16 audio samples. */
void filter_control(int32_t target,int32_t *smooth,int32_t modulation,const float *table,float *s){
 *smooth=target/64+(*smooth*63)/64;
 int32_t index=*smooth+modulation;if(index<0)index=0;if(index>16256)index=16256;
 unsigned i=(unsigned)index/128;float fraction=(float)(index&127)/128.f;
 float c=table[i]*(1.f-fraction)+table[i+1]*fraction;
 if(c!=s[5]){s[5]=c;coefficients(s);}
}
/* Resonance portion of RVA 0x26b50. s[7] is compensation strength. */
void filter_resonance(float *s,float resonance){
 if(resonance!=s[6]){s[6]=resonance;s[12]=1.f-resonance;coefficients(s);s[8]=1.f-s[7]*s[6];}
}
#ifndef VSTATION_TARGET
#include <math.h>
typedef void (__attribute__((ms_abi))*Init)(float);
typedef void (__attribute__((ms_abi))*Control)(void*);
typedef void (__attribute__((ms_abi))*Resonance)(void*,float);
void reference_filter_init(void *fn,float sr){((Init)fn)(sr);}
void reference_filter_control(void *fn,void *s){((Control)fn)(s);}
void reference_resonance(void *fn,void *s,float r){((Resonance)fn)(s,r);}
void filter_table(float sr,float *table){
 float scale=261.6260070800781f/(sr*2.f);
 for(int i=0;i<128;i++){
  float v=(float)pow(2.,(double)(i-48)/12.);v*=scale;
  if(v>=.25f)v=.2498999983072281f;
  v=(float)sin((double)v*3.141592653589793);v*=2.f;
  if(v<0)v=0;if(v>1)v=1;table[i]=v;
 }
 table[128]=0; /* Original read reaches next zero-initialized table, weight 0. */
}
#endif
