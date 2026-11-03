#include "osc_fm.h"
#include <string.h>
/* RVA 0x286a0. Float-only approximation of the original double multiply/add
   for the factor. Deliberately no software-double in the audio loop.
   Build requires -ffp-contract=off, as used by this project. */
float osc_fm_depth(float output,float *smoothed,float base,float amount){
 *smoothed=output*.1f+*smoothed*.9f;
 uint32_t bits;memcpy(&bits,smoothed,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))*smoothed=0;
 float depth=amount * (*smoothed)+base;
 if(depth<0)depth=0;if(depth>1)depth=1;
 return depth;
}
uint32_t osc_fm_increment(uint32_t base,float modulator,float depth){
 if(depth<0)depth=0;if(depth>1)depth=1;
 int32_t signed_base;memcpy(&signed_base,&base,4);
 float factor=modulator*(depth*32.f)+1.f;
 float step=(float)signed_base*factor;
 /* cvttss2si yields INT_MIN for NaN and out-of-range values. Bounds are
    exactly representable; checking before conversion avoids C undefined behaviour. */
 if(!(step>=-2147483648.f && step<2147483648.f))return 0x80000000u;
 return (uint32_t)(int32_t)step;
}
