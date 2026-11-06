#include "synth.h"
#include "shared_lfo.h"
#include "amp_runtime.h"
#include "mod_envelope.h"
#include "osc_modulation.h"
#include "osc_fm.h"
#include "velocity.h"
#include "osc_tuning.h"
#include "osc_drift.h"
#include "portamento.h"
#include "osc_phase.h"
#include "lfo_sync.h"
#include "lfo_tempo.h"
#include "voice_render_common.h"
#include "filter_tracking.h"
#include "filter_modulation.h"
#include <string.h>
#include <math.h>
extern const uint32_t vst_wave_data[];
extern const float amp_attack_44100[128],amp_fall_44100[128];
float waveform(unsigned,uint32_t,uint32_t,float,const float*,const float*,const float*);
float voice_mix_float(float,float,float,const float*,float,float);
uint32_t pulse_offset(float);
void coefficients(float*);void portable4(float*,float);void portable(float*,float);
extern const float filter_table_44100[129];
void filter_control(int32_t,int32_t*,int32_t,const float*,float*);
void filter_resonance(float*,float);
void filter_character(float*,unsigned,unsigned,unsigned);
/* Diagnostic adapter: original waveform/filter/envelope/mix primitives;
   FM1 controls, note priority and oscillator phase policy are provisional. */

extern unsigned voice_params[SYNTH_PARAM_COUNT];
#define p voice_params
/* Note numbers and MIDI velocities are 7-bit values. Keeping these per-voice
   histories byte-sized saves 768 bytes per physical voice with no value loss. */
static uint8_t notes[128],vel[128];static unsigned held;
static float unison_pitch_units;
static unsigned return_gate_mask,native_released;
static uint32_t native_overflows;
#ifdef VSTATION_TEST
static unsigned applied_gate_mask;
#endif
static unsigned sustain_down;static uint8_t key_down[128];
static VoiceOsc osc[3];
static float drift_scale[3];
static ModEnvelope mod_env;static unsigned mod_tick;
static AmpRuntime amp,fm_env;static float filter[20],gains[5],current_gain,mix_scale;
static int32_t cutoff_target,cutoff_smoothed;
static int32_t filter_env_depth,filter_lfo_depth,pitch_lfo_depth[3],pitch_env_depth[3];
static unsigned controllers[3],channel_pressure,pressure_note=128;
static uint8_t note_pressure[128]; /* 128: use channel pressure. */
static float controller_pitch[3],controller_amp_target=1.f,controller_amp_gain=1.f;
/* Original 0x4593f/0x45c4d/0x45f3f: pressure + wheel + breath, clamp then +1. */
static void update_controller_amp(void){
 float pressure=(controllers[2]/127.f)*(((int)p[110]-64)/64.f);
 float wheel=(controllers[0]/127.f)*(((int)p[108]-64)/64.f);
 float breath=(controllers[1]/127.f)*(((int)p[109]-64)/64.f);
 float x=pressure+wheel;x+=breath;if(x< -1.f)x=-1.f;if(x>1.f)x=1.f;
 controller_amp_target=1.f+x;
}
static void tune(void);
static unsigned native_mono_mode(void){return p[127]==1 || p[127]==2;}
void voice_unison_pitch(float units){unison_pitch_units=units;tune();}
static void update_controller_pitch(void){
 for(unsigned i=0;i<3;i++)controller_pitch[i]=osc_controller_pitch(controllers[i],(float)((int)p[78+i]-64));
}
static void update_filter_lfo_depth(void){
 if(!p[73]){filter_lfo_depth=(int32_t)(p[22]*128);return;}
 float offset[3];for(unsigned i=0;i<3;i++)offset[i]=filter_controller_offset(controllers[i],((int)p[81+i]-64)/64.f);
 filter_lfo_depth=filter_mod_depth_offsets(((int)p[74]-64)/128.f,offset[0],offset[1],offset[2]);
}
static void update_pitch_lfo_depth(void){
 if(!p[84]){for(unsigned i=0;i<3;i++)pitch_lfo_depth[i]=(int32_t)(p[24]*1536/127);return;}
 float offset[3];for(unsigned i=0;i<3;i++)offset[i]=filter_controller_offset(controllers[i],((int)p[86+i]-64)/64.f);
 /* Pitch and filter use the same original signed depth curve. */
 for(unsigned i=0;i<3;i++)pitch_lfo_depth[i]=filter_mod_depth_offsets(((int)p[i?88+i:85]-64)/128.f,offset[0],offset[1],offset[2]);
}
/* Original 0x46460 uses the same signed depth table as filter modulation.
   Cache per event; legacy mode keeps the original FM1 linear range. */
