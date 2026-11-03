#ifndef VSTATION_OSC_TUNING_H
#define VSTATION_OSC_TUNING_H
#include <stdint.h>
/* Bounded MIDI note and signed-byte octave/semitone/cent inputs. */
int32_t osc_base_pitch(unsigned note,int octave,int semitone,int cents,int master_cents);
int32_t osc_base_pitch_offsets(unsigned note,int octave,int semitone,int cents,int master_cents,float wheel,float breath,float pressure);
float osc_base_pitch_float(unsigned note,int octave,int semitone,int cents,int master_cents,float wheel,float breath,float pressure);
float osc_controller_pitch(unsigned value,float semitones);
#endif
