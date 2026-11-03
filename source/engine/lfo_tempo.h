#pragma once
#include <stdint.h>
/* FM1 adapter policy. 0 selects the unchanged recovered free-rate table. */
#define LFO_TEMPO_DIVISIONS 34
uint32_t lfo_tempo_increment(unsigned bpm,unsigned division,uint32_t free_rate);

uint32_t lfo_tempo_increment_milli(unsigned bpm_milli,unsigned division,uint32_t free_rate);
