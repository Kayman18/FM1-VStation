#include "osc_drift.h"
/* Original 0x47830: fixed signed offsets, not a random generator.
   The exact binary factor 127/128 needs no double on the FM1. */
float osc_drift_scale(unsigned oscillator,unsigned value){
 if(value>127)value=127;
 if(oscillator==0 || oscillator>2 || !value)return 0.f;
 float scale=(value/127.f)*.9921875f;
 scale*=oscillator==1?39.f:-17.f;
 scale*=.00048828125f;
 return scale;
}
/* Original 0x2866f..0x28696: use untruncated base pitch, before pitch
   envelope/LFO/bend/glide index changes, then truncate toward zero. */
int32_t osc_drift_increment(float base_pitch,float scale){
 if(scale==0.f)return 0;
 float x=base_pitch/49152.f;x*=scale;x*=4.f;x*=16777216.f;
 return (int32_t)x;
}
