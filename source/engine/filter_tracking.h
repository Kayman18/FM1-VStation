#ifndef FILTER_TRACKING_H
#define FILTER_TRACKING_H
#include <stdint.h>
/* Normalized cutoff/tracking, MIDI note 0..127. No clamp here: the original
   smooths the target before clamping the final modulated table index. */
int32_t filter_tracking_target(float cutoff,unsigned note,float tracking);
int32_t filter_tracking_offsets(float cutoff,unsigned note,float tracking,float wheel,float breath,float pressure);
float filter_controller_offset(unsigned value,float semitones);
#endif