static void update_pitch_env_depth(void){
 for(unsigned i=0;i<3;i++)pitch_env_depth[i]=p[101]?
  filter_mod_depth(((int)p[36+i]-64)/128.f):((int32_t)p[36+i]-64)*24;
}
static unsigned filter_note; /* Retain last gated key through the release tail. */
static void update_cutoff_target(void){
 float offset[3];for(unsigned i=0;i<3;i++)offset[i]=filter_controller_offset(controllers[i],(float)((int)p[75+i]-64));
 cutoff_target=filter_tracking_offsets(p[6]/127.f,filter_note,p[72]/127.f,offset[0],offset[1],offset[2]);
}
static void apply_controller(unsigned source,unsigned value){
 if(source>=3)return;if(value>127)value=127;
 if(controllers[source]==value)return;controllers[source]=value;update_controller_amp();update_cutoff_target();update_controller_pitch();update_filter_lfo_depth();update_pitch_lfo_depth();tune();
}
static void apply_pressure(void){
 unsigned value=pressure_note<128 && note_pressure[pressure_note]<128?note_pressure[pressure_note]:channel_pressure;
 apply_controller(2,value);
}
void synth_controller(unsigned source,unsigned value){
 if(source==2){channel_pressure=value>127?127:value;apply_pressure();}
 else apply_controller(source,value);
}
void synth_note_pressure(unsigned note,unsigned value){
 if(note>127)return;
 for(unsigned i=0;i<held;i++)if(notes[i]==note){note_pressure[note]=value>127?127:value;if(pressure_note==note)apply_pressure();return;}
}
static uint32_t lfo[2][14],noise_seed,width_mod[3];
static int32_t width_env_depth[3],width_lfo_depth[3];
float noise_step(uint32_t*);
static float fm_depth,fm_amount;
static unsigned velocity_native_active;
static Portamento glide;static unsigned glide_note,glide_valid;
static int32_t bend_offsets[3];
static int32_t bend_value; /* signed MIDI displacement from 8192 */
extern const uint32_t lfo_rate_44100[128],lfo_fade_44100[128];
void lfo_step(uint32_t*);
static unsigned clock_bpm_milli;
static void update_lfo_rates(void){
 if(clock_bpm_milli){for(unsigned i=0;i<2;i++)lfo[i][4]=lfo_tempo_increment_milli(clock_bpm_milli,p[98+i],lfo_rate_44100[p[i?26:20]]);return;}
 lfo[0][4]=lfo_tempo_increment(p[97],p[98],lfo_rate_44100[p[20]]);
 lfo[1][4]=lfo_tempo_increment(p[97],p[99],lfo_rate_44100[p[26]]);
}
static uint32_t *active_lfo(unsigned i){return shared_lfo_mask&(1u<<i)?shared_lfo_state[i]:lfo[i];}
static int32_t lfo_value(unsigned source){int32_t v;memcpy(&v,&active_lfo(source)[1],4);return v;}
uint32_t synth_faults;
static const uint8_t *data(void){return (const uint8_t*)vst_wave_data;}
static uint32_t u32(unsigned off){return vst_wave_data[off/4];}
static float f32(unsigned off){float x;memcpy(&x,data()+off,4);return x;}
static void update_bend_offsets(void){
 for(unsigned i=0;i<3;i++){
  int range=p[117+i]?(int)p[117+i]-13:(int)p[42];
  bend_offsets[i]=bend_value*range*1024;
 }
}
static int32_t pitch_offset(unsigned i){return bend_offsets[i]+glide.offset;}
/* TEST28: pitch sources update at control rate. Keep the exact original
   arithmetic, but only retune oscillators whose pitch source changed. The
   wavetable frame/blend depends on semitone + wave, not the fine 1/128-step
   increment index, so do not re-read those XIP tables while modulation stays
   inside the same semitone. */
