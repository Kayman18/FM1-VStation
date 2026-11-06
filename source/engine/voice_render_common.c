#include "voice_render_common.h"
#include "synth.h"
#include "shared_lfo.h"
#include "filter_modulation.h"
#include <string.h>
extern const uint32_t vst_wave_data[];
extern const float filter_table_44100[129];
extern unsigned voice_params[SYNTH_PARAM_COUNT];
float waveform(unsigned,uint32_t,uint32_t,float,const float*,const float*,const float*);
float voice_mix_float(float,float,float,const float*,float,float);
float noise_step(uint32_t*);
float osc_fm_depth(float,float*,float,float);
uint32_t osc_fm_increment(uint32_t,float,float);
void filter_control(int32_t,int32_t*,int32_t,const float*,float*);
void portable(float*,float);void portable4(float*,float);
void lfo_step(uint32_t*);
void mod_envelope_step(ModEnvelope*);
int32_t mod_envelope_filter(const ModEnvelope*,int32_t);
void amp_control(AmpRuntime*);float amp_audio(AmpRuntime*);
void portamento_step(Portamento*);

static inline __attribute__((always_inline)) const uint8_t *vr_data(void){return (const uint8_t*)vst_wave_data;}
static inline __attribute__((always_inline)) int32_t vr_lfo_value(const VoiceRenderContext *c,unsigned source){
 const uint32_t *s=(shared_lfo_mask&(1u<<source))?shared_lfo_state[source]:c->lfo[source];
 int32_t v;memcpy(&v,&s[1],4);return v;
}
static inline __attribute__((always_inline)) unsigned vr_pitch_lfo_mask(const VoiceRenderContext *c){unsigned m=0;for(unsigned i=0;i<3;i++)if(c->pitch_lfo_depth[i])m|=1u<<i;return m;}
static inline __attribute__((always_inline)) unsigned vr_pitch_env_mask(const VoiceRenderContext *c){unsigned m=0;for(unsigned i=0;i<3;i++)if(c->pitch_env_depth[i])m|=1u<<i;return m;}
static inline __attribute__((always_inline)) int32_t vr_pitch_offset(const VoiceRenderContext *c,unsigned i){return c->bend_offsets[i]+c->glide->offset;}

/* TEST30: small per-sample primitives are duplicated here as always-inline
 * expressions so the SRAM hot loop does not jump back to XIP for them.
 * Arithmetic and ordering are kept identical to the standalone helpers. */
static inline __attribute__((always_inline)) float vr_waveform(unsigned wave,uint32_t phase,uint32_t offset,float weight,
 const float *a,const float *b,const float *sine){
 unsigned i=phase>>21,j=(phase+offset)>>21;
 if(wave==0)return (sine[j]+sine[i])*.5f;
 if(wave==3){float x=a[i]-a[j],y=b[i]-b[j];return (y-x)*weight+x;}
 float x=a[j]+a[i],y=b[j]+b[i];return ((y-x)*weight+x)*.5f;
}
static inline __attribute__((always_inline)) float vr_waveform_normal(uint32_t phase,uint32_t offset,float weight,const float *a,const float *b){
 unsigned i=phase>>21,j=(phase+offset)>>21;float x=a[j]+a[i],y=b[j]+b[i];return ((y-x)*weight+x)*.5f;
}
static inline __attribute__((always_inline)) float vr_mix(float a,float b,float c,const float *g,float noise){
 float nn=(g[3]+.0001f)*noise;float x=g[1]*b+g[0]*a;x+=g[2]*c;x+=nn;x+=(b*a)*g[4];return x;
}
static inline __attribute__((always_inline)) float vr_noise(uint32_t *seed){
 *seed=*seed*1664525u+1013904223u;int32_t sv;memcpy(&sv,seed,4);return (float)sv*0x1p-31f;
}
static inline __attribute__((always_inline)) float vr_fm_depth(float output,float *smoothed,float base,float amount){
 *smoothed=output*.1f+*smoothed*.9f;uint32_t bits;memcpy(&bits,smoothed,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))*smoothed=0;float d=amount*(*smoothed)+base;
 if(d<0)d=0;if(d>1)d=1;return d;
}
static inline __attribute__((always_inline)) uint32_t vr_fm_inc(uint32_t base,float modulator,float depth){
 if(depth<0)depth=0;if(depth>1)depth=1;int32_t sb;memcpy(&sb,&base,4);
 float factor=modulator*(depth*32.f)+1.f;float step=(float)sb*factor;
 if(!(step>=-2147483648.f && step<2147483648.f))return 0x80000000u;return (uint32_t)(int32_t)step;
}
static inline __attribute__((always_inline)) float vr_amp_audio(AmpRuntime *e){
 if(e->mode==5 && e->smoothed<.001f)return 0;e->smoothed=e->output*.1f+e->smoothed*.9f;
 uint32_t bits;memcpy(&bits,&e->smoothed,4);if(!(bits&0x7f800000u) && (bits&0x7fffffu))e->smoothed=0;return e->smoothed;
}

