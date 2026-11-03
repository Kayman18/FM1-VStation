#ifndef OSC_FM_H
#define OSC_FM_H
#include <stdint.h>
/* Depth is the already-squared normalized control, clamped internally. */
uint32_t osc_fm_increment(uint32_t base,float modulator,float depth);
/* Recovered smoothing/depth primitive; used by the FM envelope scheduler. */
float osc_fm_depth(float output,float *smoothed,float base,float amount);
#endif
