#pragma once
#include <stdint.h>
#define SYNTH_PARAM_COUNT 170
/* p163 reverb level, p164 room type (0..5), p165 decay, p166 signed
   modulation-wheel depth, p167 local reverb gate. Native IDs 94..97 map
   to p163..p166; the gate is FM1-only and never inferred from a preset. */
/* p160 chorus start, p161 EQ start, p162 pan start: OFF/LOW/MID/HIGH.
   New-phrase dispatch changes only the original integer phase accumulator. */
/* p152 delay level, p153 free time, p154 feedback, p155 native sync (0..19),
   p156 stereo width, p157 L/R ratio (0..12), p158 signed wheel depth,
   p159 local delay gate. Full 65536-float ring in large-buffer pool. */
/* p143 chorus level, p144 chorus/phaser type, p145 free rate, p146 sync,
   p147 depth, p148 signed centre, p149 signed feedback, p150 signed wheel
   depth, p151 local enable. Native initial phase is p160. */
/* p138 signed pan (64 center), p139 modulation depth, p140 free rate,
   p141 native sync index (0..34), p142 local pan enable.
   Native pan starting position is p162. */
/* p132 EQ depth (64 neutral), p133 frequency, p134 modulation depth,
   p135 free rate, p136 native sync index (0..34), p137 local EQ enable.
   Native EQ starting position is p161. */
/* p127: 0 existing FM1 allocation, 1 native MONO, 2 MONO AG, 3 POLY 1,
   4 POLY 2. Native POLY modes draw from one dynamically shared eight-voice physical pool. p168 is
   unison count 1..4 and p169 detune. The native arpeggiator was removed in TEST41. */
/* p128 distortion amount, p129 compensation, p130 wheel depth (64 neutral),
   p131 distortion enabled. Defaults bypass; full factory FX chain is unfinished. */
void synth_init(void);
void synth_note(unsigned note,unsigned velocity);
void synth_param(unsigned id,unsigned value);
void synth_panic(void);
void synth_pitch_bend(unsigned value14);
void synth_sustain(unsigned down);
/* source: 0 wheel (CC1), 1 breath (CC2), 2 channel pressure. */
void synth_controller(unsigned source,unsigned value);
void synth_reset_controllers(void);
/* Mono diagnostic output folds active pan to mono. Hardware uses stereo. */
void synth_render(float *out,unsigned n);
/* Non-overlapping planar buffers, each with n samples. */
void synth_render_stereo(float *left,float *right,unsigned n);
extern const unsigned synth_defaults[SYNTH_PARAM_COUNT];
extern uint32_t synth_faults;

/* TEST26 diagnostic timing. The clock callback returns monotonic microseconds.
   Profiling is disabled when NULL, so host/audio equivalence tests retain the
   normal path. Stages accumulate across both 128-frame slices in one 256-frame
   hardware audio half. */
typedef uint32_t (*synth_profile_clock_fn)(void);
enum {
 SYNTH_PROF_VOICES, SYNTH_PROF_DISTORT, SYNTH_PROF_AMP, SYNTH_PROF_EQ,
 SYNTH_PROF_PAN, SYNTH_PROF_DELAY, SYNTH_PROF_CHORUS,
 SYNTH_PROF_RVB_SETUP, SYNTH_PROF_RVB_MIX, SYNTH_PROF_MIDI_LEVEL,
 SYNTH_PROF_STAGE_COUNT
};
typedef struct { uint32_t max_us[SYNTH_PROF_STAGE_COUNT]; } SynthProfile;
void synth_profile_config(synth_profile_clock_fn clock_fn);
void synth_profile_reset(void);
void synth_profile_block_begin(void);
void synth_profile_block_end(void);
void synth_profile_read(SynthProfile *out);

unsigned synth_accept_channel(unsigned channel);
unsigned synth_voice_count(void);
unsigned synth_note_overflows(void);
/* Runtime-only native POLY 1/2 pool cap. Factory presets do not store it. */
void synth_native_pool_limit_set(unsigned voices);
unsigned synth_native_pool_limit(void);

/* Clock source: 0 internal, 1 USB, 2 TRS. Timestamps are receipt milliseconds. */
void synth_clock_event(unsigned status,unsigned source,uint32_t ms);
void synth_clock_poll(uint32_t ms);

void synth_note_pressure(unsigned note,unsigned value);

void synth_midi_level(unsigned cc,unsigned value);
void synth_all_notes_off(void);

/* Optional polyphonic render worker. Configure only while audio is stopped,
   normally immediately after synth_init(). NULL hooks retain serial operation.
   submit returns 1 only if it accepted exactly one job; 0 means no job ran.
   wait must join that job. A zero result MUST mean the worker is quiescent
   (joined or physically halted), because its buffers/state may be reset next.
   State-changing synth calls remain on the audio owner between render calls.
   A failed wait latches serial fallback until synth_init(), not until panic. */
void synth_worker_config(int (*submit)(void (*)(void *), void *), int (*wait)(void));
unsigned synth_worker_failures(void);
