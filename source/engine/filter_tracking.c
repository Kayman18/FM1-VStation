#include "filter_tracking.h"
/* Original note gate 0x44128..0x44197 and tracking setter 0x46b10.
   Float addition precedes the Q7 conversion. Keep -ffp-contract=off. */
int32_t filter_tracking_offsets(float cutoff,unsigned note,float tracking,float wheel,float breath,float pressure){
 float keyed=((float)note-60.f)*tracking;
 float target=keyed+cutoff*127.f;
 target+=wheel;target+=breath;target+=pressure;
 return (int32_t)(target*128.f);
}

int32_t filter_tracking_target(float cutoff,unsigned note,float tracking){
 return filter_tracking_offsets(cutoff,note,tracking,0.f,0.f,0.f);
}
/* Original CC/pressure normalization before signed destination scaling. */
float filter_controller_offset(unsigned value,float semitones){
 if(value>127)value=127;return (value/127.f)*semitones;
}