/* TEST32: Patch 13 uses the recovered two-pole filter. Keep this sample-step
 * beside the shared voice renderer in executable SRAM so every physical voice
 * no longer jumps back to XIP once per sample. Arithmetic/order is copied
 * verbatim from filter.c::portable. */
#ifdef VSTATION_TARGET
#define VOICE_HOT_HELPER __attribute__((section(".voice_hot"),noinline,used))
#else
#define VOICE_HOT_HELPER static
#endif
VOICE_HOT_HELPER void vr_filter2_sram(float *s,float x){
 x*=s[8];
 float y=x*s[13]/((__builtin_fabsf(x)-0.9999924898147583f)*s[14]+1.0f);
 float q=s[10];
 if(q<.25f){q=__builtin_fabsf(s[3])-.25f;if(q<s[10])q=s[10];if(q>1)q=1;}
 float old=s[3];float low=s[9]*old+s[4];s[4]=low;
 float high=y-low;high-=old*q;s[2]=high;s[3]=high*s[9]+old;
 q=s[10];if(q<.25f){q=__builtin_fabsf(s[3])-.25f;if(q<s[10])q=s[10];if(q>1)q=1;}
 old=s[3];low=s[9]*old+s[4];s[4]=low;high=y-low;high-=old*q;s[2]=high;s[3]=high*s[9]+old;
}
/* TEST35: 169/200 recovered factory patches use ORIGINAL filter character
 * with drive=0.  In that exact state s[14]==0 and s[13]==1, so the nonlinear
 * normalizer above reduces algebraically to y=x after input compensation.
 * Avoiding fabs/multiply/add/divide on every sample is bit-identical for finite
 * audio and removes the dominant needless FPU work on patches such as
 * Arpeggio2.  Keep both 2-pole and 4-pole zero-drive stages in the hot section. */
static inline __attribute__((always_inline)) void vr_filter_stage(float *s,unsigned off,float x){
 float q=s[10],old=s[off+1];
 if(q<.25f){q=__builtin_fabsf(old)-.25f;if(q<s[10])q=s[10];if(q>1)q=1;}
 float low=s[9]*old+s[off+2];s[off+2]=low;
 float high=x-low;high-=old*q;s[off]=high;s[off+1]=high*s[9]+old;
}
static inline __attribute__((always_inline)) void vr_filter2_zero_drive_res0(float *s,float x);
/* OPT-A: when s[10]>=.25 the Q guard inside vr_filter_stage cannot fire (it only
 * tests s[10]), so the stage is pure linear arithmetic.  Keep filter state in
 * locals across the stages and store once.  Same operations, same order =>
 * bit-identical; only intermediate stores/reloads disappear. */
