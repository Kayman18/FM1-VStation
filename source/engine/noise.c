#include <stdint.h>
#include <string.h>
/* RVA 0x43b30. Original engine seed is zero and scale bits are 0x30000000.
   Unsigned arithmetic preserves the x86 generator's modulo-2^32 state. */
float noise_step(uint32_t *seed){
 *seed=*seed*1664525u+1013904223u;
 int32_t signed_value;memcpy(&signed_value,seed,sizeof(signed_value));
 float value=(float)signed_value;return value*0x1p-31f;
}
