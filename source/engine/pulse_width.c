#include <stdint.h>
/* Static phase displacement from RVA 0x46720; v in [-0.5,0.5).
   Add to 0x80000000 for pulse; to zero for the other original modes. */
uint32_t pulse_offset(float v){float x=v*2.f;x*=2147483648.f;return (uint32_t)(int32_t)x;}
#ifndef VSTATION_TARGET
typedef void (__attribute__((ms_abi))*Setter)(void*,unsigned char,float);
void reference_width(void *fn,void *s,unsigned char osc,float v){((Setter)fn)(s,osc,v);}
#endif
