#pragma once
#include <stdint.h>
#include "amp_runtime.h"
#include "mod_envelope.h"
#include "portamento.h"

typedef struct {
 uint32_t phase,inc,width;
 unsigned wave;
 int table_index;
 int32_t drift;
 float blend;
 const float *a,*b;
} VoiceOsc;

typedef struct {
 VoiceOsc *osc;
 uint32_t (*lfo)[14];
 uint32_t *noise_seed;
 uint32_t *width_mod;
 int32_t *pitch_lfo_depth,*pitch_env_depth,*bend_offsets;
 ModEnvelope *mod_env;
 unsigned *mod_tick;
 AmpRuntime *amp,*fm_env;
 float *filter,*gains,*current_gain;
 const float *mix_scale;
 const int32_t *cutoff_target;
 int32_t *cutoff_smoothed;
 const int32_t *filter_env_depth,*filter_lfo_depth;
 const float *fm_depth,*fm_amount;
 const unsigned *held;
 const uint8_t *vel;
 const unsigned *velocity_native_active;
 float *controller_amp_gain;
 const float *controller_amp_target;
 Portamento *glide;
 uint32_t *faults;
 void (*tune_mask)(unsigned);
 void (*tune_all)(void);
 void (*update_width)(void);
 void (*panic)(void);
} VoiceRenderContext;

void voice_render_common(const VoiceRenderContext *c,float *out,unsigned n);
typedef void (*VoiceRenderCommonFn)(const VoiceRenderContext *,float *,unsigned);
extern VoiceRenderCommonFn const voice_render_common_ptr;
