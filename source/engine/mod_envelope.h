#ifndef MOD_ENVELOPE_H
#define MOD_ENVELOPE_H
#include <stdint.h>
/* Original 0x4c-byte integer-envelope layout. Rates use precomputed Q31 tables at 44100/8 Hz. */
typedef struct { uint32_t word[19]; } ModEnvelope;
void mod_envelope_step(ModEnvelope *e);
int32_t mod_envelope_filter(const ModEnvelope *e,int32_t depth);
void mod_envelope_init(ModEnvelope *e);
void mod_envelope_gate(ModEnvelope *e,unsigned cold);
void mod_envelope_release(ModEnvelope *e);
void mod_envelope_params(ModEnvelope *e,unsigned a,unsigned d,unsigned s,unsigned r);
#endif
