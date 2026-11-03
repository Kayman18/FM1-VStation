#pragma once
void amp_level_init(void);
void amp_level_param(unsigned id,unsigned value);
void amp_level_reset(void);
void amp_level_unison(unsigned count);
void amp_level_apply(float *out,unsigned n);
float amp_level_curve(unsigned value);