static unsigned pitch_lfo_mask(void){unsigned m=0;for(unsigned i=0;i<3;i++)if(pitch_lfo_depth[i])m|=1u<<i;return m;}
static unsigned pitch_env_mask(void){unsigned m=0;for(unsigned i=0;i<3;i++)if(pitch_env_depth[i])m|=1u<<i;return m;}
/* OPT-D: in POLY modes (p127>=3) voice_render_common.c never reads the waveform
   of an oscillator whose mix gain is 0, and that oscillator feeds nothing else:
     OSC3 (idx 2): nothing                        -> idle when gains[2]==0
     OSC2 (idx 1): ring (gains[4]) and FM into OSC3 -> idle when gains[1]==0, no ring, no FM
     OSC1 (idx 0): ring, and hard-sync of OSC2       -> idle when gains[0]==0, no ring, no sync
   Control-rate retunes of an idle oscillator are wasted, so the render path skips
   them.  tune() still retunes all three, and an oscillator that becomes audible
   again is retuned immediately.  Consequence: while idle its phase advances at a
   stale increment, so if its level is raised mid-note it resumes at a different
   (still arbitrary, free-running) phase than before this change; pitch is correct.
   Output is bit-identical whenever the idle set does not change during a note. */
static unsigned osc_idle;
static unsigned panic_replay,derived_ready;
static void tune_mask_full(unsigned mask){
 /* Last gated note also anchors controller/tuning changes during release. */
 unsigned note=glide_valid?glide_note:48;
 for(unsigned i=0;i<3;i++){
  if(!(mask&(1u<<i)))continue;
  int nn=(int)note+(i?(int)p[12+i]-64:(int)p[61]-64)+((int)p[58+i]-2)*12;if(nn<0)nn=0;if(nn>126)nn=126;
  int32_t pm=osc_pitch_mod(mod_env.word[1],mod_env.word[3],pitch_env_depth[i],lfo_value(p[30]),pitch_lfo_depth[i],pitch_offset(i));
  /* Retain the existing coarse-note limits; fine/master cents use recovered
     addition order before the final table clamp. */
  float base_pitch=osc_base_pitch_float((unsigned)nn,0,0,(int)p[54+i]-64,(int)p[57]-64,controller_pitch[0],controller_pitch[1],controller_pitch[2]);
  base_pitch+=unison_pitch_units;
  int32_t drift=osc_drift_increment(base_pitch,drift_scale[i]);
  int index=(int32_t)base_pitch+pm;if(index<0)index=0;if(index>16255)index=16255;nn=index/128;
  int old_index=osc[i].table_index;unsigned old_wave=osc[i].wave;
  #ifndef VSTATION_UNCACHED_MOD
  if(old_index==index && old_wave==p[i] && osc[i].drift==drift)continue;
#endif
  osc[i].table_index=index;osc[i].drift=drift;
  osc[i].inc=u32(198672+4*(unsigned)index)+(uint32_t)drift;
  if(old_index<0 || old_index/128!=nn || old_wave!=p[i]){
   unsigned map=p[i]==1?2:0;
   osc[i].blend=f32(264212+4*(unsigned)nn);
   osc[i].a=(const float*)(data()+u32(16+4*(map*128+(unsigned)nn)));
   osc[i].b=(const float*)(data()+u32(16+4*((map+1)*128+(unsigned)nn)));
  }
  osc[i].wave=p[i];
 }
}
static void tune_mask(unsigned mask){mask&=~osc_idle;if(mask)tune_mask_full(mask);}
/* During synth_panic()'s parameter replay only the retune at id 125 (after every
   input it reads has been set, and which recomputes all three oscillators from
   scratch) can matter; the earlier ones are overwritten by it. */
static unsigned panic_replay_tune_ok;
static void tune(void){if(panic_replay_tune_ok==2)return;tune_mask_full(7u);}
static void update_osc_idle(void){
 unsigned idle=0;
 if(p[127]>=3){
  unsigned ring=gains[4]!=0.f,fm=p[45]!=0 || p[46]!=64;
  if(gains[2]==0.f)idle|=4u;
  if(gains[1]==0.f && !ring && !fm)idle|=2u;
  if(gains[0]==0.f && !ring && !p[44])idle|=1u;
 }
 unsigned woke=osc_idle&~idle;osc_idle=idle;
 if(woke)tune_mask_full(woke);
}
/* Original signed PWM setters: normalized input*2*65536*2.
   Both LFO and envelope coefficients are (encoded-64)*2048. */
