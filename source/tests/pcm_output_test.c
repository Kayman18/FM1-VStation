#include "pcm_output.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
 unsigned seed=14567;
 for(unsigned i=0;i<1000000;i++){
  seed=1664525u*seed+1013904223u;float x=((int)(seed>>8)-8388608)/8388608.f;
  seed=1664525u*seed+1013904223u;float gain=(seed>>8)/16777215.f;
  assert(pcm_output(x,gain)==(int32_t)(x*8388352.f*gain));
 }
 assert(pcm_output(2.f,1.f)==8388607);assert(pcm_output(-2.f,1.f)==-8388608);
 assert(pcm_output(INFINITY,1.f)==8388607);assert(pcm_output(-INFINITY,1.f)==-8388608);
 assert(pcm_output(NAN,1.f)==0);assert(pcm_output(0.f,1.f)==0);
 puts("PASS: final PCM saturation, NaN/Inf handling, one million in-range conversions identical to TEST13");
}
