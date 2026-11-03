#pragma once
#include <stdint.h>
#include "native_chorus.h"
#include "native_eq.h"
#include "native_pan.h"
/* Native phase selectors: OFF, LOW, MID, HIGH. No sample-loop processing.
   The note-count shadow follows native distinct-press membership independently
   of the FM1 voice cap and sustain. Its 128-entry capacity matches the bounded
   native-mono adapter, not an assertion about original pool exhaustion. */
typedef struct {
 uint8_t initial[3],pending;
 uint8_t held[128];
 uint32_t count;
} NativeFxPhase;
void native_fx_phase_init(NativeFxPhase *s);
/* Panic/Init policy: clear pending input and membership, retain selectors. */
void native_fx_phase_clear(NativeFxPhase *s);
void native_fx_phase_set(NativeFxPhase *s,unsigned effect,unsigned value);
/* External key input arms when the synthesized-note list is empty. Kept
   separate from dispatch so a deferred/generated-note path can postpone the reset until a generated
   note. Generated notes alone MUST NOT repeatedly re-arm it. */
void native_fx_phase_input(NativeFxPhase *s);
void native_fx_phase_note_on(NativeFxPhase *s,unsigned note);
void native_fx_phase_note_off(NativeFxPhase *s,unsigned note);
/* Consumes the pending request even when all selectors are OFF. Only integer
   accumulators change: cached float phase, fade, filters and tails survive. */
void native_fx_phase_dispatch(NativeFxPhase *s,NativeChorus *c,NativeEq *e,NativePan *p);