static void update_width_depths(void){
 for(unsigned i=0;i<3;i++){
  width_env_depth[i]=((int32_t)p[39+i]-64)*(p[102]?2048:512);
  width_lfo_depth[i]=p[102]?((int32_t)p[103+i]-64)*2048:(int32_t)(p[23]*32768/127);
 }
}
/* Only source changes or parameter events invalidate width offsets. */
static void update_width(void){
 /* Inlined osc_width_mod(): the envelope level and LFO value are shared by the
    three oscillators, so compute them once.  Same unsigned-wrap arithmetic. */
 uint32_t level=((mod_env.word[1]>>16)*mod_env.word[3])>>15,lv=(uint32_t)lfo_value(p[29]);
 for(unsigned i=0;i<3;i++)width_mod[i]=level*(uint32_t)width_env_depth[i]+lv*(uint32_t)width_lfo_depth[i];
}
void synth_pitch_bend(unsigned value14){
 if(value14>16383)value14=16383;
 bend_value=(int32_t)value14-8192;update_bend_offsets();tune();
}
void synth_param(unsigned id,unsigned value){
 if(id>=SYNTH_PARAM_COUNT)return;
 if(id==127){p[id]=value>4?4:value;update_osc_idle();return;}
 if(id==97){if(value<40)value=40;if(value>240)value=240;p[id]=value;update_lfo_rates();return;}
 if(id==98 || id==99){if(value>LFO_TEMPO_DIVISIONS)value=LFO_TEMPO_DIVISIONS;p[id]=value;update_lfo_rates();return;}
 if(id>=117 && id<=119 && value>25)value=25;
 if(id==115){if(value<52)value=52;if(value>76)value=76;}
 if(id==113 || id==114 || id==116 || id==124 || id==126)value=!!value;
 if((id==111 || id==112) && value>2)value=2;
 if(value>127)value=127;if(id<3 && value>3)value=3;if(id==15 || id==44)value=value?1:0;
 if((id==19 || id==25) && value>3)value=3;
 if(id>=28 && id<=30)value=value?1:0;
 if(id==42 && value>24)value=24;
 if(id>=69 && id<=71 && value>31)value=31;
 if(id==66 && value>2)value=2;if(id==67)value=value?1:0;
 if(id>=58 && id<=60 && value>4)value=4;
 if(id>=91 && id<=94)value=value?1:0;
 if(id==107 || id==106 || id==102 || id==101 || id==52 || id==53 || id==73 || id==84 || (id>=62 && id<=65))value=value?1:0;
 p[id]=value;
 if(id==125){for(unsigned i=0;i<3;i++)drift_scale[i]=osc_drift_scale(i,value);tune_mask_full(7u);}
 if(id==42 || (id>=117 && id<=119)){update_bend_offsets();tune();}
 if(id>=108 && id<=110)update_controller_amp();
 if(id==23 || (id>=39 && id<=41) || (id>=102 && id<=105))update_width_depths();
 if(id==101 || (id>=36 && id<=38)){update_pitch_env_depth();tune();}
 if(id==24 || (id>=84 && id<=90)){update_pitch_lfo_depth();tune();}
 if(id>=78 && id<=80){update_controller_pitch();tune();}
 if((id==66 || id==116) && value==0){glide.offset=0;tune();}
 if(id==45){float v=value/127.f;fm_depth=v*v;}
 if(id==46 || id==106){float v=((int)p[46]-64)/(p[106]?128.f:64.f);float doubled=v*2.f;fm_amount=doubled*doubled;if(v<0)fm_amount=-fm_amount;}
 if(id==47)fm_env.attack=amp_attack_44100[value];
 if(id==48)fm_env.decay=amp_fall_44100[value];
 if(id>=16 && id<19)osc[id-16].width=pulse_offset(((int)value-64)/128.f);
 if(id==19)lfo[0][0]=value;
 if(id==25)lfo[1][0]=value;
 if(id==20 || id==26)update_lfo_rates();
 if(id==21)lfo[0][5]=lfo_fade_44100[value];
 if(id==27)lfo[1][5]=lfo_fade_44100[value];
 if(id<3 || id==13 || id==14 || id==24 || id==30 || (id>=36 && id<=38) || id==42 || (id>=54 && id<=61))tune();
 /* OPT-E: synth_panic() replays all 170 parameters; the shared derived-state tail
    below only depends on the final parameter values, so during that replay it
    runs just where it can matter: first (filter_resonance is stateful: it only
    acts when its argument changes), at the last mod-envelope parameter (31..34
    setter), at the last filter-character parameter (122..124 setter) and at the
    last id.  Every skipped run would be overwritten by a later one. */
 if(panic_replay && id!=0 && id!=34 && id!=124 && id!=SYNTH_PARAM_COUNT-1)return;
 /* Original mixer setter 0x2b3a0 scales source levels before summing.
    Retain original noise bias even at zero; legacy mode cancels it at zero. */
 float source_scale=p[107]?.25f:1.f;mix_scale=p[107]?1.f:.25f;
 for(unsigned i=0;i<3;i++)gains[i]=(p[i+3]/127.f)*source_scale;
 gains[3]=p[107]?(p[43]/127.f)*source_scale:(p[43]?p[43]/127.f:-.0001f);
 gains[4]=(p[12]/127.f)*source_scale;
 update_osc_idle();
 /* Match the setter's float rounding order before integer conversion. */
 update_cutoff_target();
 filter_env_depth=p[73]?filter_mod_depth(((int)p[35]-64)/128.f):((int32_t)p[35]-64)*256;
 update_filter_lfo_depth();
 filter_resonance(filter,p[7]/127.f);
 if(id>=122 && id<=124)filter_character(filter,p[122],p[123],p[124]);
 amp.attack=amp_attack_44100[p[8]];amp.decay=amp_fall_44100[p[9]];amp.release=amp_fall_44100[p[11]];
 float s=p[10]/127.f;amp.sustain=s*s;
 if(id>=31 && id<=34)mod_envelope_params(&mod_env,p[31],p[32],p[33],p[34]);
 update_width();
}
void synth_panic(void){return_gate_mask=0;native_released=0;native_overflows=0;
#ifdef VSTATION_TEST
 applied_gate_mask=0;
#endif
 controller_amp_target=controller_amp_gain=1.f;channel_pressure=0;pressure_note=128;memset(note_pressure,128,sizeof(note_pressure));memset(&glide,0,sizeof(glide));glide_valid=0;glide_note=48;filter_note=60;memset(controllers,0,sizeof(controllers));memset(controller_pitch,0,sizeof(controller_pitch));velocity_native_active=0;held=0;sustain_down=0;noise_seed=0;memset(key_down,0,sizeof(key_down));bend_value=0;mod_envelope_init(&mod_env);mod_tick=0;memset(lfo,0,sizeof(lfo));memset(&amp,0,sizeof(amp));amp.mode=5;amp.gain=1;
 memset(&fm_env,0,sizeof(fm_env));fm_env.mode=5;fm_env.gain=1;
 fm_env.tick=15;fm_env.release=amp_fall_44100[0];
 memset(filter,0,sizeof(filter));filter[6]=-1;filter[8]=1;filter[13]=1;filter[14]=.1f;
 #ifndef PANIC_FORCE_FULL
 if(derived_ready){
  /* OPT-G: light rebuild.  The replay below exists only to re-derive state from
     p[]; every setter in it is also run by synth_param() whenever its parameter
     changes, so all of that derived state is already current - except the part
     this function's own resets just wiped (amp, fm_env, mod_env, lfo, filter) and
     the part that depends on the runtime state they reset (controller gain and
     pitch, bend offsets, cutoff target, modulation depths, width, tuning).  Redo
     exactly that, in the order the replay would have, and skip the 170-call walk.
     Build with -DPANIC_FORCE_FULL to fall back to the replay; the two paths are
     checked bit-identical by tests/panic_equiv (every id, every preset). */
  amp.attack=amp_attack_44100[p[8]];amp.decay=amp_fall_44100[p[9]];amp.release=amp_fall_44100[p[11]];
  {float sus=p[10]/127.f;amp.sustain=sus*sus;}
  fm_env.attack=amp_attack_44100[p[47]];fm_env.decay=amp_fall_44100[p[48]];
  mod_envelope_params(&mod_env,p[31],p[32],p[33],p[34]);
  lfo[0][0]=p[19];lfo[1][0]=p[25];update_lfo_rates();
  lfo[0][5]=lfo_fade_44100[p[21]];lfo[1][5]=lfo_fade_44100[p[27]];
  filter_resonance(filter,p[7]/127.f);filter_character(filter,p[122],p[123],p[124]);
  update_controller_amp();update_controller_pitch();update_bend_offsets();
  update_cutoff_target();update_filter_lfo_depth();update_pitch_lfo_depth();
  update_width();tune_mask_full(7u);
 }else
 #endif
 {
  panic_replay=1;panic_replay_tune_ok=2;for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++)synth_param(i,p[i]);panic_replay=0;panic_replay_tune_ok=0;
  derived_ready=1;
 }
 current_gain=0;
 cutoff_smoothed=cutoff_target;
 filter_control(cutoff_target,&cutoff_smoothed,0,filter_table_44100,filter);
}
void synth_init(void){clock_bpm_milli=0;derived_ready=0;memcpy(p,synth_defaults,sizeof(p));unison_pitch_units=0.f;memset(osc,0,sizeof(osc));for(unsigned i=0;i<3;i++)osc[i].table_index=-1;synth_faults=0;synth_panic();tune();}
/* Last-note priority and sustain are FM1 adapter policies. Released notes
   stay in their original order while the pedal is down; re-press replaces
   their existing entry. Pedal-up removes released notes in one pass. */
