#include <stdint.h>
#ifndef VSTATION_TARGET
typedef float (__attribute__((ms_abi)) *OriginalOsc)(void*);
float reference_osc(void *fn,void *state){return ((OriginalOsc)fn)(state);}
#endif
/* Recovered waveform-selection arithmetic from RVA 0x37100.
   Caller supplies phase/increment, pulse offset and the selected note tables.
   Modulation conversion and oscillator sync remain outside this primitive. */
float waveform(unsigned wave,uint32_t phase,uint32_t offset,float weight,
 const float *a,const float *b,const float *sine){
 unsigned i=phase>>21,j=(phase+offset)>>21;
 if(wave==0)return (sine[j]+sine[i])*.5f;
 if(wave==3){float x=a[i]-a[j],y=b[i]-b[j];return (y-x)*weight+x;}
 float x=a[j]+a[i],y=b[j]+b[i];return ((y-x)*weight+x)*.5f;
}
