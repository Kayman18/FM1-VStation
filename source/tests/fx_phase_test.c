#include "synth.h"
#include "native_fx_phase.h"
#include "native_delay.h"
#include "preset.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const NativeFxPhase *synth_test_fx_phase(void);
extern const NativeChorus *synth_test_chorus(void);
extern const NativeEq *synth_test_eq(void);
extern const NativePan *synth_test_pan(void);
extern const NativeDelay *synth_test_delay(void);
static NativeChorus c_before;
static NativeEq e_before;
static NativePan p_before;
static NativeDelay d_before;
static float l[8192],r[8192],a[8192],b[8192];
static void save(void){c_before=*synth_test_chorus();e_before=*synth_test_eq();p_before=*synth_test_pan();d_before=*synth_test_delay();}
static void unchanged(void){assert(!memcmp(&c_before,synth_test_chorus(),sizeof c_before));assert(!memcmp(&e_before,synth_test_eq(),sizeof e_before));assert(!memcmp(&p_before,synth_test_pan(),sizeof p_before));assert(!memcmp(&d_before,synth_test_delay(),sizeof d_before));}
static void only_phases(unsigned c,unsigned e,unsigned p){
 if(c)c_before.phase=(c-1u)<<21;
 if(e)e_before.lfo[3]=(e-1u)<<30;
 if(p)p_before.phase=(p-1u)<<21;
 unchanged();
}
static void render(unsigned n){synth_render_stereo(l,r,n);for(unsigned i=0;i<n;i++)assert(isfinite(l[i])&&isfinite(r[i]));}
static void setup(unsigned mode,unsigned cap){
 synth_init();synth_param(95,cap);synth_param(127,mode);
 for(unsigned i=160;i<163;i++)assert(preset_param_max(i)==3 && synth_defaults[i]==0);
 synth_param(131,1);synth_param(128,51);
 synth_param(137,1);synth_param(132,80);synth_param(134,60);synth_param(135,91);
 synth_param(142,1);synth_param(139,80);synth_param(140,77);
 synth_param(151,1);synth_param(143,45);synth_param(145,89);synth_param(147,35);synth_param(149,85);
 synth_param(159,1);synth_param(152,45);synth_param(154,90);
}
int main(void){
 unsigned cases=0;
 for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<4;cap++)
 for(unsigned c=0;c<4;c++)for(unsigned e=0;e<4;e++)for(unsigned p=0;p<4;p++){
  setup(mode,cap);synth_param(160,c);synth_param(161,e);synth_param(162,p);render(37);
  save();synth_note(60,100);only_phases(c,e,p);assert(synth_test_fx_phase()->count==1 && !synth_test_fx_phase()->pending);
  render(77);save();synth_note(64,110);unchanged();
  synth_note(60,0);unchanged();synth_note(65,100);unchanged();
  synth_note(64,0);synth_note(65,0);assert(!synth_test_fx_phase()->count);unchanged();
  synth_sustain(1);synth_note(67,100);only_phases(c,e,p);render(41);
  save();synth_note(67,0);assert(!synth_test_fx_phase()->count);unchanged();
  synth_note(60,100);only_phases(c,e,p); /* Pedal-held audio is not list membership. */
  render(19);save();synth_note(60,100);synth_note(60,0);assert(synth_test_fx_phase()->count==1);unchanged();
  synth_note(62,100);unchanged();synth_note(60,0);synth_note(62,0);assert(!synth_test_fx_phase()->count);unchanged();
  synth_sustain(0);synth_note(55,110);only_phases(c,e,p);render(23);save();
  synth_note(128,100);synth_note(9999,0);synth_note(54,0);synth_note_pressure(55,88);unchanged();
  /* Rate/clock ownership changes do not masquerade as input phrase triggers. */
  unsigned cp=synth_test_chorus()->phase,ep=synth_test_eq()->lfo[3],pp=synth_test_pan()->phase;
  synth_clock_event(0xfa,1,500);synth_clock_event(0xfc,1,501);synth_clock_poll(2000);
  assert(cp==synth_test_chorus()->phase && ep==synth_test_eq()->lfo[3] && pp==synth_test_pan()->phase);
  save();synth_all_notes_off();assert(!synth_test_fx_phase()->count);unchanged();synth_note(48,100);only_phases(c,e,p);
  synth_panic();assert(!synth_test_fx_phase()->count && !synth_test_fx_phase()->pending);
  assert(synth_test_fx_phase()->initial[0]==c && synth_test_fx_phase()->initial[1]==e && synth_test_fx_phase()->initial[2]==p);
  ++cases;
 }
 /* All OFF is consumed immediately; selecting HIGH during an overlapping note
    must not unexpectedly apply a previously armed reset. */
 setup(0,3);synth_note(60,100);render(83);save();
 synth_param(160,3);synth_param(161,3);synth_param(162,3);synth_note(62,100);unchanged();
 synth_note(60,0);synth_note(62,0);synth_note(65,100);only_phases(3,3,3);
 /* Frozen effects still receive the same event-level accumulator write. */
 for(unsigned id=0;id<3;id++)synth_param((unsigned[]){151,137,142}[id],0);
 synth_param(159,0);synth_all_notes_off();render(111);save();synth_note(60,100);only_phases(3,3,3);
 save();render(111);unchanged();
 /* Shadow overflow is bounded; releases remain safe for repeated presses. */
 synth_init();for(unsigned i=0;i<5000;i++)synth_note(60,100);
 assert(synth_test_fx_phase()->count==128 && synth_test_fx_phase()->held[60]==128);
 for(unsigned i=0;i<5000;i++)synth_note(60,0);assert(synth_test_fx_phase()->count==0);
 for(unsigned i=0;i<128;i++)synth_note(i,100);assert(synth_test_fx_phase()->count==128);
 synth_all_notes_off();assert(!synth_test_fx_phase()->count);
 /* Same event schedule, differently partitioned renders, all FX active. */
 for(unsigned pass=0;pass<2;pass++){
  setup(0,3);synth_param(160,2);synth_param(161,1);synth_param(162,3);
  for(unsigned event=0;event<32;event++){
   if(event%4==0)synth_all_notes_off();synth_note(48+event%12,100);
   float *ll=(pass?a:l)+event*256,*rr=(pass?b:r)+event*256;
   if(!pass)synth_render_stereo(ll,rr,256);
   else{unsigned off=0;while(off<256){unsigned n=(off%27)+1;if(n>256-off)n=256-off;synth_render_stereo(ll+off,rr+off,n);off+=n;}}
  }
 }
 assert(!memcmp(a,l,sizeof a)&&!memcmp(b,r,sizeof b));
 unsigned values[SYNTH_PARAM_COUNT];uint8_t bytes[PRESET_BYTES];memcpy(values,synth_defaults,sizeof values);
 assert(preset_encode(values,bytes,sizeof bytes)&&preset_apply(bytes,sizeof bytes));
 assert(!synth_test_fx_phase()->count);for(unsigned i=0;i<3;i++)assert(!synth_test_fx_phase()->initial[i]);
 synth_param(160,9999);assert(synth_test_fx_phase()->initial[0]==3);
 printf("PASS: FX phrase phase: %u selector/mode/cap cases; overlap, duplicate presses, sustain, returns, invalid/off notes, tails, bypass, reset, bounded shadow, block invariance, Init\n",cases);
}
