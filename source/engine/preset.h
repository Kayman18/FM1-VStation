#pragma once
#include <stddef.h>
#include <stdint.h>
#include "synth.h"
#define PRESET_BYTES (8+SYNTH_PARAM_COUNT+4)
#define PRESET_TEST40_BYTES (8+177+4) /* TEST23 through TEST40 legacy 177-control layout; removed tail ignored by TEST41+. */
#define PRESET_TEST22_BYTES (8+168+4) /* TEST22 layout. */
#define PRESET_TEST20_BYTES (8+163+4) /* TEST20 and TEST21 use this same layout. */
#define PRESET_TEST18_BYTES (8+160+4) /* TEST18 and TEST19 use this same layout. */
#define PRESET_TEST12_BYTES (8+127+4)
#define PRESET_TEST13_BYTES (8+128+4)
#define PRESET_TEST14_BYTES (8+132+4)
#define PRESET_TEST15_BYTES (8+138+4)
#define PRESET_TEST17_BYTES (8+152+4)
#define PRESET_TEST16_BYTES (8+143+4)
/* Complete FM1 parameter snapshot, version 1. Only exact TEST12 (127),
   TEST13 (128), TEST14 (132), TEST15 (138), TEST16 (143), TEST17 (152), TEST18/TEST19 (160), TEST20/TEST21 (163), TEST22 (168), TEST23-TEST40 (177), and current (170) layouts are accepted. Older
   snapshots retain prior controls and migrate with newly added effects bypassed.
   No original VST bank accepted. Apply runs only on the audio owner. */
int preset_decode(const uint8_t *data,size_t size,unsigned out[SYNTH_PARAM_COUNT]);
int preset_apply(const uint8_t *data,size_t size);

unsigned preset_param_min(unsigned id);
unsigned preset_param_max(unsigned id);
int preset_encode(const unsigned values[SYNTH_PARAM_COUNT],uint8_t *out,size_t size);