static inline __attribute__((always_inline)) void vr_filter4_guardfree(float *s,float x){
 x*=s[8];
 const float c=s[9],q=s[10];
 float oa=s[3],la=s[4],ob=s[17],lb=s[18],ha,hb;
 la=c*oa+la;ha=x-la;ha-=oa*q;oa=ha*c+oa;
 lb=c*ob+lb;hb=la-lb;hb-=ob*q;ob=hb*c+ob;
 la=c*oa+la;ha=x-la;ha-=oa*q;oa=ha*c+oa;
 lb=c*ob+lb;hb=la-lb;hb-=ob*q;ob=hb*c+ob;
 s[2]=ha;s[3]=oa;s[4]=la;s[16]=hb;s[17]=ob;s[18]=lb;
}
static inline __attribute__((always_inline)) void vr_filter2_zero_drive(float *s,float x){
 if(s[10]>=.25f){vr_filter2_zero_drive_res0(s,x);return;}
 x*=s[8];vr_filter_stage(s,2,x);vr_filter_stage(s,2,x);
}
static inline __attribute__((always_inline)) void vr_filter4_zero_drive(float *s,float x){
 if(s[10]>=.25f){vr_filter4_guardfree(s,x);return;}
 x*=s[8];vr_filter_stage(s,2,x);vr_filter_stage(s,16,s[4]);vr_filter_stage(s,2,x);vr_filter_stage(s,16,s[4]);
}
static inline __attribute__((always_inline)) void vr_filter2_zero_drive_res0(float *s,float x){
 /* resonance==0 => s[12]==1 and recovered coefficients guarantee s[10]>.25,
    so the nonlinear Q guard cannot fire. Keep both stages in locals; only the
    second-stage state is externally observable. */
 x*=s[8];float q=s[10],c=s[9],old=s[3],low=c*old+s[4];float high=x-low;high-=old*q;old=high*c+old;
 low=c*old+low;high=x-low;high-=old*q;old=high*c+old;s[2]=high;s[3]=old;s[4]=low;
}

/* TEST36 aggressive 4-voice path: algebraically collapse the two identical
 * zero-resonance SVF correction passes. This preserves the same linear
 * transfer/state equations in real arithmetic but evaluates them with four
 * multiplies/sample instead of six; float roundoff can differ slightly. */
static inline __attribute__((always_inline)) void vr_filter2_collapsed_coeffs(float *s,float *c2,float *lo_old,float *old_delta,float *old_old){
 float c=s[9],q=s[10],cc=c*c,a=1.f-c*(c+q);
 *c2=cc;*lo_old=c*(1.f+a);*old_delta=2.f*c-cc*q-cc*c;*old_old=(1.f-c*q)*a-cc*(1.f+a);
}
static inline __attribute__((always_inline)) void vr_filter2_collapsed(float *s,float x,float c2,float lo_old,float old_delta,float old_old){
 x*=s[8];float old=s[3],low=s[4],d=x-low;
 s[4]=low+c2*d+lo_old*old;
 s[3]=old_old*old+old_delta*d;
}
/* TEST37 performance mode: POLY 1/POLY 2 run the filter state at half
 * audio cadence.  Recompute the SVF coefficient for the ~22.05 kHz internal
 * cadence using a cheap polynomial approximation to 2*sin(2*asin(c/2)).
 * The external render/output rate remains 44.1 kHz.  This is intentionally a
 * sound/performance trade, not an exact V-Station equivalence path. */
static inline __attribute__((always_inline)) void vr_filter_half_coeffs(float *s){
 float c=s[5];float h=c*1.5f;if(h>1.35f)h=1.35f;
 float q=s[12]*.707099974155426f,t=h*q;t*=.5f;q*=1.f-t;if(q<h*.125f)q=h*.125f;
 s[9]=h;q*=2.f;s[10]=q;s[11]=q*2.f;
}
static inline __attribute__((always_inline)) float vr_amp_audio_aggressive(AmpRuntime *e){
 if(e->mode==5 && e->smoothed<.001f)return 0.f;
 /* Reassociated one-multiply form. Envelope output only changes at control rate. */
 e->smoothed+=(e->output-e->smoothed)*.1f;return e->smoothed;
}
static inline __attribute__((always_inline)) void vr_fm_smooth_only(float output,float *smoothed){
 *smoothed=output*.1f+*smoothed*.9f;uint32_t bits;memcpy(&bits,smoothed,4);
 if(!(bits&0x7f800000u) && (bits&0x7fffffu))*smoothed=0;
}

/* These control-rate steps are also shared by every physical voice. Keeping
 * one copy in SRAM avoids repeated XIP code fetches during dense polyphonic tails. */
