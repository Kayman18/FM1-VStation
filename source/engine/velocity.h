#ifndef VSTATION_VELOCITY_H
#define VSTATION_VELOCITY_H
#include <stdint.h>
/* Original normalized sensitivity, normally -0.5 .. +0.5. */
float velocity_gain(unsigned velocity,float sensitivity,unsigned cubic);
uint32_t velocity_mod_gain(float gain);
#endif
