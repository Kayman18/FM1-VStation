#include "synth.h"
#include "amp_level.h"
#include "shared_lfo.h"
#include "voice_internal.h"
#include "midi_clock.h"
#include "midi_level.h"
#include "distortion.h"
#include "native_eq.h"
#include "native_pan.h"
#include "native_chorus.h"
#include "native_delay.h"
#include "native_reverb.h"
#include "native_fx_phase.h"
#include "native_unison.h"

/* TEST26: low-overhead target profiling. A caller brackets one complete
   hardware audio half with block_begin/end; render calls inside that half add
   stage time. With no clock callback (normal host tests), this is inert. */
static synth_profile_clock_fn profile_clock;
static uint32_t profile_accum[SYNTH_PROF_STAGE_COUNT];
static uint32_t profile_max[SYNTH_PROF_STAGE_COUNT];
static unsigned profile_active;
static uint32_t profile_stamp(void){return profile_active && profile_clock ? profile_clock() : 0;}
static void profile_add(unsigned stage,uint32_t start){
 if(profile_active && profile_clock && stage<SYNTH_PROF_STAGE_COUNT)profile_accum[stage]+=profile_clock()-start;
}
void synth_profile_config(synth_profile_clock_fn clock_fn){profile_clock=clock_fn;synth_profile_reset();}
void synth_profile_reset(void){for(unsigned i=0;i<SYNTH_PROF_STAGE_COUNT;i++)profile_accum[i]=profile_max[i]=0;profile_active=0;}
void synth_profile_block_begin(void){for(unsigned i=0;i<SYNTH_PROF_STAGE_COUNT;i++)profile_accum[i]=0;profile_active=profile_clock!=0;}
void synth_profile_block_end(void){if(!profile_active)return;profile_active=0;for(unsigned i=0;i<SYNTH_PROF_STAGE_COUNT;i++)if(profile_accum[i]>profile_max[i])profile_max[i]=profile_accum[i];}
void synth_profile_read(SynthProfile *out){if(!out)return;for(unsigned i=0;i<SYNTH_PROF_STAGE_COUNT;i++)out->max_us[i]=profile_max[i];}
static NativeFxPhase fx_phase;
#ifndef PLAT_HOST
static float delay_memory[NATIVE_DELAY_SAMPLES] __attribute__((section(".pool")));
#else
static float delay_memory[NATIVE_DELAY_SAMPLES];
#endif
static NativeDelay delay;
static unsigned delay_enabled;
#ifndef PLAT_HOST
#define RVSEC(name) __attribute__((section(name)))
#else
#define RVSEC(name)
#endif
/* Exact compact native reverb history.  The desktop object stores the twelve
   lines in one sparse 65536-float ring; independent rings are equivalent and
   let the FM1 place them across otherwise-unused writable SRAM regions. */
static float rv0[1904] RVSEC(".reverb_debug");
static float rv1[1431] RVSEC(".pool");
static float rv2[1075]; /* Compact reverb history segment; placement may be adjusted for target RAM headroom. */
static float rv3[807];
static float rv4[613] RVSEC(".reverb_noinit");
static float rv5[471];
static float rv6[1464] RVSEC(".pool");
static float rv7[3808] RVSEC(".reverb_rt");
static float rv8[5859] RVSEC(".pool");
static float rv9[3662];
static float rv10[1611] RVSEC(".pool");
static float rv11[5830] RVSEC(".pool");
static float *const reverb_history[NATIVE_REVERB_LINES]={rv0,rv1,rv2,rv3,rv4,rv5,rv6,rv7,rv8,rv9,rv10,rv11};
static NativeReverb reverb;
static unsigned reverb_enabled;
static float reverb_left[128] __attribute__((section(".pool"),aligned(64)));
static float reverb_right[128] RVSEC(".reverb_debug") __attribute__((aligned(64)));
static NativeChorus chorus;
static unsigned chorus_enabled;
static NativePan pan;
static unsigned pan_enabled;
static NativeEq eq;
static unsigned eq_enabled,internal_bpm;
static void eq_apply(float *out,unsigned n){if(eq_enabled)native_eq_process(&eq,out,n);}
static Distortion distortion;
static unsigned distortion_enabled;
static void distortion_apply(float *out,unsigned n){
 if(!distortion_enabled)return;
 for(unsigned i=0;i<n;i++)out[i]=distortion_step(&distortion,out[i]);
}
static void distortion_reset(void){distortion_wheel(&distortion,0.f);distortion.smoothed=0.f;}
static void profiled_distortion(float *out,unsigned n){uint32_t t=profile_stamp();distortion_apply(out,n);profile_add(SYNTH_PROF_DISTORT,t);}
static void profiled_amp(float *out,unsigned n){uint32_t t=profile_stamp();amp_level_apply(out,n);profile_add(SYNTH_PROF_AMP,t);}
static void profiled_eq(float *out,unsigned n){uint32_t t=profile_stamp();eq_apply(out,n);profile_add(SYNTH_PROF_EQ,t);}
static MidiLevel midi_level;
#include <string.h>
const unsigned synth_defaults[SYNTH_PARAM_COUNT]={2,2,3,90,30,20,71,30,20,45,90,40,0,64,52,1,64,64,64,0,64,0,0,0,0,0,64,0,0,0,0,20,45,90,40,64,64,64,64,64,64,64,2,0,0,0,64,20,45,64,64,64,0,1,64,64,64,64,2,2,2,64,1,1,1,1,0,1,64,0,0,0,0,0,64,64,64,64,64,64,64,64,64,64,0,64,64,64,64,64,64,0,0,0,0,3,0,120,0,0,0,0,0,64,64,64,0,0,64,64,64,0,0,0,0,64,1,0,0,0,64,0,0,0,0,0,0,0,0,127,64,0,64,78,0,0,0,0,64,0,0,0,0,0,0,0,0,0,64,64,64,0,0,64,0,0,127,0,64,0,0,0,0,0,0,64,64,0,1,0};/* RC: eight physical native voices in one dynamically shared pool.
   Legacy FM1 mode retains its three-voice policy. Immutable wave/rate tables
   are shared, all summing remains in original order, and voice stealing fades
   the old signal for 64 samples before replacing state. */
