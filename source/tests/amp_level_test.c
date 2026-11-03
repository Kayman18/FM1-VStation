#include "amp_level.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
static float a[2048],b[2048];
int main(void){
 for(unsigned v=52;v<=76;v++)assert(fabsf(amp_level_curve(v)-powf(10.f,((int)v-70)/20.f))<1e-6f);
 amp_level_init();for(unsigned i=0;i<2048;i++)a[i]=b[i]=.1f;
 amp_level_apply(a,2048);assert(!memcmp(a,b,sizeof(a)));
 amp_level_param(121,1);amp_level_param(120,52);amp_level_apply(a,2048);
 amp_level_init();amp_level_param(121,1);amp_level_param(120,52);
 for(unsigned i=0;i<2048;i++)amp_level_apply(b+i,1);
 assert(!memcmp(a,b,sizeof(a)));
 for(unsigned i=1;i<2048;i++)assert(a[i]<=a[i-1] && a[i]>0.f);
 assert(fabsf(a[2047]-.1f*amp_level_curve(52))<1e-6f);
 amp_level_param(121,0);for(unsigned i=0;i<2048;i++)a[i]=1.f;
 amp_level_apply(a,2048);assert(a[2047]==1.f);
 puts("Original amplitude curve, unity bypass, smoothing and block invariance passed");
}
