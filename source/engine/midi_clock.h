#pragma once
#include <stdint.h>
typedef struct {uint32_t first,last;uint32_t spans[4],sum;unsigned ticks,seen,locked,stopped,bpm_milli,count,cursor;} MidiClock;
void midi_clock_reset(MidiClock *c);
void midi_clock_event(MidiClock *c,unsigned status,uint32_t ms);
void midi_clock_poll(MidiClock *c,uint32_t ms);