enum { MAX_VOICES=8 };
typedef struct {unsigned note,key,pending,velocity,pressure,fade_out,fade_in,stack_index;uint32_t age;} Slot;
typedef struct {unsigned used,key,note,velocity,mask;uint32_t age;} NativeGroup;
#ifndef PLAT_HOST
#define NATIVE_STATE_NOINIT __attribute__((section(".noinit")))
#else
#define NATIVE_STATE_NOINIT
#endif
static Slot slots[MAX_VOICES] NATIVE_STATE_NOINIT;
static NativeGroup groups[MAX_VOICES] NATIVE_STATE_NOINIT;
static unsigned limit=3,native_pool_limit=8,note_mode,unison_count=1,unison_detune,channel,pedal,bend=8192,controls[3];static uint32_t serial;
static unsigned effective_unison_count(void){
 unsigned cap=native_pool_limit>MAX_VOICES?MAX_VOICES:native_pool_limit;
 return unison_count>cap?cap:unison_count;
}
uint32_t synth_faults;
static void (*const init_v[MAX_VOICES])(void)={v0_init,v1_init,v2_init,v3_init,v4_init,v5_init,v6_init,v7_init};
static void (*const panic_v[MAX_VOICES])(void)={v0_panic,v1_panic,v2_panic,v3_panic,v4_panic,v5_panic,v6_panic,v7_panic};
static void (*const note_v[MAX_VOICES])(unsigned,unsigned)={v0_note,v1_note,v2_note,v3_note,v4_note,v5_note,v6_note,v7_note};
static void (*const pressure_v[MAX_VOICES])(unsigned,unsigned)={v0_note_pressure,v1_note_pressure,v2_note_pressure,v3_note_pressure,v4_note_pressure,v5_note_pressure,v6_note_pressure,v7_note_pressure};
static void (*const param_v[MAX_VOICES])(unsigned,unsigned)={v0_param,v1_param,v2_param,v3_param,v4_param,v5_param,v6_param,v7_param};
static void (*const bend_v[MAX_VOICES])(unsigned)={v0_pitch_bend,v1_pitch_bend,v2_pitch_bend,v3_pitch_bend,v4_pitch_bend,v5_pitch_bend,v6_pitch_bend,v7_pitch_bend};
static void (*const pedal_v[MAX_VOICES])(unsigned)={v0_sustain,v1_sustain,v2_sustain,v3_sustain,v4_sustain,v5_sustain,v6_sustain,v7_sustain};
static void (*const control_v[MAX_VOICES])(unsigned,unsigned)={v0_controller,v1_controller,v2_controller,v3_controller,v4_controller,v5_controller,v6_controller,v7_controller};
static void (*const reset_v[MAX_VOICES])(void)={v0_reset_controllers,v1_reset_controllers,v2_reset_controllers,v3_reset_controllers,v4_reset_controllers,v5_reset_controllers,v6_reset_controllers,v7_reset_controllers};
static void (*const render_v[MAX_VOICES])(float*,unsigned)={v0_render,v1_render,v2_render,v3_render,v4_render,v5_render,v6_render,v7_render};
static unsigned (*const active_v[MAX_VOICES])(void)={v0_active,v1_active,v2_active,v3_active,v4_active,v5_active,v6_active,v7_active};
static void (*const alloff_v[MAX_VOICES])(void)={v0_all_notes_off,v1_all_notes_off,v2_all_notes_off,v3_all_notes_off,v4_all_notes_off,v5_all_notes_off,v6_all_notes_off,v7_all_notes_off};
static void (*const unison_v[MAX_VOICES])(float)={v0_unison_pitch,v1_unison_pitch,v2_unison_pitch,v3_unison_pitch,v4_unison_pitch,v5_unison_pitch,v6_unison_pitch,v7_unison_pitch};
static void (*const clock_v[MAX_VOICES])(unsigned)={v0_clock_rate,v1_clock_rate,v2_clock_rate,v3_clock_rate,v4_clock_rate,v5_clock_rate,v6_clock_rate,v7_clock_rate};
static void (*const transport_v[MAX_VOICES])(void)={v0_transport_start,v1_transport_start,v2_transport_start,v3_transport_start,v4_transport_start,v5_transport_start,v6_transport_start,v7_transport_start};
static void (*const shared_refresh_v[MAX_VOICES])(void)={v0_shared_lfo_refresh,v1_shared_lfo_refresh,v2_shared_lfo_refresh,v3_shared_lfo_refresh,v4_shared_lfo_refresh,v5_shared_lfo_refresh,v6_shared_lfo_refresh,v7_shared_lfo_refresh};
static uint32_t *const fault_v[MAX_VOICES]={&v0_faults,&v1_faults,&v2_faults,&v3_faults,&v4_faults,&v5_faults,&v6_faults,&v7_faults};
static unsigned (*const overflow_v[MAX_VOICES])(void)={v0_note_overflows,v1_note_overflows,v2_note_overflows,v3_note_overflows,v4_note_overflows,v5_note_overflows,v6_note_overflows,v7_note_overflows};
/* One owner, one in-flight job. CPU1 never writes the owner's stack. The
   descriptor is published by the platform's submit barrier, and the output is
   consumed only after wait's acquire barrier. Shared LFO state stays immutable
   for the entire slice; only the owner advances it after joining. */