/* Original gains are sampled at a note gate, as in 0x43de0. Mode 0 retains
   the diagnostic amplitude response; mode 1 uses recovered envelope gains. */
static void gate_velocity(unsigned amp_gate_on,unsigned mod_gate_on,unsigned fm_gate_on){
 unsigned v=held?vel[held-1]:127;
 if(amp_gate_on){velocity_native_active=p[53];
  amp.gain=p[53]?velocity_gain(v,((int)p[49]-64)/128.f,p[52]):1.f;
 }
 if(mod_gate_on){float mg=velocity_gain(v,((int)p[50]-64)/128.f,p[52]);
  mod_env.word[3]=(!p[53] && p[50]==64)?32767:velocity_mod_gain(mg);
 }
 if(fm_gate_on)fm_env.gain=velocity_gain(v,((int)p[51]-64)/128.f,p[52]);
}
/* Per-envelope gate flags follow original 0x44062..0x440f3. The decision
   to suppress them on overlapping notes is an FM1 mono legato policy.
   Pedal-held notes count as overlap. A new phrase always receives a gate. */
/* Recovered time/interval rates; FM1 overlap and interrupted-glide policy. */
static void glide_gate(unsigned overlap,unsigned new_note){
 unsigned next=notes[held-1];
 if(native_mono_mode()){
  /* Recovered 0x44cd0 leaf: use the last note number, never add the
     unfinished glide. Mono-return 0x28e80 supplies zero duration. */
  if(!new_note || !p[116] || !p[66] || !p[68])glide.offset=0;
  else if(p[115]!=64)glide.offset=((int)p[115]-64)*8388608;
  else if(!glide_valid || (p[127]==2 && !overlap))glide.offset=0;
  else glide.offset=((int)glide_note-(int)next)*8388608;
  glide_note=next;glide_valid=1;
  portamento_configure(&glide,p[66]==1,p[68]);return;
 }
 if(!p[116] || !p[66] || !p[68])glide.offset=0;
 /* Original preglide takes precedence over previous-note interval.
    A fresh note may start above/below target even without a previous key. */
 else if(new_note && p[115]!=64)glide.offset=((int)p[115]-64)*8388608;
 else if(!glide_valid || (p[67] && !overlap))glide.offset=0;
 else glide.offset+=((int)glide_note-(int)next)*8388608;
 glide_note=next;glide_valid=1;
 portamento_configure(&glide,p[66]==1,p[68]);
}
static void gate_note(unsigned overlap,unsigned new_note){
 glide_gate(overlap,new_note);
 filter_note=notes[held-1];pressure_note=filter_note;apply_pressure();update_cutoff_target();
 unsigned cold=amp.mode==5 && amp.smoothed<.001f;
 if(cold)for(unsigned i=0;i<3;i++)if(p[69+i])osc[i].phase=osc_start_phase(p[69+i]);
 unsigned gates=0;
 for(unsigned i=0;i<3;i++)if(p[62+i] || !overlap || (!native_mono_mode() && cold))gates|=1u<<i;
 for(unsigned i=0;i<2;i++){
  unsigned always=p[111+i]?p[111+i]==2:p[65];
  if(always || !overlap || (!native_mono_mode() && cold))gates|=8u<<i;
 }
 /* Native return path 0x292c0 -> 0x28e80 -> 0x43de0 reuses gate flags.
    New-note selection still follows the FM1 overlap/cold-voice policy. */
 if(new_note)return_gate_mask=gates;
 else if(native_mono_mode() || p[126])gates=return_gate_mask;
 else gates&=7u; /* Existing return behavior never restarts LFO fades. */
#ifdef VSTATION_TEST
 applied_gate_mask=gates;
#endif
 unsigned ag=gates&1u,mg=gates&2u,fg=gates&4u;
 gate_velocity(ag,mg,fg);
 if(fg){if(cold){fm_env.value=0;fm_env.smoothed=0;}fm_env.mode=0;}
 if(mg)mod_envelope_gate(&mod_env,cold);
 if(ag)amp_gate(&amp,1);
 for(unsigned i=0;i<2;i++)if(gates&(8u<<i)){
  active_lfo(i)[6]=0;
  if(p[91+i])lfo_key_sync(active_lfo(i),p[93+i]);
 }
 tune();update_width();
}
static void resume_note(unsigned old){
 if(!held){fm_env.mode=3;amp_release(&amp);mod_envelope_release(&mod_env);}
 else if(old!=notes[held-1])gate_note(1,0);
}
/* Native direct sustain releases all three envelopes together unless the
   amplitude envelope is already quiet. It never edits the held-key list. */