static inline __attribute__((always_inline)) int32_t vr_signed_word(uint32_t x){int32_t y;memcpy(&y,&x,4);return y;}
static inline __attribute__((always_inline)) int32_t vr_floor_shift(int32_t x,unsigned n){return x>=0?x/(1<<n):-1-(int32_t)((~(uint32_t)x)>>n);}
VOICE_HOT_HELPER void vr_lfo_step_sram(uint32_t *s){
 unsigned mode=s[0]&255;int32_t out=vr_signed_word(s[1]);
 if(mode==0){uint32_t a=s[3];if(a&0x80000000u)a=0u-a;out=vr_floor_shift(vr_signed_word(a-0x40000000u),16);}
 else if(mode==1){int32_t hi=(int32_t)(s[3]>>16);if(hi&32768)hi-=65536;out=vr_floor_shift(hi*32768,16);}
 else if(mode==2)out=(int32_t)((s[3]>>16)&0x8000u);
 else if(mode==3 && (uint32_t)(s[3]-s[2])>0x80000000u){s[2]=s[3];s[12]=s[12]*1664525u+1013904223u;out=vr_signed_word(s[12]);}
 s[3]+=s[4];s[6]+=s[5];if(s[6]>0x7fffffffu)s[6]=0x7fffffffu;
 out=vr_signed_word((uint32_t)out*(s[6]>>16))/32768;s[1]=(uint32_t)out;
}
static inline __attribute__((always_inline)) uint32_t vr_mul_shift31(uint32_t a,uint32_t b){
 uint32_t a0=a&65535u,a1=a>>16,b0=b&65535u,b1=b>>16;
 uint32_t lo=a0*b0,t=a1*b0+(lo>>16);uint32_t u=a0*b1+(t&65535u);
 uint32_t hi=a1*b1+(t>>16)+(u>>16);lo=(u<<16)|(lo&65535u);return (hi<<1)|(lo>>31);
}
VOICE_HOT_HELPER void vr_mod_step_sram(ModEnvelope *e){
 uint32_t *s=e->word,mode=s[0]&255u,v=s[1],target=s[13];
 if(mode==0){v+=s[9];if(v>=0x7fffffffu){v=0x7fffffffu;mode=1;}}
 else if(mode==1){if(v-target<=16u){v=target;mode=2;}else v=vr_mul_shift31(s[11],v-target)+target;}
 else if(mode==2)v=target;
 else if(mode==3 || mode==4){if(v<=16u){v=0;mode=5;}else v=vr_mul_shift31(s[mode==3?15:17],v);}
 s[0]=(s[0]&0xffffff00u)|mode;s[1]=v;
}
/* TEST35: the filter-control trio executes once per 16 audio samples per
 * physical voice.  Inline exact copies into the SRAM renderer to remove three
 * XIP calls at every control tick.  Float/integer operation order matches the
 * standalone recovered routines. */
static inline __attribute__((always_inline)) int32_t vr_filter_lfo_inline(int32_t value,int32_t depth){
 uint32_t bits=(uint32_t)value*(uint32_t)depth;int32_t product;memcpy(&product,&bits,4);return product/32768;
}
static inline __attribute__((always_inline)) int32_t vr_mod_filter_inline(const ModEnvelope *e,int32_t depth){
 uint32_t level=((e->word[1]>>16)*e->word[3])>>15;uint32_t bits=level*(uint32_t)depth;int32_t product;memcpy(&product,&bits,4);return product/32768;
}
static inline __attribute__((always_inline)) void vr_coefficients_inline(float *s){
 float c=s[5],q=s[12]*0.707099974155426f;float t=c*q;t*=.5f;q*=1.0f-t;if(q<c*.125f)q=c*.125f;s[9]=c;q*=2.0f;s[10]=q;q*=2.0f;s[11]=q;
}
static inline __attribute__((always_inline)) void vr_filter_control_inline(int32_t target,int32_t *smooth,int32_t modulation,const float *table,float *s){
 *smooth=target/64+(*smooth*63)/64;int32_t index=*smooth+modulation;if(index<0)index=0;if(index>16256)index=16256;unsigned i=(unsigned)index/128;float fraction=(float)(index&127)/128.f;float c=table[i]*(1.f-fraction)+table[i+1]*fraction;if(c!=s[5]){s[5]=c;vr_coefficients_inline(s);}
}
#ifdef VSTATION_TARGET
typedef void (*VrFilterFn)(float*,float);
typedef void (*VrPortFn)(Portamento*);
static VrFilterFn volatile vr_portable4_fn=portable4;
static VrPortFn volatile vr_port_step_fn=portamento_step;
#define VR_FILTER_CONTROL vr_filter_control_inline
#define VR_FILTER_LFO vr_filter_lfo_inline
#define VR_MOD_FILTER vr_mod_filter_inline
#define VR_PORTABLE4(...) vr_portable4_fn(__VA_ARGS__)
#define VR_PORT_STEP(...) vr_port_step_fn(__VA_ARGS__)
#else
#define VR_FILTER_CONTROL filter_control
#define VR_FILTER_LFO filter_lfo_mod
#define VR_MOD_FILTER mod_envelope_filter
#define VR_PORTABLE4 portable4
#define VR_PORT_STEP portamento_step
#endif

