#include "synth.h"
#include "amp_level.h"
#include "osc_fm.h"
#include "velocity.h"
#include "osc_tuning.h"
#include "portamento.h"
#include "osc_phase.h"
#include "lfo_sync.h"
#include "filter_tracking.h"
#include "filter_modulation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static float a[44100],b[44100],buf[256];
static void render(float *out,unsigned count,unsigned block){while(count){unsigned n=count<block?count:block;synth_render(out,n);out+=n;count-=n;}}
static void scenario(float *out,unsigned block,unsigned mod){synth_init();synth_param(95,2);synth_param(95,1);if(mod){synth_param(22,127);synth_param(23,127);synth_param(24,127);synth_param(20,127);synth_param(35,100);synth_param(36,90);synth_param(40,100);}synth_note(48,100);render(out,11025,block);synth_note(60,127);render(out+11025,11025,block);synth_note(60,0);render(out+22050,11025,block);synth_note(48,0);render(out+33075,11025,block);}
static void extensions_scenario(float *out,unsigned block){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(19,3);synth_param(25,3);synth_param(20,127);
 synth_param(26,110);synth_param(23,90);synth_param(24,50);synth_param(29,1);
 synth_param(54,80);synth_param(55,29);synth_param(57,70);synth_param(58,3);synth_param(53,1);synth_param(49,100);synth_param(50,30);synth_param(51,100);synth_param(52,1);synth_param(46,100);synth_param(47,35);synth_param(48,90);synth_param(45,96);synth_param(43,80);synth_param(44,1);synth_param(13,71);synth_note(48,100);
 render(out,22050,block);synth_note(48,0);render(out+22050,22050,block);
}
static void pedal_scenario(float *out,unsigned pedal){
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(48,100);if(pedal)synth_sustain(1);
 render(out,11025,256);if(pedal)synth_note(48,0);
 render(out+11025,11025,73);
 if(pedal)synth_sustain(0);else synth_note(48,0);
 render(out+22050,22050,256);
}
static void legato_scenario(float *out,unsigned block,unsigned extra_note){
 synth_init();synth_param(95,2);synth_param(95,1);for(unsigned id=62;id<=65;id++)synth_param(id,0);
 synth_param(53,1);synth_param(49,110);synth_param(50,110);synth_param(51,110);
 synth_param(35,100);synth_param(46,100);synth_param(48,110);
 synth_param(22,50);synth_param(21,80);synth_param(27,80);
 synth_note(48,60);render(out,11025,block);
 if(extra_note)synth_note(48,127);
 render(out+11025,11025,block);synth_note(48,0);render(out+22050,22050,block);
}
static void legato_overlap(float *out,unsigned block){
 synth_init();synth_param(95,2);synth_param(95,1);for(unsigned id=62;id<=65;id++)synth_param(id,0);
 synth_param(46,100);synth_param(48,110);synth_param(35,100);
 synth_note(48,100);render(out,11025,block);
 synth_sustain(1);synth_note(48,0);synth_note(60,90);render(out+11025,11025,block);
 synth_note(60,0);synth_sustain(0);render(out+22050,11025,block);
 synth_note(43,100);render(out+33075,11025,block);
}
static void glide_scenario(float *out,unsigned block,unsigned shape,unsigned time){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(66,shape);synth_param(68,time);
 synth_note(48,100);render(out,11025,block);
 synth_note(72,100);render(out+11025,11025,block);
 synth_pitch_bend(12000);synth_note(36,100);render(out+22050,11025,block);
 synth_note(36,0);render(out+33075,11025,block);
}
static void tracking_scenario(float *out,unsigned block,unsigned amount,unsigned release_edit){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(72,amount);synth_param(66,2);synth_param(68,50);
 synth_note(48,100);render(out,11025,block);synth_sustain(1);
 synth_note(72,100);synth_note(72,0);render(out+11025,11025,block);
 synth_sustain(0);render(out+22050,11025,block);synth_note(48,0);
 if(release_edit)synth_param(72,amount); /* Re-applying must use the release note. */
 render(out+33075,11025,block);
}
static void filter_curve_scenario(float *out,unsigned block,unsigned original,unsigned lfo_depth,unsigned env_depth){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(72,80);synth_param(73,original);synth_param(74,lfo_depth);
 synth_param(35,env_depth);synth_param(22,70);synth_param(26,100);synth_param(28,1);
 synth_note(48,100);render(out,11025,block);synth_note(72,127);render(out+11025,11025,block);
 synth_note(72,0);render(out+22050,11025,block);synth_note(48,0);render(out+33075,11025,block);
}
static void controller_scenario(float *out,unsigned block,unsigned source,unsigned amount,unsigned reset){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(75+source,amount);synth_param(72,60);synth_param(11,100);
 synth_note(48,100);render(out,11025,block);synth_controller(source,127);
 render(out+11025,11025,block);synth_note(48,0);synth_controller(source,75);
 render(out+22050,11025,block);
 if(reset)synth_reset_controllers();else synth_controller(source,0);
 render(out+33075,11025,block);
}
static void pitch_controller_scenario(float *out,unsigned block){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(78,76);synth_param(79,57);synth_param(80,67);
 synth_param(66,2);synth_param(68,45);synth_param(54,79);synth_param(57,55);
 synth_note(60,100);render(out,11025,block);synth_controller(0,100);synth_note(72,100);
 render(out+11025,11025,block);synth_controller(1,90);synth_controller(2,60);synth_note(72,0);
 render(out+22050,11025,block);synth_note(60,0);synth_reset_controllers();render(out+33075,11025,block);
}
static void filter_lfo_controller_scenario(float *out,unsigned block,unsigned reset){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(73,1);synth_param(81,96);synth_param(82,48);synth_param(83,80);
 synth_param(28,1);synth_param(26,100);synth_param(27,70);synth_param(11,100);
 synth_note(60,100);render(out,11025,block);synth_controller(0,127);synth_controller(2,90);
 render(out+11025,11025,block);synth_controller(1,100);synth_note(60,0);
 render(out+22050,11025,block);
 if(reset)synth_reset_controllers();else for(unsigned source=0;source<3;source++)synth_controller(source,0);
 render(out+33075,11025,block);
}
static void vibrato_controller_scenario(float *out,unsigned block,unsigned active,unsigned reset){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(84,1);synth_param(86,active?88:64);synth_param(87,active?48:64);synth_param(88,active?80:64);
 synth_param(30,1);synth_param(26,90);synth_param(27,50);synth_param(11,100);
 synth_note(60,100);render(out,11025,block);synth_controller(0,127);synth_controller(2,90);
 render(out+11025,11025,block);synth_controller(1,100);synth_note(60,0);
 render(out+22050,11025,block);
 if(reset)synth_reset_controllers();else for(unsigned source=0;source<3;source++)synth_controller(source,0);
 render(out+33075,11025,block);
}
static void independent_pitch_lfo(float *out,unsigned block){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(84,1);synth_param(85,80);synth_param(89,48);synth_param(90,70);
 synth_param(86,72);synth_param(30,1);synth_param(26,90);synth_param(36,90);
 synth_note(60,100);render(out,22050,block);synth_controller(0,100);synth_note(60,0);render(out+22050,22050,block);
}
static void key_sync_scenario(float *out,unsigned block,unsigned sync){
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(91,sync);synth_param(92,sync);synth_param(93,1);
 synth_param(24,50);synth_param(22,80);synth_param(28,1);synth_param(20,90);synth_param(26,80);
 synth_note(60,100);render(out,11025,block);synth_note(72,100);render(out+11025,11025,block);
 synth_note(72,0);render(out+22050,11025,block);synth_note(60,0);render(out+33075,11025,block);
}
int main(void){
 uint32_t ls[14]={0};ls[12]=123;ls[1]=456;lfo_key_sync(ls,0);assert(ls[2]==0x40000000u && ls[3]==ls[2] && ls[12]==123 && ls[1]==456);
 lfo_key_sync(ls,1);assert(ls[2]==0 && ls[3]==0);
 ls[0]=2;lfo_key_sync(ls,0);assert(ls[2]==0x80000000u);
 key_sync_scenario(a,256,1);key_sync_scenario(b,73,1);assert(!memcmp(a,b,sizeof(a)));
 key_sync_scenario(b,256,0);assert(memcmp(a,b,sizeof(a)));

 independent_pitch_lfo(a,256);independent_pitch_lfo(b,73);assert(!memcmp(a,b,sizeof(a)));
 /* Muted oscillator base depths cannot leak into audible oscillator pitch. */
 const unsigned depth_id[3]={85,89,90};
 for(unsigned audible=0;audible<3;audible++){
  for(unsigned variant=0;variant<2;variant++){
   synth_init();synth_param(95,2);synth_param(95,1);synth_param(84,1);synth_param(20,90);
   for(unsigned osc=0;osc<3;osc++){synth_param(3+osc,osc==audible?100:0);synth_param(depth_id[osc],osc==audible?76:(variant?95:64));}
   synth_note(60,100);render(variant?b:a,44100,256);
  }
  assert(!memcmp(a,b,sizeof(a)));
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(84,1);synth_param(20,90);
  for(unsigned osc=0;osc<3;osc++)synth_param(3+osc,osc==audible?100:0);
  synth_note(60,100);render(b,44100,256);assert(memcmp(a,b,sizeof(a))); /* Each oscillator responds. */
 }

 for(unsigned source=0;source<3;source++)for(unsigned amount=40;amount<=88;amount+=48){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(84,1);synth_param(86+source,amount);synth_controller(source,127);synth_param(20,90);synth_note(60,100);render(a,44100,256);
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(84,1);synth_param(85,amount);synth_param(89,amount);synth_param(90,amount);synth_param(20,90);synth_note(60,100);render(b,44100,256);
  assert(!memcmp(a,b,sizeof(a)));
 }
 vibrato_controller_scenario(a,256,1,0);vibrato_controller_scenario(b,73,1,0);assert(!memcmp(a,b,sizeof(a)));
 vibrato_controller_scenario(b,256,1,1);assert(!memcmp(a,b,sizeof(a)));
 vibrato_controller_scenario(b,256,0,0);assert(memcmp(a,b,sizeof(a))); /* Must run LFO retuning while old p24 is zero. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(24,45);synth_note(60,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(24,45);synth_param(86,127);synth_controller(0,127);synth_note(60,100);render(b,44100,256);
 assert(!memcmp(a,b,sizeof(a))); /* Legacy mode bypass. */

 assert(filter_mod_depth_offsets(0.f,.5f,0.f,-.5f)==0);
 assert(filter_mod_depth_offsets(.25f,1.f,1.f,1.f)==16256);
 assert(filter_mod_depth_offsets(-.25f,-1.f,-1.f,-1.f)==-16256);
 for(unsigned source=0;source<3;source++)for(unsigned amount=32;amount<=96;amount+=64){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(73,1);synth_param(81+source,amount);synth_controller(source,127);synth_note(60,100);render(a,44100,256);
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(73,1);synth_param(74,amount);synth_note(60,100);render(b,44100,256);
  assert(!memcmp(a,b,sizeof(a))); /* Controller input equals matching base depth. */
 }
 filter_lfo_controller_scenario(a,256,0);filter_lfo_controller_scenario(b,73,0);assert(!memcmp(a,b,sizeof(a)));
 filter_lfo_controller_scenario(b,256,1);assert(!memcmp(a,b,sizeof(a)));
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(22,60);synth_note(60,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(22,60);synth_param(81,127);synth_controller(0,127);synth_note(60,100);render(b,44100,256);
 assert(!memcmp(a,b,sizeof(a))); /* TEST6 curve ignores new depth routes. */

 assert(osc_controller_pitch(127,12.f)==1536.f);
 assert(osc_controller_pitch(0,-64.f)==0.f);
 assert(osc_base_pitch_offsets(60,0,0,0,0,128.f,256.f,-64.f)==8000);
 for(unsigned source=0;source<3;source++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(78+source,76);synth_controller(source,127);synth_note(60,100);render(a,44100,256);
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(61,76);synth_param(13,76);synth_param(14,64);synth_note(60,100);render(b,44100,256);
  assert(!memcmp(a,b,sizeof(a))); /* Full +12 route equals all-oscillator transpose. */
 }
 pitch_controller_scenario(a,256);pitch_controller_scenario(b,73);assert(!memcmp(a,b,sizeof(a)));
 /* No-op pitch update during release must retain note 72, not fallback to 48. */
 for(unsigned variant=0;variant<2;variant++){
  float *out=variant?b:a;synth_init();synth_param(95,2);synth_param(95,1);synth_param(11,100);synth_note(72,100);render(out,22050,256);
  synth_note(72,0);if(variant)synth_pitch_bend(8192);render(out+22050,22050,256);
 }
 assert(!memcmp(a,b,sizeof(a)));

 assert(filter_controller_offset(127,12.f)==12.f);
 assert(filter_controller_offset(0,-64.f)==0.f);
 assert(filter_tracking_offsets(0.f,60,0.f,12.f,-6.f,3.f)==1152);
 for(unsigned source=0;source<3;source++){
  controller_scenario(a,256,source,100,0);controller_scenario(b,73,source,100,0);assert(!memcmp(a,b,sizeof(a)));
  controller_scenario(b,256,source,100,1);assert(!memcmp(a,b,sizeof(a))); /* Reset on release. */
  controller_scenario(b,256,source,28,0);assert(memcmp(a,b,sizeof(a))); /* Polarity. */
 }
 /* Neutral routes cannot alter sound, even before note-on. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);for(unsigned source=0;source<3;source++)synth_controller(source,127);
 synth_note(48,100);render(b,44100,256);assert(!memcmp(a,b,sizeof(a)));
 /* Panic clears latched controller values before another phrase. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(75,100);synth_controller(0,127);synth_panic();synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(75,100);synth_note(48,100);render(b,44100,256);assert(!memcmp(a,b,sizeof(a)));

 assert(filter_mod_depth(0.f)==0);
 assert(filter_mod_depth(-.5f)==-16256 && filter_mod_depth(.5f)==16256);
 assert(filter_mod_depth(.25f)==2048 && filter_mod_depth(-.25f)==-2048);
 assert(filter_lfo_mod(-32767,16256)==-(32767*16256/32768));
 int32_t last=-16256;
 for(unsigned i=0;i<128;i++){int32_t v=filter_mod_depth(((int)i-64)/128.f);assert(v>=last);last=v;}
 filter_curve_scenario(a,256,1,100,96);filter_curve_scenario(b,73,1,100,96);assert(!memcmp(a,b,sizeof(a)));
 filter_curve_scenario(b,256,1,28,96);assert(memcmp(a,b,sizeof(a))); /* LFO polarity. */
 filter_curve_scenario(b,256,0,100,96);assert(memcmp(a,b,sizeof(a))); /* Original curve differs. */
 /* Zero depths must give identical audio in both modes. */
 for(unsigned mode=0;mode<2;mode++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(73,mode);synth_param(22,0);synth_param(35,64);synth_param(74,64);
  synth_note(48,100);render(mode?b:a,44100,256);
 }
 assert(!memcmp(a,b,sizeof(a)));

 assert(filter_tracking_target(0.f,60,1.f)==0);
 assert(filter_tracking_target(0.f,72,1.f)==1536);
 assert(filter_tracking_target(0.f,48,1.f)==-1536);
 assert(filter_tracking_target(1.f,127,1.f)==24832); /* Clamp after smoothing. */
 for(unsigned amount=0;amount<=127;amount+=127){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(72,amount);synth_note(60,100);render(amount?b:a,44100,256);
 }
 assert(!memcmp(a,b,sizeof(a))); /* Tracking pivot is MIDI 60. */
 tracking_scenario(a,256,127,0);tracking_scenario(b,73,127,0);assert(!memcmp(a,b,sizeof(a)));
 tracking_scenario(b,256,127,1);assert(!memcmp(a,b,sizeof(a))); /* Release latch. */
 tracking_scenario(b,256,0,0);assert(memcmp(a,b,sizeof(a)));

 assert(osc_start_phase(1)==0 && osc_start_phase(16)==0x80000000u && osc_start_phase(31)==0);
 for(unsigned pass=0;pass<2;pass++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(4,0);synth_param(5,0);synth_param(69,pass?16:1);
  synth_note(48,100);render(pass?b:a,44100,256);
 }
 assert(memcmp(a,b,sizeof(a)));

 assert(portamento_time_ms(0)==0 && portamento_time_ms(127)==10000);
 Portamento configured={0,12*8388608,0,0};
 portamento_configure(&configured,1,127);assert(configured.rate==-(12*8388608/441000));
 configured.offset=12*8388608;portamento_configure(&configured,0,127);assert(configured.rate>0);
 portamento_configure(&configured,0,0);assert(configured.offset==0 && configured.rate==0);
 Portamento pg={0,100000,0,1073741824};portamento_step(&pg);assert(pg.offset==50000);
 pg=(Portamento){1,-100,0,30};for(unsigned i=0;i<4;i++)portamento_step(&pg);assert(pg.offset==0);
 pg=(Portamento){1,100,0,-30};for(unsigned i=0;i<4;i++)portamento_step(&pg);assert(pg.offset==0);
 for(unsigned shape=1;shape<=2;shape++){
  glide_scenario(a,256,shape,64);glide_scenario(b,73,shape,64);assert(!memcmp(a,b,sizeof(a)));
  glide_scenario(b,256,0,64);assert(memcmp(a,b,sizeof(a)));
  /* Zero time is immediate and must preserve the no-glide sequence here. */
  glide_scenario(a,256,shape,0);assert(!memcmp(a,b,sizeof(a)));
 }

 /* Same-note legato must preserve envelopes, fade and latched native gain,
    even when the second event carries a different velocity. */
 legato_scenario(a,256,0);legato_scenario(b,73,1);assert(!memcmp(a,b,sizeof(a)));
 /* Pedal overlap, final release and new phrase remain chunk-independent. */
 legato_overlap(a,256);legato_overlap(b,73);assert(!memcmp(a,b,sizeof(a)));
 float phrase_energy=0;for(unsigned i=33075;i<44100;i++)phrase_energy+=fabsf(a[i]);
 assert(phrase_energy>0 && synth_faults==0);

 /* Octave tuning must equal transposing the incoming MIDI note. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(60,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);for(unsigned id=58;id<=60;id++)synth_param(id,3);
 synth_note(48,100);render(b,44100,73);assert(!memcmp(a,b,sizeof(a)));
 /* Master cents equal the same cents on all three oscillators. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(57,91);synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);for(unsigned id=54;id<=56;id++)synth_param(id,91);
 synth_note(48,100);render(b,44100,73);assert(!memcmp(a,b,sizeof(a)));
 /* Opposite cent offsets cancel before integer table-index truncation. */
 assert(osc_base_pitch(48,0,0,31,-31)==48*128);
 /* Tuning a muted oscillator must not leak into the others (FM off). */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(3,0);synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(3,0);synth_param(54,127);synth_param(61,71);
 synth_note(48,100);render(b,44100,256);assert(!memcmp(a,b,sizeof(a)));

 /* Recovered gain limits, curve selection and exact Q15 unity. */
 assert(velocity_gain(127,0,0)==1 && velocity_gain(1,0,1)==1);
 assert(velocity_gain(127,-.5f,0)==0);
 assert(velocity_gain(0,.5f,0)==0);
 assert(velocity_mod_gain(1)==32768);
 assert(velocity_gain(64,.5f,1)<velocity_gain(64,.5f,0));
 /* Native neutral velocity must produce identical audio for soft/hard notes. */
 for(unsigned pass=0;pass<2;pass++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(53,1);synth_note(48,pass?127:1);
  render(pass?b:a,44100,pass?73:256);
 }
 assert(!memcmp(a,b,sizeof(a)));
 /* Fully negative amplitude response silences maximum velocity. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(53,1);synth_param(49,0);synth_note(48,127);
 render(a,44100,256);for(unsigned i=0;i<44100;i++)assert(a[i]==0);
 /* FM and modulation velocity have independent audible destinations. */
 for(unsigned dest=50;dest<=51;dest++){
  for(unsigned pass=0;pass<2;pass++){
   synth_init();synth_param(95,2);synth_param(95,1);synth_param(53,1);synth_param(dest,127);
   if(dest==50)synth_param(35,100);
   else{synth_param(46,100);synth_param(48,110);synth_param(3,0);synth_param(4,0);synth_param(5,100);}
   synth_note(48,pass?127:24);render(pass?b:a,44100,256);
  }
  assert(memcmp(a,b,sizeof(a)));
 }

 /* Defined conversion at signed/overflow boundaries, including reverse FM. */
 assert(osc_fm_increment(0x01000000u,-1.f,1.f)==0xe1000000u);
 assert(osc_fm_increment(0x7fffffffu,1.f,1.f)==0x80000000u);
 assert(osc_fm_increment(0x80000000u,0.f,0.f)==0x80000000u);
 float sm=0;assert(osc_fm_depth(1,&sm,0,1)==.1f);
 sm=0;assert(osc_fm_depth(1,&sm,0,-1)==0);
 sm=1;assert(osc_fm_depth(1,&sm,1,4)==1);
 /* Envelope-only FM must alter carrier audio with static FM at zero. */
 for(unsigned pass=0;pass<2;pass++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(3,0);synth_param(4,0);synth_param(5,100);
  synth_param(46,pass?110:64);synth_param(47,0);synth_param(48,110);
  synth_note(48,100);render(pass?b:a,44100,256);
 }
 assert(memcmp(a,b,sizeof(a)));

 /* FM affects carrier 3 even when the modulator mixer level is zero. */
 for(unsigned pass=0;pass<2;pass++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(3,0);synth_param(4,0);synth_param(5,100);
  synth_param(45,pass?100:0);synth_note(48,100);render(pass?b:a,44100,256);
 }
 assert(memcmp(a,b,sizeof(a)));
 /* Zero carrier level isolates the FM path from oscillators 1 and 2. */
 for(unsigned pass=0;pass<2;pass++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(5,0);synth_param(45,pass?127:0);
  synth_note(48,100);render(pass?b:a,44100,256);
 }
 assert(!memcmp(a,b,sizeof(a)));

 /* new features preserve chunk-size independence. */
 extensions_scenario(a,256);extensions_scenario(b,73);assert(!memcmp(a,b,sizeof(a)));
 /* Noise-only voice must produce finite nonzero audio. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(3,0);synth_param(4,0);synth_param(5,0);synth_param(43,100);
 synth_note(48,100);render(a,44100,256);float energy=0;
 for(unsigned i=0;i<44100;i++){assert(isfinite(a[i]));energy+=fabsf(a[i]);}assert(energy>0);
 /* Sync must change oscillator 2 at an unequal pitch. */
 for(unsigned pass=0;pass<2;pass++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(3,0);synth_param(4,100);synth_param(5,0);
  synth_param(13,71);synth_param(44,pass);synth_note(48,100);render(pass?b:a,44100,256);
 }
 assert(memcmp(a,b,sizeof(a)));

 /* held-by-pedal audio equals held-by-key audio, including release. */
 pedal_scenario(a,1);pedal_scenario(b,0);assert(!memcmp(a,b,sizeof(a)));
 /* repeated sustained notes must stay within the 128-note stack. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_sustain(1);
 for(unsigned i=0;i<1024;i++){synth_note(i%128,100);synth_note(i%128,0);}
 synth_sustain(0);for(unsigned i=0;i<2000;i++)synth_render(buf,256);
 for(unsigned i=0;i<256;i++)assert(buf[i]==0);

 /* minimum bend with two-semitone range equals note minus two. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_pitch_bend(0);synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(46,100);render(b,44100,73);assert(!memcmp(a,b,sizeof(a)));
 /* Range zero must ignore wheel movement. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(42,0);synth_pitch_bend(16383);synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(48,100);render(b,44100,73);assert(!memcmp(a,b,sizeof(a)));
 scenario(a,256,0);scenario(b,73,0);assert(!memcmp(a,b,sizeof(a)));
 scenario(a,256,1);scenario(b,73,1);assert(!memcmp(a,b,sizeof(a)));
 for(unsigned dest=22;dest<25;dest++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_note(48,100);render(a,44100,256);
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(dest,127);synth_note(48,100);render(b,44100,73);
  assert(memcmp(a,b,sizeof(a)));for(unsigned i=0;i<44100;i++)assert(isfinite(b[i]));
 }
 /* selecting LFO2 must isolate a destination from LFO1. */
 for(unsigned dest=22;dest<25;dest++){
  for(unsigned pass=0;pass<2;pass++){
   synth_init();synth_param(95,2);synth_param(95,1);synth_param(dest,100);synth_param(dest+6,1);
   synth_param(26,80);synth_param(27,20);
   synth_param(19,pass?2:0);synth_param(20,pass?127:0);
   synth_note(48,100);render(pass?b:a,44100,pass?73:256);
  }
  assert(!memcmp(a,b,sizeof(a)));
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(dest,100);synth_param(dest+6,1);
  synth_param(26,12);synth_param(27,20);synth_note(48,100);
  render(b,44100,73);assert(memcmp(a,b,sizeof(a)));
 }
 /* modulation envelope must affect the signal. */
 synth_init();synth_param(95,2);synth_param(95,1);synth_note(48,100);render(a,44100,256);
 synth_init();synth_param(95,2);synth_param(95,1);synth_param(35,100);synth_note(48,100);render(b,44100,73);
 assert(memcmp(a,b,sizeof(a)));
 synth_init();synth_param(95,2);synth_param(95,1);for(unsigned n=0;n<128;n++)synth_note(n,100);
 for(unsigned n=0;n<128;n++)synth_note(n,0);
 for(unsigned i=0;i<2000;i++)synth_render(buf,256);
 for(unsigned i=0;i<256;i++)assert(buf[i]==0);
 /* The existing +12 dB master control may boost above full scale.
    Voice mix and distortion are bounded by 1; master is bounded by this curve.
    Separate PCM tests verify safe 24-bit clipping at the hardware boundary. */
 unsigned seed=7654;float peak=0;unsigned samples=0;
 for(unsigned patch=0;patch<128;patch++){
  synth_init();synth_param(95,2);synth_param(95,1);for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++){
   seed=seed*1664525u+1013904223u;unsigned value=id<3?(seed>>24)%4:id==15?patch&1:(seed>>24)&127;
   /* This bound is for the voice/master core. Parallel FX sends can legitimately
      exceed it and have dedicated stress/PCM-saturation tests. Keep local gates off. */
   if(id==131||id==137||id==142||id==151||id==159||id==167)value=0;
   synth_param(id,value);
  }
  for(unsigned j=0;j<64;j++){
   seed=seed*1664525u+1013904223u;unsigned note=(seed>>24)&127;
   synth_note(note,127);synth_render(buf,256);synth_note(note,0);
   for(unsigned i=0;i<256;i++){assert(isfinite(buf[i]));assert(fabsf(buf[i])<=4.f*amp_level_curve(76)+.00001f);if(fabsf(buf[i])>peak)peak=fabsf(buf[i]);}
   samples+=256;assert(synth_faults==0);
  }
 }
 for(unsigned poles=0;poles<2;poles++)for(unsigned resonance=0;resonance<2;resonance++){
  synth_init();synth_param(95,2);synth_param(95,1);synth_param(15,poles);synth_param(7,resonance*127);synth_note(48,127);
  for(unsigned sweep=0;sweep<4;sweep++)for(unsigned cutoff=0;cutoff<128;cutoff++){
   synth_param(6,(sweep&1)?127-cutoff:cutoff);synth_render(buf,256);
   for(unsigned i=0;i<256;i++){assert(isfinite(buf[i]));assert(fabsf(buf[i])<=1);}
   assert(synth_faults==0);samples+=256;
  }
 }
 synth_panic();synth_render(buf,256);for(unsigned i=0;i<256;i++)assert(buf[i]==0);
 printf("PASS: default/active LFO block invariance, all three destinations affect output, 128 held notes, release, panic; %u stress samples; peak %.8f; zero DSP faults\n",samples,peak);
}