static void native_release(void){
 if(amp.mode==5 && amp.smoothed<.001f)return;
 amp.mode=3;fm_env.mode=3;mod_envelope_release(&mod_env);
}
void synth_sustain(unsigned down){
 down=down?1:0;if(down==sustain_down)return;sustain_down=down;
 if(down)return;
 if(native_mono_mode()){if(native_released)native_release();return;}
 unsigned old=held?notes[held-1]:128,w=0;
 for(unsigned i=0;i<held;i++)if(key_down[notes[i]]){notes[w]=notes[i];vel[w++]=vel[i];}
 held=w;resume_note(old);
}
static void native_mono_note(unsigned note,unsigned velocity){
 unsigned overlap=held!=0;
 if(velocity){
  /* FM1 safety bound, not a claim about the original note pool's limit or
     exhaustion policy. Reject newest input without overwriting a key. */
  if(held>=128){if(native_overflows!=UINT32_MAX)++native_overflows;return;}
  notes[held]=note;vel[held++]=velocity>127?127:velocity;
  native_released=0;note_pressure[note]=128;
  if(amp.mode==5 && amp.smoothed<.001f)cutoff_smoothed=cutoff_target;
  gate_note(overlap,1);return;
 }
 /* The original pitch-list head is the oldest matching press (FIFO). */
 unsigned index=0;while(index<held && notes[index]!=note)++index;
 if(index==held)return;
 unsigned was_tail=index==held-1;
 for(unsigned i=index+1;i<held;i++){notes[i-1]=notes[i];vel[i-1]=vel[i];}
 --held;
 if(!was_tail)return;
 if(held){native_released=0;gate_note(1,0);}
 else{native_released=1;if(!sustain_down)native_release();}
}
void synth_note(unsigned note,unsigned velocity){
 if(note>127)return;
 if(native_mono_mode()){native_mono_note(note,velocity);return;}
 unsigned overlap=held!=0;unsigned old=held?notes[held-1]:128,index=held;
 for(unsigned i=0;i<held;i++)if(notes[i]==note){index=i;break;}
 key_down[note]=velocity?1:0;
 if(!velocity && (index==held || sustain_down))return;
 if(index<held){for(unsigned j=index+1;j<held;j++){notes[j-1]=notes[j];vel[j-1]=vel[j];}--held;}
 if(velocity){note_pressure[note]=128;notes[held]=note;vel[held++]=velocity>127?127:velocity;
  if(amp.mode==5 && amp.smoothed<.001f)cutoff_smoothed=cutoff_target;
  gate_note(overlap,1);}
 else resume_note(old);
}
void synth_render(float *out,unsigned n){
 static const VoiceRenderContext c={
  osc,lfo,&noise_seed,width_mod,pitch_lfo_depth,pitch_env_depth,bend_offsets,
  &mod_env,&mod_tick,&amp,&fm_env,filter,gains,&current_gain,&mix_scale,
  &cutoff_target,&cutoff_smoothed,&filter_env_depth,&filter_lfo_depth,
  &fm_depth,&fm_amount,&held,vel,&velocity_native_active,
  &controller_amp_gain,&controller_amp_target,&glide,&synth_faults,
  tune_mask,tune,update_width,synth_panic
 };
 voice_render_common_ptr(&c,out,n);
}