/* amp_control only performs envelope arithmetic when tick==0. Avoid two XIP
 * calls per voice/sample for the other 15 samples while preserving the exact
 * tick transition and calling the original routine on the control sample. */
static inline __attribute__((always_inline)) void vr_amp_control_fast(AmpRuntime *e){
 if(e->tick){e->tick=(e->tick+1)&15u;return;}
 if(e->mode==0){e->value+=e->attack;if(e->value>=1.0f){e->value=1.0f;e->mode=1;}}
 else if(e->mode==1){float delta=e->value-e->sustain;if(delta<=.001f){e->value=e->sustain;e->mode=e->sustain==0.0f?5:2;}else e->value=delta*e->decay+e->sustain;}
 else if(e->mode==2)e->value=e->sustain;
 else if(e->mode==3){float delta=e->value;if(delta<=.001f){e->value=0;e->mode=5;}else e->value=delta*e->release;}
 else if(e->mode==5)e->value=0;
 e->output=e->gain*e->value;e->tick=1;
}

#ifdef VSTATION_TARGET
#define VOICE_HOT __attribute__((section(".voice_hot"),noinline,used))
#else
#define VOICE_HOT
#endif
VOICE_HOT void voice_render_common(const VoiceRenderContext *c,float *out,unsigned n){
 unsigned *p=voice_params;const float *sine=(const float*)(vr_data()+2064+23*8192);
 VoiceOsc *osc=c->osc,*o0=&osc[0],*o1=&osc[1],*o2=&osc[2];AmpRuntime *amp=c->amp,*fm_env=c->fm_env;ModEnvelope *mod_env=c->mod_env;
 float *filter=c->filter;uint32_t *width_mod=c->width_mod;
 /* TEST35: all of these controls are immutable for one render call. Hoist them
    out of the per-sample loop so the common path does not repeatedly fetch the
    preset array or re-evaluate three-entry masks. */
 const unsigned filter4=p[15]!=0,sync_on=p[44]!=0,fm_on=p[45]!=0 || p[46]!=64,glide_on=p[116]!=0 && p[66]!=0;
 const unsigned filter_out=filter4?18u:4u,pitch_lfo_mask=vr_pitch_lfo_mask(c),pitch_env_mask=vr_pitch_env_mask(c);
 const unsigned filter_source=p[28];
 const uint32_t *filter_lfo_state=(shared_lfo_mask&(1u<<filter_source))?shared_lfo_state[filter_source]:c->lfo[filter_source];
 const int filter_zero_drive=filter[14]==0.f && filter[13]==1.f;
 const int noise_off=p[43]==0;
 const float gain_target=*c->held?(*c->velocity_native_active?1.f:c->vel[*c->held-1]/127.f):*c->current_gain;
 const float controller_target=*c->controller_amp_target,mix_scale=*c->mix_scale;
 const int controller_active=controller_target!=1.f || *c->controller_amp_gain!=1.f;
 const int poly_perf=p[127]>=3;
 const int need_osc0=!poly_perf || c->gains[0]!=0.f || c->gains[4]!=0.f;
 const int need_osc1=!poly_perf || c->gains[1]!=0.f || c->gains[4]!=0.f || fm_on;
 const int need_osc2=!poly_perf || c->gains[2]!=0.f;
 if(poly_perf)vr_filter_half_coeffs(filter);
 /* Four-voice target: Patch 13 and many factory sounds land in this exact
    configuration.  Version the loop once per render call so the sample path
    carries no FM/sync/filter-mode/controller/glide branches.  The operations
    retained below are byte-for-byte the same arithmetic as the general path. */
 if(!filter4 && filter_zero_drive && !fm_on && !sync_on && (!glide_on || c->glide->offset==0) && !controller_active && filter[12]==1.f && o0->wave!=0 && o0->wave!=3 && o1->wave!=0 && o1->wave!=3 && o2->wave!=0 && o2->wave!=3){
  float fc2,flo_old,fold_delta,fold_old;vr_filter2_collapsed_coeffs(filter,&fc2,&flo_old,&fold_delta,&fold_old);
  const int half_ring=noise_off && c->gains[0]==0.f && c->gains[1]==0.f && c->gains[4]!=0.f;
  float ring_hold=0.f;
  for(unsigned k=0;k<n;k++){
#ifdef VSTATION_UNCACHED_MOD
   c->update_width();
#endif
   if(amp->tick==0){int32_t lv;memcpy(&lv,&filter_lfo_state[1],4);VR_FILTER_CONTROL(*c->cutoff_target,c->cutoff_smoothed,VR_FILTER_LFO(lv,*c->filter_lfo_depth)+VR_MOD_FILTER(mod_env,*c->filter_env_depth),filter_table_44100,filter);if(poly_perf)vr_filter_half_coeffs(filter);vr_filter2_collapsed_coeffs(filter,&fc2,&flo_old,&fold_delta,&fold_old);}
   float a0=0.f,a1=0.f;
   if(!half_ring || !(k&1u)){a0=vr_waveform_normal(o0->phase,o0->width+width_mod[0],o0->blend,o0->a,o0->b);a1=vr_waveform_normal(o1->phase,o1->width+width_mod[1],o1->blend,o1->a,o1->b);if(half_ring)ring_hold=a0*a1;}
   o0->phase+=o0->inc;o1->phase+=o1->inc;
   float a2=vr_waveform_normal(o2->phase,o2->width+width_mod[2],o2->blend,o2->a,o2->b);o2->phase+=o2->inc;
   float mixed;if(half_ring)mixed=(c->gains[2]*a2+ring_hold*c->gains[4])*mix_scale;else{float noise=noise_off?0.f:vr_noise(c->noise_seed);mixed=vr_mix(a0,a1,a2,c->gains,noise)*mix_scale;}
   if(!poly_perf || !(amp->tick&1u))vr_filter2_collapsed(filter,mixed,fc2,flo_old,fold_delta,fold_old);
   *c->current_gain+=(gain_target-*c->current_gain)*.002f;
   float y=filter[4]*vr_amp_audio_aggressive(amp)*(*c->current_gain)*.5f;
   if(amp->tick==0){vr_lfo_step_sram(c->lfo[0]);vr_lfo_step_sram(c->lfo[1]);if(pitch_lfo_mask)c->tune_mask(pitch_lfo_mask);}
   if(*c->mod_tick==0){vr_mod_step_sram(mod_env);if(pitch_env_mask)c->tune_mask(pitch_env_mask);c->update_width();}*c->mod_tick=(*c->mod_tick+1)&7u;
   vr_amp_control_fast(amp);vr_amp_control_fast(fm_env);
   uint32_t bits;memcpy(&bits,&y,4);if((bits&0x7f800000u)==0x7f800000u){++*c->faults;c->panic();y=0;}if(y>1)y=1;if(y< -1)y=-1;out[k]=y;
  }
  return;
 }
 float perf_last_a1=0.f,perf_last_fm=0.f;unsigned perf_seeded=0;
 for(unsigned k=0;k<n;k++){
#ifdef VSTATION_UNCACHED_MOD
  c->update_width();
#endif
  if(amp->tick==0){int32_t lv;memcpy(&lv,&filter_lfo_state[1],4);VR_FILTER_CONTROL(*c->cutoff_target,c->cutoff_smoothed,VR_FILTER_LFO(lv,*c->filter_lfo_depth)+VR_MOD_FILTER(mod_env,*c->filter_env_depth),filter_table_44100,filter);if(poly_perf)vr_filter_half_coeffs(filter);}
  const int audio_step=!poly_perf || !(amp->tick&1u) || !perf_seeded;
  if(audio_step){
   float noise=(poly_perf && noise_off)?0.f:vr_noise(c->noise_seed),fm=0.f;
   if(fm_on)fm=vr_fm_depth(fm_env->output,&fm_env->smoothed,*c->fm_depth,*c->fm_amount);else if(!poly_perf)vr_fm_smooth_only(fm_env->output,&fm_env->smoothed);
   float a0=need_osc0?vr_waveform(o0->wave,o0->phase,(o0->wave==3?0x80000000u:0)+o0->width+width_mod[0],o0->blend,o0->a,o0->b,sine):0.f;
   uint32_t before=o0->phase;o0->phase+=o0->inc;if(sync_on && (before&0x80000000u) && !(o0->phase&0x80000000u))o1->phase=0;
   float a1=need_osc1?vr_waveform(o1->wave,o1->phase,(o1->wave==3?0x80000000u:0)+o1->width+width_mod[1],o1->blend,o1->a,o1->b,sine):0.f;o1->phase+=o1->inc;
   float a2=need_osc2?vr_waveform(o2->wave,o2->phase,(o2->wave==3?0x80000000u:0)+o2->width+width_mod[2],o2->blend,o2->a,o2->b,sine):0.f;
   uint32_t step2=o2->inc;if(fm_on)step2=vr_fm_inc(step2,a1,fm);o2->phase+=step2;
   float mixed=vr_mix(a0,a1,a2,c->gains,noise)*mix_scale;
   if(filter_zero_drive){if(filter4)vr_filter4_zero_drive(filter,mixed);else vr_filter2_zero_drive(filter,mixed);}else if(filter4)VR_PORTABLE4(filter,mixed);else vr_filter2_sram(filter,mixed);
   perf_last_a1=a1;perf_last_fm=fm;perf_seeded=1;
  }else{
   uint32_t before=o0->phase;o0->phase+=o0->inc;if(sync_on && (before&0x80000000u) && !(o0->phase&0x80000000u))o1->phase=0;
   o1->phase+=o1->inc;uint32_t step2=o2->inc;if(fm_on)step2=vr_fm_inc(step2,perf_last_a1,perf_last_fm);o2->phase+=step2;
   if(!noise_off)*c->noise_seed=*c->noise_seed*1664525u+1013904223u;
  }
  *c->current_gain+=(gain_target-*c->current_gain)*.002f;
  float y=filter[filter_out]*vr_amp_audio(amp)*(*c->current_gain)*.5f;
  if(controller_active){*c->controller_amp_gain=controller_target*.002f+(*c->controller_amp_gain)*.998f;y*=*c->controller_amp_gain;}
  if(amp->tick==0){vr_lfo_step_sram(c->lfo[0]);vr_lfo_step_sram(c->lfo[1]);if(pitch_lfo_mask)c->tune_mask(pitch_lfo_mask);}
  if(*c->mod_tick==0){vr_mod_step_sram(mod_env);if(pitch_env_mask)c->tune_mask(pitch_env_mask);c->update_width();}*c->mod_tick=(*c->mod_tick+1)&7u;
  vr_amp_control_fast(amp);vr_amp_control_fast(fm_env);
  if(glide_on && c->glide->offset){
   int32_t previous=c->glide->offset;VR_PORT_STEP(c->glide);if(c->glide->offset==previous)c->glide->offset=0;unsigned changed=0;
   for(unsigned i=0;i<3;i++)if((c->bend_offsets[i]+previous)/65536!=vr_pitch_offset(c,i)/65536)changed=1;if(changed)c->tune_all();
  }
  uint32_t bits;memcpy(&bits,&y,4);if((bits&0x7f800000u)==0x7f800000u){++*c->faults;c->panic();y=0;}if(y>1)y=1;if(y< -1)y=-1;out[k]=y;
 }
}

VoiceRenderCommonFn const voice_render_common_ptr=voice_render_common;
