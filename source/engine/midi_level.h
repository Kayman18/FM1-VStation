#pragma once
typedef struct {float current,target,step;unsigned remaining,volume,expression;} MidiLevel;
void midi_level_init(MidiLevel *s);
void midi_level_set(MidiLevel *s,unsigned cc,unsigned value);
void midi_level_apply(MidiLevel *s,float *out,unsigned count);

/* Advance the shared volume ramp once per stereo frame, not once per channel. */
void midi_level_apply_stereo(MidiLevel *s,float *left,float *right,unsigned count);