void synth_reset_controllers(void){
 channel_pressure=0;memset(note_pressure,128,sizeof(note_pressure));
 memset(controllers,0,sizeof(controllers));update_controller_amp();update_cutoff_target();update_controller_pitch();update_filter_lfo_depth();update_pitch_lfo_depth();
 synth_sustain(0);synth_pitch_bend(8192);
}

unsigned voice_active(void){return held || amp.mode!=5 || amp.smoothed>=.001f;}

#ifdef VSTATION_TEST
uint32_t voice_test_lfo(unsigned i,unsigned word){return i<2 && word<14?lfo[i][word]:0;}
#endif

/* Transport restart affects tempo-synced LFOs only, without re-gating notes. */
void voice_transport_start(void){for(unsigned i=0;i<2;i++)if(p[98+i])lfo_key_sync(active_lfo(i),p[93+i]);}

void voice_clock_rate(unsigned bpm_milli){clock_bpm_milli=bpm_milli;update_lfo_rates();}

#ifdef VSTATION_TEST
unsigned voice_test_pressure(void){return controllers[2];}
#endif

/* Release the whole mono stack without transiently re-gating older notes. */
void synth_all_notes_off(void){
 memset(key_down,0,sizeof(key_down));
 if(native_mono_mode()){held=0;native_released=1;if(!sustain_down)native_release();return;}
 if(sustain_down)return;
 held=0;fm_env.mode=3;amp_release(&amp);mod_envelope_release(&mod_env);
}

