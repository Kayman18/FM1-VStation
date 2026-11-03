#include "lfo_sync.h"
/* Original note gate 0x4419d..0x44212; phase table RVA 0x1538f0.
   Reset crossing history with phase; leave waveform output and RNG intact. */
void lfo_key_sync(uint32_t *state,unsigned phase){
 static const uint32_t start[4][2]={{0x40000000u,0u},{0u,0x80000000u},{0x80000000u,0u},{0u,0u}};
 unsigned wave=state[0]&255u;if(wave>3)wave=3;
 state[2]=state[3]=start[wave][phase?1:0];
}