enum { VOICE_RENDER_SLICE = 128 };
static struct {
 void (*render)(float *,unsigned);
 unsigned count;
 float samples[VOICE_RENDER_SLICE] __attribute__((aligned(64)));
} voice_job __attribute__((aligned(64)));
static int (*worker_submit)(void (*)(void *),void *);
static int (*worker_wait)(void);
static unsigned worker_failures;
static float native_voice_buffer[MAX_VOICES][VOICE_RENDER_SLICE] __attribute__((aligned(64)));
static struct {unsigned count,offset,nvoices,index[MAX_VOICES/2];} native_voice_job;
static uint32_t fault_sum(void){uint32_t n=worker_failures;for(unsigned i=0;i<MAX_VOICES;i++)n+=*fault_v[i];return n;}
static void render_worker_native(void *argument){(void)argument;for(unsigned j=0;j<native_voice_job.nvoices;j++){unsigned i=native_voice_job.index[j];render_v[i](native_voice_buffer[i]+native_voice_job.offset,native_voice_job.count);}}
void synth_worker_config(int (*submit)(void (*)(void *),void *),int (*wait)(void)){
 worker_submit=(submit && wait)?submit:0;worker_wait=(submit && wait)?wait:0;
}
unsigned synth_worker_failures(void){return worker_failures;}
static void render_worker_voice(void *argument){
 (void)argument;voice_job.render(voice_job.samples,voice_job.count);
}
static void render_worker_reverb(void *argument){
 (void)argument;
 native_reverb_process(&reverb,voice_job.samples,reverb_left,reverb_right,voice_job.count);
}