#ifdef VSTATION_TEST
int32_t voice_test_pitch_env(unsigned i){return i<3?pitch_env_depth[i]:0;}
#endif

#ifdef VSTATION_TEST
int32_t voice_test_width_depth(unsigned i,unsigned env){return i<3?(env?width_env_depth[i]:width_lfo_depth[i]):0;}
#endif

#ifdef VSTATION_TEST
float voice_test_fm_amount(void){return fm_amount;}
#endif

#ifdef VSTATION_TEST
float voice_test_mix(unsigned i){return i<5?gains[i]:mix_scale;}
#endif

#ifdef VSTATION_TEST
float voice_test_controller_amp(unsigned target){return target?controller_amp_target:controller_amp_gain;}
#endif

/* Shared output changes at renderer boundaries, before either voice renders. */
const uint32_t *voice_private_lfo(unsigned i){return lfo[i&1u];}
void voice_shared_lfo_refresh(void){
 if(shared_lfo_mask&(1u<<p[30])){unsigned m=pitch_lfo_mask();if(m)tune_mask(m);}
 if(shared_lfo_mask&(1u<<p[29]))update_width();
}

#ifdef VSTATION_TEST
const uint32_t *voice_test_selected_lfo(unsigned i){return active_lfo(i&1u);}
#endif

#ifdef VSTATION_TEST
int32_t voice_test_glide_offset(void){return glide.offset;}
#endif

#ifdef VSTATION_TEST
int32_t voice_test_bend_offset(unsigned i){return i<3?bend_offsets[i]:0;}
#endif

#ifdef VSTATION_TEST
float voice_test_filter(unsigned i){return i<19?filter[i]:0.f;}
#endif

#ifdef VSTATION_TEST
int32_t voice_test_drift(unsigned i){return i<3?osc[i].drift:0;}
#endif

#ifdef VSTATION_TEST
unsigned voice_test_return_gates(unsigned applied){return applied?applied_gate_mask:return_gate_mask;}
#endif

/* Bounded note-pool diagnostics; reset by panic/mode changes. */
unsigned voice_note_overflows(void){return native_overflows;}
#ifdef VSTATION_TEST
unsigned voice_test_native_state(unsigned field,unsigned index){
 switch(field){
  case 0:return held;case 1:return native_released;case 2:return native_overflows;
  case 3:return index<held?notes[index]:128;
  case 4:return index<held?vel[index]:128;
  case 5:return amp.mode;case 6:return glide_note;case 7:return sustain_down;
  case 8:return fm_env.mode;
  default:return 0;
 }
}
float voice_test_native_amp_gain(void){return amp.gain;}
#endif
