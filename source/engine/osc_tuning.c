#include "osc_tuning.h"
/* RVA 0x285cb..0x28661, with controller offsets. Original order matters at
   fractional boundaries: sum float cents before truncating to 1/128 semitone. */
float osc_base_pitch_float(unsigned note,int octave,int semitone,int cents,int master_cents,float wheel,float breath,float pressure){
 float pitch=(float)octave*12.f;pitch*=128.f;
 pitch+=(float)semitone*128.f;
 pitch+=(float)note*128.f;
 pitch+=((float)cents*128.f)/100.f;
 pitch+=((float)master_cents*128.f)/100.f;
 /* Original 0x2864a..0x28661: wheel, pressure, breath before truncation. */
 pitch+=wheel;pitch+=pressure;pitch+=breath;
 return pitch;
}

int32_t osc_base_pitch_offsets(unsigned note,int octave,int semitone,int cents,int master_cents,float wheel,float breath,float pressure){
 return (int32_t)osc_base_pitch_float(note,octave,semitone,cents,master_cents,wheel,breath,pressure);
}

int32_t osc_base_pitch(unsigned note,int octave,int semitone,int cents,int master_cents){
 return osc_base_pitch_offsets(note,octave,semitone,cents,master_cents,0.f,0.f,0.f);
}
float osc_controller_pitch(unsigned value,float semitones){
 if(value>127)value=127;float depth=semitones*128.f;return (value/127.f)*depth;
}
