#include "midi_level.h"
void midi_level_init(MidiLevel *s){s->current=s->target=1.f;s->step=0;s->remaining=0;s->volume=s->expression=127;}
void midi_level_set(MidiLevel *s,unsigned cc,unsigned value){
 if(value>127)value=127;
 if(cc==7)s->volume=value;else if(cc==11)s->expression=value;else return;
 float target=(float)(s->volume*s->expression)/16129.f;
 if(target==s->target)return; /* Repeated MIDI values must not restart a ramp. */
 s->target=target;s->remaining=256;s->step=(target-s->current)/256.f;
}
void midi_level_apply(MidiLevel *s,float *out,unsigned count){
 if(!s->remaining && s->current==1.f)return;
 for(unsigned i=0;i<count;i++){
  if(s->remaining){s->current+=s->step;if(!--s->remaining)s->current=s->target;}
  out[i]*=s->current;
 }
}

void midi_level_apply_stereo(MidiLevel *s,float *left,float *right,unsigned count){
 if(!s->remaining && s->current==1.f)return;
 for(unsigned i=0;i<count;i++){
  if(s->remaining){s->current+=s->step;if(!--s->remaining)s->current=s->target;}
  left[i]*=s->current;right[i]*=s->current;
 }
}