static MidiClock clock_state;
static unsigned clock_source,effective_bpm_milli;
static void shared_config(void){for(unsigned i=0;i<2;i++)shared_lfo_config(i,v0_private_lfo(i));}
static void clock_apply(void){
 unsigned bpm=clock_source && clock_state.locked?clock_state.bpm_milli:0;
 if(bpm==effective_bpm_milli)return;effective_bpm_milli=bpm;
 for(unsigned i=0;i<MAX_VOICES;i++)clock_v[i](bpm);shared_config();
 native_eq_tempo(&eq,bpm?bpm/1000.f:(float)internal_bpm);
 native_pan_tempo(&pan,bpm?bpm/1000.f:(float)internal_bpm);
 native_chorus_tempo(&chorus,bpm?bpm/1000.f:(float)internal_bpm);
 native_delay_tempo(&delay,bpm?bpm/1000.f:(float)internal_bpm);
}
void synth_clock_poll(uint32_t ms){if(!clock_source)return;midi_clock_poll(&clock_state,ms);clock_apply();}
void synth_clock_event(unsigned status,unsigned source,uint32_t ms){
 if(!clock_source || source!=clock_source)return;
 midi_clock_event(&clock_state,status,ms);clock_apply();
 if(status==0xfa)for(unsigned i=0;i<MAX_VOICES;i++)transport_v[i]();
}
static void clear_slots(void){memset(slots,0,sizeof(slots));memset(groups,0,sizeof(groups));for(unsigned i=0;i<MAX_VOICES;i++)slots[i].pressure=slots[i].note=slots[i].pending=128;serial=0;}
void synth_init(void){native_fx_phase_init(&fx_phase);worker_submit=0;worker_wait=0;worker_failures=0;native_delay_init(&delay,delay_memory);native_delay_time(&delay,64/127.f);delay_enabled=0;native_reverb_init(&reverb,reverb_history);native_reverb_decay_index(&reverb,64);reverb_enabled=0;native_chorus_init(&chorus);chorus_enabled=0;native_pan_init(&pan);pan_enabled=0;native_eq_init(&eq);eq_enabled=0;internal_bpm=120;distortion_init(&distortion);distortion_enabled=0;amp_level_init();shared_lfo_init();midi_level_init(&midi_level);midi_clock_reset(&clock_state);clock_source=0;effective_bpm_milli=0;for(unsigned i=0;i<MAX_VOICES;i++)init_v[i]();shared_config();clear_slots();limit=3;native_pool_limit=8;note_mode=0;unison_count=1;unison_detune=0;amp_level_unison(1);channel=0;pedal=0;bend=8192;memset(controls,0,sizeof(controls));synth_faults=0;}
void synth_panic(void){native_fx_phase_clear(&fx_phase);native_delay_reset(&delay);native_delay_wheel(&delay,0.f);native_reverb_reset(&reverb);native_reverb_wheel(&reverb,0.f);native_chorus_reset(&chorus);native_chorus_wheel(&chorus,0.f);native_pan_reset(&pan);native_eq_reset(&eq);distortion_reset();amp_level_reset();for(unsigned i=0;i<MAX_VOICES;i++)panic_v[i]();shared_lfo_panic();shared_config();clear_slots();pedal=0;bend=8192;memset(controls,0,sizeof(controls));}
void synth_param(unsigned id,unsigned value){
 if(id>=SYNTH_PARAM_COUNT)return;
 if(id>=168){
  if(id==168){unsigned next=value<1?1:value>MAX_VOICES?MAX_VOICES:value;if(next!=unison_count){synth_panic();unison_count=next;}amp_level_unison(note_mode?effective_unison_count():1);for(unsigned i=0;i<MAX_VOICES;i++)param_v[i](id,next);return;}
  if(value>127)value=127;unison_detune=value;for(unsigned i=0;i<MAX_VOICES;i++)param_v[i](id,value);for(unsigned i=0;i<MAX_VOICES;i++){unsigned pos=(note_mode==1||note_mode==2)?i:slots[i].stack_index;unison_v[i](native_unison_pitch(effective_unison_count(),unison_detune,pos));}return;
 }
 if(id>=163){
  if(id==167){reverb_enabled=!!value;return;}
  if(id==164){native_reverb_type(&reverb,value);return;}
  if(value>127)value=127;
  if(id==163)native_reverb_level(&reverb,value/127.f);
  else if(id==165)native_reverb_decay_index(&reverb,value);
  else native_reverb_wheel_depth(&reverb,((int)value-64)/128.f);
  return;
 }
 if(id>=160){native_fx_phase_set(&fx_phase,id-160,value);return;}
 if(id>=152){
  if(id==159){delay_enabled=!!value;return;}
  if(id==155){native_delay_sync(&delay,value);return;}
  if(id==157){native_delay_ratio(&delay,value);return;}
  if(value>127)value=127;
  if(id==152)native_delay_level(&delay,value/127.f);
  else if(id==153)native_delay_time(&delay,value/127.f);
  else if(id==154)native_delay_feedback(&delay,value/127.f);
  else if(id==156)native_delay_width(&delay,value/127.f);
  else native_delay_wheel_depth(&delay,((int)value-64)/128.f);
  return;
 }
 if(id>=143){
  if(id==151){chorus_enabled=!!value;return;}
  if(id==144){native_chorus_type(&chorus,value);return;}
  if(id==146){native_chorus_sync(&chorus,value);return;}
  if(value>127)value=127;
  if(id==143)native_chorus_level(&chorus,value/127.f);
  else if(id==145)native_chorus_rate(&chorus,value/127.f);
  else if(id==147)native_chorus_depth(&chorus,value/127.f);
  else if(id==148)native_chorus_centre(&chorus,((int)value-64)/128.f);
  else if(id==149)native_chorus_feedback(&chorus,((int)value-64)/128.f);
  else native_chorus_wheel_depth(&chorus,((int)value-64)/128.f);
  return;
 }
 if(id>=138){
  if(id==142){pan_enabled=!!value;return;}
  if(id==141){native_pan_sync(&pan,value);return;}
  if(value>127)value=127;
  if(id==138)native_pan_position(&pan,((int)value-64)/128.f);
  else if(id==139)native_pan_depth(&pan,value/127.f);
  else native_pan_rate(&pan,value/127.f);
  return;
 }
 if(id>=132){
  if(id==137){eq_enabled=!!value;return;}
  if(id==136){native_eq_sync(&eq,value);return;}
  if(value>127)value=127;
  if(id==132)native_eq_depth(&eq,((int)value-64)/128.f);
  else if(id==133)native_eq_frequency(&eq,value);
  else if(id==134)native_eq_modulation(&eq,value/127.f);
  else native_eq_rate(&eq,value/127.f);
  return;
 }
 if(id==97){internal_bpm=value<40?40:value>240?240:value;if(!effective_bpm_milli){native_eq_tempo(&eq,(float)internal_bpm);native_pan_tempo(&pan,(float)internal_bpm);native_chorus_tempo(&chorus,(float)internal_bpm);native_delay_tempo(&delay,(float)internal_bpm);}}
 if(id>=128){
  if(id==131){distortion_enabled=!!value;return;}
  if(value>127)value=127;
  if(id==128)distortion_amount(&distortion,value/127.f);
  else if(id==129)distortion_compensation(&distortion,value/127.f);
  else distortion_depth(&distortion,((int)value-64)/128.f);
  return;
 }
 if(id==127){if(value>4)value=4;if(value!=note_mode){synth_panic();note_mode=value;}for(unsigned i=0;i<MAX_VOICES;i++)param_v[i](id,value);amp_level_unison(note_mode?effective_unison_count():1);for(unsigned i=0;i<MAX_VOICES;i++){unsigned pos=(note_mode==1||note_mode==2)?i:slots[i].stack_index;unison_v[i](native_unison_pitch(effective_unison_count(),unison_detune,pos));}return;}
 if(id==120 || id==121){amp_level_param(id,value);return;}
 if(id==100){if(value>2)value=2;if(value!=clock_source){clock_source=value;midi_clock_reset(&clock_state);clock_apply();}return;}
 if(id==95){unsigned next=value<1?1:value>3?3:value;if(next!=limit){synth_panic();limit=next;}return;}
 if(id==96){if(value>16)value=16;if(value!=channel){synth_panic();channel=value;}return;}
 for(unsigned i=0;i<MAX_VOICES;i++)param_v[i](id,value);
 if(id==113 || id==114){shared_lfo_enable(id-113,!!value,v0_private_lfo(id-113));for(unsigned i=0;i<MAX_VOICES;i++)shared_refresh_v[i]();}
 if(id==19 || id==20 || id==21 || id==25 || id==26 || id==27 || id==97 || id==98 || id==99)shared_config();
}
unsigned synth_accept_channel(unsigned c){return c<16 && (!channel || channel==c+1);}
void synth_native_pool_limit_set(unsigned voices){
 unsigned next=voices<1?1:voices>MAX_VOICES?MAX_VOICES:voices;
 if(next==native_pool_limit)return;
 synth_panic();native_pool_limit=next;
 amp_level_unison(note_mode?effective_unison_count():1);
 for(unsigned i=0;i<MAX_VOICES;i++){unsigned pos=(note_mode==1||note_mode==2)?i:slots[i].stack_index;unison_v[i](native_unison_pitch(effective_unison_count(),unison_detune,pos));}
}
unsigned synth_native_pool_limit(void){return native_pool_limit;}
unsigned synth_voice_count(void){unsigned n=0,count=note_mode?MAX_VOICES:limit;for(unsigned i=0;i<count;i++)if(active_v[i]() || slots[i].pending<128)n++;return n;}
unsigned synth_note_overflows(void){unsigned n=0;for(unsigned i=0;i<MAX_VOICES;i++)n+=overflow_v[i]();return n;}
void synth_pitch_bend(unsigned v){bend=v>16383?16383:v;for(unsigned i=0;i<MAX_VOICES;i++)bend_v[i](bend);}
void synth_controller(unsigned source,unsigned value){if(source>=3)return;controls[source]=value>127?127:value;if(source==0){distortion_wheel(&distortion,controls[0]/127.f);native_chorus_wheel(&chorus,controls[0]/127.f);native_delay_wheel(&delay,controls[0]/127.f);native_reverb_wheel(&reverb,controls[0]/127.f);}for(unsigned i=0;i<MAX_VOICES;i++)control_v[i](source,controls[source]);}
void synth_sustain(unsigned down){pedal=!!down;for(unsigned i=0;i<MAX_VOICES;i++)pedal_v[i](pedal);
 if(!note_mode && !pedal)for(unsigned i=0;i<3;i++)if(!slots[i].key && slots[i].pending<128){slots[i].pending=128;slots[i].note=128;}
}
void synth_reset_controllers(void){native_delay_wheel(&delay,0.f);native_reverb_wheel(&reverb,0.f);native_chorus_wheel(&chorus,0.f);distortion_wheel(&distortion,0.f);midi_level_set(&midi_level,11,127);for(unsigned i=0;i<MAX_VOICES;i++)slots[i].pressure=128;memset(controls,0,sizeof(controls));pedal=0;bend=8192;for(unsigned i=0;i<MAX_VOICES;i++)reset_v[i]();
 if(!note_mode)for(unsigned i=0;i<3;i++)if(!slots[i].key && slots[i].pending<128){slots[i].pending=128;slots[i].note=128;}
}
static void replace_voice(unsigned i){
 Slot *s=&slots[i];unsigned note=s->pending,velocity=s->velocity;s->pending=128;
 panic_v[i]();unison_v[i](native_unison_pitch(note_mode?effective_unison_count():1,unison_detune,s->stack_index));bend_v[i](bend);pedal_v[i](pedal);for(unsigned c=0;c<3;c++)control_v[i](c,controls[c]);
 if(note<128){note_v[i](note,velocity);if(s->pressure<128)pressure_v[i](note,s->pressure);if(!s->key)note_v[i](note,0);s->fade_in=64;}else s->note=128;
}
static void legacy_note(unsigned note,unsigned velocity){
 if(note>127)return;if(velocity>127)velocity=127;
 if(limit==1){v0_note(note,velocity);return;}
 if(!velocity){for(unsigned i=0;i<limit;i++)if(slots[i].note==note){Slot *s=&slots[i];s->key=0;
   if(s->fade_out){if(!pedal){s->pending=128;s->note=128;}}
   else note_v[i](note,0);
  }return;}
 /* Repeated note reuses its slot; note-off cannot release an unrelated steal. */
 for(unsigned i=0;i<limit;i++)if(slots[i].note==note){Slot *s=&slots[i];s->key=1;s->pressure=128;s->age=++serial;
  if(s->fade_out){s->pending=note;s->velocity=velocity;}else note_v[i](note,velocity);return;
 }
 unsigned chosen=limit;
 for(unsigned i=0;i<limit;i++)if(!slots[i].fade_out && !active_v[i]()){chosen=i;break;}
 if(chosen==limit){
  for(unsigned i=0;i<limit;i++)if(!slots[i].key && !pedal && (chosen==limit || slots[i].age<slots[chosen].age))chosen=i;
  if(chosen==limit){chosen=0;for(unsigned i=1;i<limit;i++)if(slots[i].age<slots[chosen].age)chosen=i;}
 }
 Slot *s=&slots[chosen];unsigned old=s->note;unsigned busy=active_v[chosen]() || s->fade_out;
 s->note=note;s->key=1;s->pressure=128;s->age=++serial;
 if(busy){s->pending=note;s->velocity=velocity;
  if(!s->fade_out){pedal_v[chosen](0);if(old<128)note_v[chosen](old,0);s->fade_out=64;}
 }else{note_v[chosen](note,velocity);s->fade_in=64;}
}
static unsigned group_mask(void){unsigned m=0;for(unsigned g=0;g<MAX_VOICES;g++)if(groups[g].used)m|=groups[g].mask;return m;}
static void cleanup_groups(void){
 for(unsigned g=0;g<MAX_VOICES;g++)if(groups[g].used && !groups[g].key){unsigned alive=0;
  for(unsigned i=0;i<MAX_VOICES;i++)if((groups[g].mask&(1u<<i)) && (active_v[i]() || slots[i].pending<128 || slots[i].fade_out)){alive=1;break;}
  if(!alive){unsigned mask=groups[g].mask;groups[g].used=0;for(unsigned i=0;i<MAX_VOICES;i++)if(mask&(1u<<i)){slots[i].note=slots[i].pending=128;slots[i].key=0;slots[i].pressure=128;}}
 }
}
static int choose_group_slot(void){for(unsigned g=0;g<MAX_VOICES;g++)if(!groups[g].used)return (int)g;return -1;}
static int oldest_group(unsigned released_first){int best=-1;for(unsigned g=0;g<MAX_VOICES;g++)if(groups[g].used && (!released_first || !groups[g].key)){if(best<0 || groups[g].age<groups[best].age)best=(int)g;}return best;}
static void native_poly_note(unsigned note,unsigned velocity){
 cleanup_groups();
 if(!velocity){int best=-1;for(unsigned g=0;g<MAX_VOICES;g++)if(groups[g].used && groups[g].key && groups[g].note==note && (best<0 || groups[g].age<groups[best].age))best=(int)g;
  if(best<0)return;NativeGroup *ng=&groups[best];ng->key=0;for(unsigned i=0;i<MAX_VOICES;i++)if(ng->mask&(1u<<i)){Slot *sl=&slots[i];sl->key=0;if(sl->fade_out && sl->pending==note){if(!pedal){sl->pending=128;sl->note=128;}}else note_v[i](note,0);}return;
 }
 unsigned stack_count=effective_unison_count(),need=stack_count,used=group_mask(),selected=0;
 for(unsigned i=0;i<native_pool_limit && need;i++)if(!(used&(1u<<i)) && !active_v[i]()){selected|=1u<<i;--need;}
 unsigned victim_note=128;
 if(need){int victim=oldest_group(1);if(victim<0)victim=oldest_group(0);if(victim<0)return;selected=groups[victim].mask;victim_note=groups[victim].note;groups[victim].used=0;need=0;}
 int gi=choose_group_slot();if(gi<0)return;NativeGroup *ng=&groups[gi];ng->used=ng->key=1;ng->note=note;ng->velocity=velocity;ng->mask=selected;ng->age=++serial;
 unsigned stack=0;for(unsigned i=0;i<MAX_VOICES;i++)if(selected&(1u<<i)){Slot *sl=&slots[i];unsigned busy=active_v[i]() || sl->fade_out;sl->note=note;sl->key=1;sl->pressure=128;sl->age=ng->age;sl->velocity=velocity;sl->stack_index=stack;unison_v[i](native_unison_pitch(stack_count,unison_detune,stack++));
  if(busy){sl->pending=note;if(!sl->fade_out){pedal_v[i](0);if(victim_note<128)note_v[i](victim_note,0);sl->fade_out=64;}}
  else{note_v[i](note,velocity);sl->fade_in=64;}
 }
}
static void direct_note(unsigned note,unsigned velocity){
 if(note>127)return;if(velocity>127)velocity=127;
 if(velocity){native_fx_phase_input(&fx_phase);native_fx_phase_note_on(&fx_phase,note);native_fx_phase_dispatch(&fx_phase,&chorus,&eq,&pan);}else native_fx_phase_note_off(&fx_phase,note);
 if(note_mode==1 || note_mode==2){unsigned count=effective_unison_count();for(unsigned i=0;i<count;i++){unison_v[i](native_unison_pitch(count,unison_detune,i));note_v[i](note,velocity);}return;}
 if(note_mode>=3){native_poly_note(note,velocity);return;}
 legacy_note(note,velocity);
}
void synth_note(unsigned note,unsigned velocity){direct_note(note,velocity);}
void synth_note_pressure(unsigned note,unsigned value){
 if(note>127)return;if(value>127)value=127;
 if(note_mode==1 || note_mode==2){unsigned count=effective_unison_count();for(unsigned i=0;i<count;i++)pressure_v[i](note,value);return;}
 if(note_mode>=3){for(unsigned g=0;g<MAX_VOICES;g++)if(groups[g].used && groups[g].note==note)for(unsigned i=0;i<MAX_VOICES;i++)if(groups[g].mask&(1u<<i)){if(slots[i].fade_out && slots[i].pending==note)slots[i].pressure=value;else pressure_v[i](note,value);}return;}
 if(limit==1){v0_note_pressure(note,value);return;}
 for(unsigned i=0;i<3;i++)if(slots[i].note==note){if(slots[i].fade_out && slots[i].pending==note)slots[i].pressure=value;else pressure_v[i](note,value);}
}
void synth_midi_level(unsigned cc,unsigned value){midi_level_set(&midi_level,cc,value);}
void synth_all_notes_off(void){
 native_fx_phase_clear(&fx_phase);
 if(note_mode==1 || note_mode==2){unsigned count=effective_unison_count();for(unsigned i=0;i<count;i++)alloff_v[i]();return;}
 if(note_mode>=3){for(unsigned g=0;g<MAX_VOICES;g++)if(groups[g].used)groups[g].key=0;for(unsigned i=0;i<MAX_VOICES;i++){alloff_v[i]();slots[i].key=0;}return;}
 v0_all_notes_off();v1_all_notes_off();v2_all_notes_off();for(unsigned i=0;i<3;i++){slots[i].key=0;if(!pedal && slots[i].pending<128){slots[i].pending=slots[i].note=128;slots[i].pressure=128;}}
}
static unsigned native_mask(void){
 if(note_mode==1 || note_mode==2){unsigned count=effective_unison_count();return (1u<<count)-1u;}
 cleanup_groups();return group_mask();
}
static void native_refresh_shared(unsigned mask){for(unsigned i=0;i<MAX_VOICES;i++)if(mask&(1u<<i))shared_refresh_v[i]();}
static int render_native_pre(float *out,unsigned n){
 float *start=out;unsigned total=n;uint32_t voice_t=profile_stamp();
 while(n){unsigned count=n>VOICE_RENDER_SLICE?VOICE_RENDER_SLICE:n,mask=native_mask();
  if(note_mode>=3)for(unsigned i=0;i<MAX_VOICES;i++)if((mask&(1u<<i)) && slots[i].fade_out && slots[i].fade_out<count)count=slots[i].fade_out;
  if(shared_lfo_mask){unsigned boundary=shared_lfo_boundary();count=shared_lfo_slice(count);if(boundary)native_refresh_shared(mask);}
  unsigned list[MAX_VOICES],vc=0;for(unsigned i=0;i<MAX_VOICES;i++)if(mask&(1u<<i))list[vc++]=i;
  unsigned submitted=0;
  /* RC: retain one worker handoff per slice. The dynamically shared
     pool splits as evenly as possible across the two cores; POOL 8 is 4+4. */
  unsigned worker_n=0,cpu0_n=vc;
  if(vc>1 && worker_submit && !worker_failures){worker_n=(vc+1)/2;cpu0_n=vc-worker_n;native_voice_job.count=count;native_voice_job.offset=0;native_voice_job.nvoices=worker_n;for(unsigned j=0;j<worker_n;j++)native_voice_job.index[j]=list[cpu0_n+j];submitted=worker_submit(render_worker_native,0);if(!submitted){worker_n=0;cpu0_n=vc;}}
  for(unsigned j=0;j<cpu0_n;j++){unsigned i=list[j];render_v[i](native_voice_buffer[i],count);}
  if(submitted && !worker_wait()){++worker_failures;synth_panic();memset(start,0,total*sizeof(*start));synth_faults=fault_sum();profile_add(SYNTH_PROF_VOICES,voice_t);return 0;}
  if(shared_lfo_mask)shared_lfo_advance(count);
  /* OPT-B: when no voice is fading in this slice, sum active voice buffers in the
     same ascending order without per-sample mask/slot bookkeeping (x*1.f==x). */
  {const float *src[MAX_VOICES];unsigned sv=0,fading=0;
   for(unsigned i=0;i<MAX_VOICES;i++)if(mask&(1u<<i)){src[sv++]=native_voice_buffer[i];if(note_mode>=3 && (slots[i].fade_out || slots[i].fade_in))fading=1;}
   if(!fading){for(unsigned k=0;k<count;k++){float sum=0.f;for(unsigned j=0;j<sv;j++)sum+=src[j][k];out[k]=sum;}}
   else
  for(unsigned k=0;k<count;k++){float sum=0.f;for(unsigned i=0;i<MAX_VOICES;i++)if(mask&(1u<<i)){float g=1.f;if(note_mode>=3){Slot *sl=&slots[i];if(sl->fade_out)g=sl->fade_out--/64.f;if(sl->fade_in){g*=(64-sl->fade_in)/64.f;sl->fade_in--;}}sum+=native_voice_buffer[i][k]*g;}out[k]=sum;}}
  if(note_mode>=3){for(unsigned i=0;i<MAX_VOICES;i++)if((mask&(1u<<i)) && !slots[i].fade_out && slots[i].pending<128)replace_voice(i);for(unsigned i=0;i<MAX_VOICES;i++)if((mask&(1u<<i)) && !slots[i].fade_out && slots[i].note==128 && active_v[i]())replace_voice(i);cleanup_groups();}
  out+=count;n-=count;
 }
 profile_add(SYNTH_PROF_VOICES,voice_t);
 profiled_distortion(start,total);profiled_amp(start,total);profiled_eq(start,total);synth_faults=fault_sum();return 1;
}
static void render_pre_output_segment(float *out,unsigned n){
 float *start=out;unsigned total=n;
 if(note_mode){render_native_pre(out,n);return;}
 if(limit==1){uint32_t voice_t=profile_stamp();if(shared_lfo_mask){unsigned left=n;float *dst=out;while(left){unsigned boundary=shared_lfo_boundary();unsigned count=shared_lfo_slice(left);if(boundary)v0_shared_lfo_refresh();v0_render(dst,count);shared_lfo_advance(count);dst+=count;left-=count;}}else v0_render(out,n);profile_add(SYNTH_PROF_VOICES,voice_t);profiled_distortion(out,n);profiled_amp(out,n);profiled_eq(out,n);synth_faults=fault_sum();return;}
 uint32_t voice_t=profile_stamp();
 while(n){unsigned count=n>VOICE_RENDER_SLICE?VOICE_RENDER_SLICE:n;float voices[3][VOICE_RENDER_SLICE];
  for(unsigned i=0;i<limit;i++)if(slots[i].fade_out && slots[i].fade_out<count)count=slots[i].fade_out;
  if(shared_lfo_mask){unsigned boundary=shared_lfo_boundary();count=shared_lfo_slice(count);if(boundary){v0_shared_lfo_refresh();v1_shared_lfo_refresh();v2_shared_lfo_refresh();}}
  unsigned last=limit-1,submitted=0;
  float *voice_output[3]={voices[0],voices[1],voices[2]};
  if(worker_submit && !worker_failures){
   voice_job.render=render_v[last];voice_job.count=count;
   submitted=worker_submit(render_worker_voice,0);
  }
  for(unsigned i=0;i<(submitted?last:limit);i++)render_v[i](voices[i],count);
  if(submitted){
   if(!worker_wait()){
    ++worker_failures;synth_panic();memset(start,0,total*sizeof(*start));
    synth_faults=fault_sum();profile_add(SYNTH_PROF_VOICES,voice_t);return;
   }
   voice_output[last]=voice_job.samples;
  }
  if(shared_lfo_mask)shared_lfo_advance(count);
  for(unsigned k=0;k<count;k++){float sum=0;for(unsigned i=0;i<limit;i++){Slot *sl=&slots[i];float g=1;
    if(sl->fade_out)g=sl->fade_out--/64.f;
    if(sl->fade_in){g*=(64-sl->fade_in)/64.f;sl->fade_in--;}
    sum+=voice_output[i][k]*g;
   }out[k]=sum*(limit==2?.5f:1.f/3.f);
  }
  for(unsigned i=0;i<limit;i++)if(!slots[i].fade_out && slots[i].pending<128)replace_voice(i);
  for(unsigned i=0;i<limit;i++)if(!slots[i].fade_out && slots[i].note==128 && active_v[i]())replace_voice(i);
  out+=count;n-=count;
 }
 profile_add(SYNTH_PROF_VOICES,voice_t);
 profiled_distortion(start,total);profiled_amp(start,total);profiled_eq(start,total);
 synth_faults=fault_sum();
}
static void render_pre_output(float *out,unsigned n){render_pre_output_segment(out,n);}
void synth_render(float *out,unsigned n){
 render_pre_output(out,n);
 if(!reverb_enabled){
  if(delay_enabled)for(unsigned i=0;i<n;i++){
   float input=out[i],l=input,r=input,dl,dr,cl,cr;
   if(pan_enabled)native_pan_step(&pan,input,&l,&r);
   native_delay_step(&delay,input,&dl,&dr);l+=dl;r+=dr;
   if(chorus_enabled){native_chorus_step(&chorus,input,&cl,&cr);l=cl+l;r=cr+r;}
   out[i]=(l+r)*.5f;
  }
  else if(chorus_enabled)for(unsigned i=0;i<n;i++){
   float input=out[i],l=input,r=input,wet_l,wet_r;
   if(pan_enabled)native_pan_step(&pan,input,&l,&r);
   native_chorus_step(&chorus,input,&wet_l,&wet_r);
   l+=wet_l;r+=wet_r;out[i]=(l+r)*.5f;
  }
  else if(pan_enabled)for(unsigned i=0;i<n;i++){float l,r;native_pan_step(&pan,out[i],&l,&r);out[i]=(l+r)*.5f;}
  midi_level_apply(&midi_level,out,n);return;
 }
 /* Diagnostic mono path keeps the exact stereo wet arithmetic then folds it. */
 for(unsigned i=0;i<n;i++){
  float input=out[i],l=input,r=input,dl,dr,cl,cr,rl,rr;
  if(pan_enabled)native_pan_step(&pan,input,&l,&r);
  if(delay_enabled){native_delay_step(&delay,input,&dl,&dr);l+=dl;r+=dr;}
  if(chorus_enabled){native_chorus_step(&chorus,input,&cl,&cr);l=cl+l;r=cr+r;}
  native_reverb_step(&reverb,input,&rl,&rr);l+=rl;r+=rr;out[i]=(l+r)*.5f;
 }
 midi_level_apply(&midi_level,out,n);
}
void synth_render_stereo(float *left,float *right,unsigned n){
 float *start_left=left,*start_right=right;unsigned total=n;
 render_pre_output(left,n);
 if(!reverb_enabled){
  uint32_t route_t=profile_stamp();
  if(delay_enabled)for(unsigned i=0;i<n;i++){
   float input=left[i],dl,dr,cl,cr;
   if(pan_enabled)native_pan_step(&pan,input,left+i,right+i);else right[i]=input;
   native_delay_step(&delay,input,&dl,&dr);left[i]+=dl;right[i]+=dr;
   if(chorus_enabled){native_chorus_step(&chorus,input,&cl,&cr);left[i]=cl+left[i];right[i]=cr+right[i];}
  }
  else if(chorus_enabled)for(unsigned i=0;i<n;i++){
   float input=left[i],wet_l,wet_r;
   if(pan_enabled)native_pan_step(&pan,input,left+i,right+i);else right[i]=input;
   native_chorus_step(&chorus,input,&wet_l,&wet_r);left[i]+=wet_l;right[i]+=wet_r;
  }
  else if(pan_enabled){for(unsigned i=0;i<n;i++)native_pan_step(&pan,left[i],left+i,right+i);}
  else memcpy(right,left,n*sizeof(*right));
  profile_add(SYNTH_PROF_PAN,route_t);
  uint32_t midi_t=profile_stamp();midi_level_apply_stereo(&midi_level,left,right,n);profile_add(SYNTH_PROF_MIDI_LEVEL,midi_t);return;
 }
 unsigned offset=0;
 while(offset<n){
  unsigned count=n-offset>VOICE_RENDER_SLICE?VOICE_RENDER_SLICE:n-offset;
  uint32_t t=profile_stamp();for(unsigned i=0;i<count;i++)voice_job.samples[i]=left[offset+i];
  voice_job.count=count;unsigned submitted=0;
  if(worker_submit && !worker_failures)submitted=worker_submit(render_worker_reverb,0);
  if(!submitted)native_reverb_process(&reverb,voice_job.samples,reverb_left,reverb_right,count);
  profile_add(SYNTH_PROF_RVB_SETUP,t);
  float *l=left+offset,*r=right+offset;
  t=profile_stamp();if(pan_enabled){for(unsigned i=0;i<count;i++)native_pan_step(&pan,voice_job.samples[i],l+i,r+i);}
  else {memcpy(l,voice_job.samples,count*sizeof(*l));memcpy(r,voice_job.samples,count*sizeof(*r));}profile_add(SYNTH_PROF_PAN,t);
  t=profile_stamp();if(delay_enabled)native_delay_process_add(&delay,voice_job.samples,l,r,count);profile_add(SYNTH_PROF_DELAY,t);
  t=profile_stamp();if(chorus_enabled){if(note_mode>=3)native_chorus_process_add_half(&chorus,voice_job.samples,l,r,count);else native_chorus_process_add(&chorus,voice_job.samples,l,r,count);}profile_add(SYNTH_PROF_CHORUS,t);
  if(submitted && !worker_wait()){
   ++worker_failures;synth_panic();memset(start_left,0,total*sizeof(*start_left));memset(start_right,0,total*sizeof(*start_right));
   synth_faults=fault_sum();return;
  }
  t=profile_stamp();for(unsigned i=0;i<count;i++){l[i]+=reverb_left[i];r[i]+=reverb_right[i];}profile_add(SYNTH_PROF_RVB_MIX,t);
  offset+=count;
 }
 uint32_t midi_t=profile_stamp();midi_level_apply_stereo(&midi_level,left,right,n);profile_add(SYNTH_PROF_MIDI_LEVEL,midi_t);
}
#ifdef VSTATION_TEST
const NativeFxPhase *synth_test_fx_phase(void){return &fx_phase;}
const NativeDelay *synth_test_delay(void){return &delay;}
unsigned synth_test_delay_enabled(void){return delay_enabled;}
const NativeReverb *synth_test_reverb(void){return &reverb;}
unsigned synth_test_reverb_enabled(void){return reverb_enabled;}
const NativeChorus *synth_test_chorus(void){return &chorus;}
unsigned synth_test_chorus_enabled(void){return chorus_enabled;}
const NativePan *synth_test_pan(void){return &pan;}
unsigned synth_test_pan_enabled(void){return pan_enabled;}
const NativeEq *synth_test_eq(void){return &eq;}
unsigned synth_test_eq_enabled(void){return eq_enabled;}
const Distortion *synth_test_distortion(void){return &distortion;}
unsigned synth_test_distortion_enabled(void){return distortion_enabled;}
unsigned synth_test_owner(unsigned i){return i<MAX_VOICES?slots[i].note:128;}
unsigned synth_test_pending(unsigned i){return i<MAX_VOICES?slots[i].pending:128;}
#endif
