#include "synth.h"
#include "pcm_output.h"
#include "preset.h"
#include "preset_catalog.h"
#include "factory_presets.h"
#include "portamento.h"
#include "perf.h"
#include "dual_core.h"
#undef C_HI
#define C_HI RGB(80,160,240)
typedef struct {unsigned kind,a,b;} VEvent;
#ifndef PLAT_HOST
static VEvent events[128] __attribute__((section(".noinit")));
#else
static VEvent events[128];
#endif
static volatile unsigned wr,rd,lost,panic_pending;
static uint8_t pending_preset[PRESET_BYTES];
static unsigned pending_values[SYNTH_PARAM_COUNT];
/* 0 idle, 1 queued/owned by audio, 2 applied awaiting UI, 3 rejected. */
static volatile unsigned preset_state;
static unsigned browser_status,browser_dirty=1,browser_index,browser_autoload_pending,native_pool_limit_ui=8;
static unsigned requested_dual=1;
static unsigned editor_custom,pending_editor_custom,pending_pool_limit=8;
static unsigned values[SYNTH_PARAM_COUNT],page,keys_before,buttons_before,ui_redraw,last_draw,midi_count;
static uint32_t app_event_peak_us,app_synth_peak_us,app_pcm_peak_us;
#ifndef PLAT_HOST
static uint32_t app_profile_now_us(void){return fm1_ticks()/FM1_TICKS_PER_US;}
#else
static uint32_t app_profile_now_us(void){return 0;}
#endif
static void app_profile_peak(uint32_t *peak,uint32_t value){if(value>*peak)*peak=value;}
static void app_profile_reset(void){app_event_peak_us=app_synth_peak_us=app_pcm_peak_us=0;synth_profile_reset();}
static void app_perf_reset_all(void){perf_reset();app_profile_reset();}
static int octave=-1,key_note[NKEYS];static float output_gain,pcm[128],pcm_right[128];
static int demo_note=-1;static unsigned demo_on,demo_left,demo_index;
static const unsigned ids[60][4]={{0,1,2,12},{3,4,5,107},{6,7,15,14},{8,9,10,11},{16,17,18,12},{19,20,21,22},{22,23,24,12},{25,26,27,22},{28,29,30,12},{31,32,33,34},{35,6,7,15},{36,37,38,101},{39,40,41,102},{42,13,14,12},{43,44,13,12},{45,14,4,5},{46,47,48,106},{49,50,51,52},{53,42,13,14},{54,55,56,57},{58,59,60,61},{62,63,64,65},{66,67,68,115},{69,70,71,62},{72,6,7,15},{73,74,35,28},{75,76,77,72},{78,79,80,42},{81,82,83,73},{84,85,24,30},{86,87,88,84},{85,89,90,84},{91,93,92,94},{95,96,42,53},{97,98,99,100},{103,104,105,102},{108,109,110,53},{111,112,91,92},{113,114,93,94},{116,66,67,68},{117,118,119,42},{120,121,49,53},{122,123,124,7},{125,54,55,56},{126,95,62,65},{127,168,169,95},{131,128,129,130},{137,132,133,134},{135,136,97,100},{142,138,139,140},{141,97,100,140},{151,144,143,150},{147,148,149,145},{146,97,100,145},{159,152,153,154},{157,156,158,152},{155,97,100,153},{160,161,162,95},{167,163,164,165},{166,163,164,165}};
enum { PARAM_PAGES=sizeof(ids)/sizeof(ids[0]), PERFORMANCE_PAGE=PARAM_PAGES, PRESET_PAGE=PARAM_PAGES+1, WORKER_PAGE=PARAM_PAGES+2, STAGE1_PAGE=PARAM_PAGES+3, STAGE2_PAGE=PARAM_PAGES+4, PAGE_COUNT=PARAM_PAGES+5 };
static const char *const titles[]={"OSCILLATORS / RING","MIX / LEVEL SCALE","FILTER / OSC3 TUNE","AMPLITUDE ENVELOPE","OSCILLATOR WIDTH","LFO / FADE","LFO DESTINATIONS","LFO 2 / FADE","LFO SOURCE ROUTING","MOD ENVELOPE","ENVELOPE / FILTER","ENV / OSC PITCH","ENV / OSC WIDTH","MIDI / TUNING","NOISE / OSC SYNC","OSC2 > OSC3 FM","FM ENVELOPE","VELOCITY DEPTHS","VELOCITY / MIDI","FINE TUNING / CENTS","OCTAVES / OSC1 TUNE","NOTE RETRIGGER","PORTAMENTO","OSC START / DEGREES","FILTER KEY TRACK","FILTER MOD CURVE","MIDI > FILTER / SEMI","MIDI > PITCH / SEMI","MIDI > FILTER LFO","PITCH LFO / OSC1","MIDI > PITCH LFO","OSC PITCH LFO DEPTH","LFO KEY SYNC","VOICES / MIDI","LFO TEMPO","OSC PWM LFO DEPTH","MIDI > AMPLITUDE","LFO FADE TRIGGER","LFO VOICE MODE","GLIDE ENABLE / MODE","OSC PITCH BEND","AMPLITUDE LEVEL","FILTER CHARACTER","VCO DRIFT / CENTS","NOTE RETURN","NATIVE VOICE MODE","NATIVE DISTORTION","NATIVE EQ","EQ MOD / CLOCK","NATIVE PAN","PAN MOD / CLOCK","NATIVE CHORUS","CHORUS / SHAPE","CHORUS / CLOCK","NATIVE DELAY","DELAY / STEREO","DELAY / CLOCK","FX PHRASE START","NATIVE REVERB","REVERB / MOD","PERFORMANCE","PRESETS","CPU WORKER","DSP STAGES 1","DSP STAGES 2"};
static const char *const labels[SYNTH_PARAM_COUNT]={"OSC1 WAVE","OSC2 WAVE","OSC3 WAVE","OSC1 LEVEL","OSC2 LEVEL","OSC3 LEVEL","CUTOFF","RESONANCE","ATTACK","DECAY","SUSTAIN","RELEASE","RING LEVEL","OSC2 SEMI","OSC3 SEMI","FILTER POLES","OSC1 WIDTH","OSC2 WIDTH","OSC3 WIDTH","LFO WAVE","LFO RATE","FADE IN","LFO > FILTER","LFO > WIDTH","LFO > PITCH","LFO2 WAVE","LFO2 RATE","LFO2 FADE","FILTER SOURCE","WIDTH SOURCE","PITCH SOURCE","MOD ATTACK","MOD DECAY","MOD SUSTAIN","MOD RELEASE","ENV > FILTER","ENV > OSC1","ENV > OSC2","ENV > OSC3","ENV > WIDTH1","ENV > WIDTH2","ENV > WIDTH3","BEND SEMI","NOISE LEVEL","OSC2 SYNC","FM DEPTH","ENV > FM","FM ATTACK","FM DECAY","VEL > AMP","VEL > MOD","VEL > FM","VEL CURVE","VEL MODE","OSC1 CENTS","OSC2 CENTS","OSC3 CENTS","MASTER CENTS","OSC1 OCTAVE","OSC2 OCTAVE","OSC3 OCTAVE","OSC1 SEMI","AMP TRIGGER","MOD TRIGGER","FM TRIGGER","LFO FADE","GLIDE SHAPE","GLIDE GATE","GLIDE ms","OSC1 START","OSC2 START","OSC3 START","KEY TRACK %","DEPTH CURVE","LFO BIPOLAR","WHEEL > FILTER","BREATH > FILTER","PRESS > FILTER","WHEEL > PITCH","BREATH > PITCH","PRESS > PITCH","WHEEL > F LFO","BREATH > F LFO","PRESS > F LFO","PITCH CURVE","OSC1 P LFO","WHEEL > P LFO","BREATH > P LFO","PRESS > P LFO","OSC2 P LFO","OSC3 P LFO","LFO1 KEY SYNC","LFO2 KEY SYNC","LFO1 PHASE","LFO2 PHASE","VOICE LIMIT","MIDI CHANNEL","TEMPO BPM","LFO1 DIVISION","LFO2 DIVISION","CLOCK SOURCE","ENV P CURVE","PWM MODE","OSC1 PWM LFO","OSC2 PWM LFO","OSC3 PWM LFO","FM ENV SCALE","MIX SCALE","WHEEL > AMP","BREATH > AMP","PRESS > AMP","LFO1 TRIGGER","LFO2 TRIGGER","LFO1 VOICES","LFO2 VOICES","PREGLIDE SEMI","GLIDE ENABLE","OSC1 BEND","OSC2 BEND","OSC3 BEND","AMP LEVEL dB","LEVEL MODE","OVERDRIVE","Q NORMALIZE","FILTER MODE","VCO DRIFT","RETURN GATES","NOTE MODE","DIST AMOUNT","DIST COMP","WHEEL DEPTH","DIST ENABLE","EQ DEPTH","EQ FREQUENCY","EQ MOD DEPTH","EQ RATE","EQ DIVISION","EQ ENABLE","PAN POSITION","PAN MOD DEPTH","PAN RATE","PAN DIVISION","PAN ENABLE","CHORUS LEVEL","CHORUS TYPE","CHORUS RATE","CHORUS DIV","CHORUS DEPTH","CHORUS CENTRE","CHORUS FDBK","CH WHEEL DEPTH","CHORUS ENABLE","DELAY LEVEL","DELAY TIME ms","DELAY FDBK","DELAY DIV","DELAY WIDTH","L/R RATIO","DL WHEEL DEPTH","DELAY ENABLE","CHORUS START","EQ START","PAN START","REVERB LEVEL","REVERB TYPE","REVERB DECAY","RV WHEEL DEPTH","REVERB ENABLE","UNISON VOICES","UNISON DETUNE"};
#define VBARRIER() __asm__ volatile("" ::: "memory")
static void number(int x,char *out){char b[16];unsigned n=0,k=0;if(x<0){out[k++]='-';x=-x;}do{b[n++]=(char)('0'+x%10);x/=10;}while(x);while(n)out[k++]=b[--n];out[k]=0;}
static void post(unsigned kind,unsigned a,unsigned b){unsigned w=wr;if(w-rd>=128){++lost;panic_pending=1;return;}events[w&127]=(VEvent){kind,a,b};VBARRIER();wr=w+1;}
/* RC browser editor protocol over USB-MIDI SysEx.
   Data bytes are 7-bit clean. Host requests use F0 7D 'V' 'S' 01 CMD ... F7.
   Values are unsigned 14-bit fields (all current synth parameters are <= 240). */
enum { ED_PROTO=1, ED_HELLO=1, ED_GET_STATE=2, ED_SET_PARAM=3, ED_LOAD_PRESET=4,
       ED_PANIC=5, ED_SET_POOL=6, ED_SET_STATE=7, ED_GET_PERF=8, ED_RESET_PERF=9,
       ED_SET_DUAL=10, ED_SET_SCAN=11, ED_R_HELLO=0x41, ED_R_STATE=0x42,
       ED_R_PARAM=0x43, ED_R_PRESET=0x44, ED_R_ACK=0x45, ED_R_POOL=0x46, ED_R_PERF=0x47 };
static uint8_t editor_tx[384];
static unsigned editor_connected;
static unsigned editor_u14(const uint8_t *p){return (unsigned)p[0]|((unsigned)p[1]<<7);}
static unsigned editor_put_u14(uint8_t *p,unsigned v){p[0]=(uint8_t)(v&127u);p[1]=(uint8_t)((v>>7)&127u);return 2;}
static unsigned editor_put_u28(uint8_t *p,uint32_t v){if(v>0x0fffffffu)v=0x0fffffffu;p[0]=(uint8_t)(v&127u);p[1]=(uint8_t)((v>>7)&127u);p[2]=(uint8_t)((v>>14)&127u);p[3]=(uint8_t)((v>>21)&127u);return 4;}
static unsigned editor_begin(unsigned cmd){editor_tx[0]=0xF0;editor_tx[1]=0x7D;editor_tx[2]='V';editor_tx[3]='S';editor_tx[4]=ED_PROTO;editor_tx[5]=(uint8_t)cmd;return 6;}
static void editor_finish(unsigned n){editor_tx[n++]=0xF7;plat_sysex_send(editor_tx,n);}
static void editor_ack(unsigned cmd,unsigned status){unsigned n=editor_begin(ED_R_ACK);editor_tx[n++]=(uint8_t)cmd;editor_tx[n++]=(uint8_t)status;editor_finish(n);}
static void editor_send_hello(void){unsigned n=editor_begin(ED_R_HELLO);editor_tx[n++]=2;n+=editor_put_u14(editor_tx+n,SYNTH_PARAM_COUNT);n+=editor_put_u14(editor_tx+n,FACTORY_PRESET_COUNT);editor_tx[n++]=31;editor_finish(n);}
static void editor_send_param(unsigned id,unsigned value){if(!editor_connected)return;unsigned n=editor_begin(ED_R_PARAM);n+=editor_put_u14(editor_tx+n,id);n+=editor_put_u14(editor_tx+n,value);editor_finish(n);}
static void editor_send_pool(void){if(!editor_connected)return;unsigned n=editor_begin(ED_R_POOL);editor_tx[n++]=(uint8_t)native_pool_limit_ui;editor_finish(n);}
static void editor_send_preset(void){if(!editor_connected)return;unsigned n=editor_begin(ED_R_PRESET);n+=editor_put_u14(editor_tx+n,browser_index);editor_finish(n);}
static void editor_send_state(void){
 if(!editor_connected)return;
 unsigned n=editor_begin(ED_R_STATE);n+=editor_put_u14(editor_tx+n,browser_index);editor_tx[n++]=(uint8_t)native_pool_limit_ui;
 for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++)n+=editor_put_u14(editor_tx+n,values[id]);
 editor_tx[n++]=(uint8_t)(editor_custom!=0);editor_finish(n);
}
static void editor_send_perf(void){
 if(!editor_connected)return;
 perf_stats_t ps;SynthProfile sp;perf_read(&ps);synth_profile_read(&sp);
 uint32_t v[]={ps.audio_pct,ps.peak_pct,ps.last_us,ps.max_us,ps.deadline_misses,ps.dropouts,ps.faults+synth_faults,lost,midi_count,
  ps.cpu0_us,ps.cpu0_peak_us,ps.worker_us,ps.worker_peak_us,ps.wait_us,ps.wait_peak_us,ps.jobs,ps.blocks,ps.seconds,ps.cpu_mhz,ps.scan_div,ps.status,ps.dual,requested_dual,
  app_event_peak_us,app_synth_peak_us,app_pcm_peak_us,sp.max_us[SYNTH_PROF_VOICES],sp.max_us[SYNTH_PROF_DISTORT],
  sp.max_us[SYNTH_PROF_AMP],sp.max_us[SYNTH_PROF_EQ],sp.max_us[SYNTH_PROF_PAN],sp.max_us[SYNTH_PROF_DELAY],sp.max_us[SYNTH_PROF_CHORUS],
  sp.max_us[SYNTH_PROF_RVB_SETUP],sp.max_us[SYNTH_PROF_RVB_MIX],sp.max_us[SYNTH_PROF_MIDI_LEVEL]};
 unsigned n=editor_begin(ED_R_PERF);for(unsigned i=0;i<sizeof(v)/sizeof(v[0]);i++)n+=editor_put_u28(editor_tx+n,v[i]);editor_finish(n);
}
/* Called only by the UI producer. One immutable snapshot may be in flight. */
int app_request_preset(const uint8_t *data,unsigned size){
 if(preset_state || wr-rd>=128)return 0;
 if(!preset_decode(data,size,pending_values))return 0;
 if(pending_values[168]>8)pending_values[168]=8;
 if(!preset_encode(pending_values,pending_preset,PRESET_BYTES))return 0;
 pending_pool_limit=native_pool_limit_ui;pending_editor_custom=0;browser_status=1;browser_dirty=1;ui_redraw=1;preset_state=1;VBARRIER();
 post(4,0,0);return 1;
}
static int app_request_factory(unsigned index){
 if(index>=FACTORY_PRESET_COUNT || preset_state || wr-rd>=128)return 0;
 for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++)pending_values[id]=factory_preset_values[index][id];
 if(pending_values[168]>8)pending_values[168]=8;
 /* These are global/runtime controls absent from the packed factory record. */
 pending_values[52]=values[52];pending_values[95]=values[95];pending_values[96]=values[96];pending_values[100]=values[100];pending_values[116]=values[116];
 if(!preset_encode(pending_values,pending_preset,PRESET_BYTES))return 0;
 pending_pool_limit=native_pool_limit_ui;pending_editor_custom=0;browser_status=1;browser_dirty=1;ui_redraw=1;preset_state=1;VBARRIER();post(4,0,0);return 1;
}
static int editor_request_state(const uint8_t *p,uint32_t n){
 if(n<9u+2u*SYNTH_PARAM_COUNT || preset_state || wr-rd>=128)return 0;
 unsigned slot=editor_u14(p+5),custom=p[7]?1u:0u,pool=p[8];if(slot>FACTORY_PRESET_COUNT)return 0;if(pool<1)pool=1;if(pool>8)pool=8;
 unsigned o=9;for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++,o+=2){unsigned v=editor_u14(p+o),lo=preset_param_min(id),hi=preset_param_max(id);if(id==168&&hi>8)hi=8;if(v<lo)v=lo;if(v>hi)v=hi;pending_values[id]=v;}
 if(!preset_encode(pending_values,pending_preset,PRESET_BYTES))return 0;
 browser_index=slot;browser_status=1;browser_dirty=1;browser_autoload_pending=0;pending_pool_limit=pool;pending_editor_custom=custom;ui_redraw=1;preset_state=1;VBARRIER();post(4,0,0);return 1;
}
static int browser_load_selected(void){
 pending_editor_custom=0;return browser_index?app_request_factory(browser_index-1):app_request_preset(init_preset,sizeof(init_preset));
}
static void browser_move(int d){
 const int span=(int)FACTORY_PRESET_COUNT+1;
 int n=((int)browser_index+d%span)%span;if(n<0)n+=span;
 browser_index=(unsigned)n;editor_custom=0;browser_status=0;browser_dirty=1;browser_autoload_pending=1;
}
unsigned app_preset_status(void){return preset_state;}
int app_sysex(const uint8_t *p,uint32_t n){
 if(n<5 || p[0]!=0x7D || p[1]!='V' || p[2]!='S' || p[3]!=ED_PROTO)return 0;
 unsigned cmd=p[4];editor_connected=1;
 if(cmd==ED_HELLO){editor_send_hello();return 1;}
 if(cmd==ED_GET_STATE){editor_send_state();return 1;}
 if(cmd==ED_GET_PERF){editor_send_perf();return 1;}
 if(cmd==ED_SET_PARAM){
  if(n<9){editor_ack(cmd,2);return 1;}
  unsigned id=editor_u14(p+5),v=editor_u14(p+7);
  if(id>=SYNTH_PARAM_COUNT || preset_state){editor_ack(cmd,id>=SYNTH_PARAM_COUNT?3:1);return 1;}
  unsigned lo=preset_param_min(id),hi=preset_param_max(id);if(id==168 && hi>8)hi=8;if(v<lo)v=lo;if(v>hi)v=hi;
  values[id]=v;editor_custom=1;post(1,id,v);ui_redraw=1;browser_dirty=1;editor_send_param(id,v);return 1;
 }
 if(cmd==ED_LOAD_PRESET){
  if(n<7){editor_ack(cmd,2);return 1;}unsigned slot=editor_u14(p+5);
  if(slot>FACTORY_PRESET_COUNT || preset_state){editor_ack(cmd,slot>FACTORY_PRESET_COUNT?3:1);return 1;}
  browser_index=slot;editor_custom=0;pending_editor_custom=0;browser_status=0;browser_dirty=1;browser_autoload_pending=0;
  if(!browser_load_selected()){editor_ack(cmd,4);return 1;}editor_ack(cmd,0);return 1;
 }
 if(cmd==ED_PANIC){post(2,0,0);editor_ack(cmd,0);return 1;}
 if(cmd==ED_SET_POOL){
  if(n<6){editor_ack(cmd,2);return 1;}unsigned v=p[5];if(v<1)v=1;if(v>8)v=8;
  native_pool_limit_ui=v;post(5,v,0);ui_redraw=1;editor_send_pool();return 1;
 }
 if(cmd==ED_SET_STATE){if(!editor_request_state(p,n)){editor_ack(cmd,preset_state?1:2);return 1;}editor_ack(cmd,0);return 1;}
 if(cmd==ED_RESET_PERF){app_perf_reset_all();editor_ack(cmd,0);editor_send_perf();return 1;}
 if(cmd==ED_SET_DUAL){
  if(n<6){editor_ack(cmd,2);return 1;}requested_dual=p[5]?1u:0u;perf_mode((int)requested_dual);app_profile_reset();editor_ack(cmd,0);editor_send_perf();return 1;
 }
 if(cmd==ED_SET_SCAN){
  if(n<6){editor_ack(cmd,2);return 1;}perf_scan_mode(p[5]>1u?4u:1u);app_profile_reset();editor_ack(cmd,0);editor_send_perf();return 1;
 }
 editor_ack(cmd,5);return 1;
}
static void preset_ui_sync(void){
 unsigned state=preset_state;VBARRIER();
 if(state==2){browser_status=2;browser_dirty=1;memcpy(values,pending_values,sizeof(values));native_pool_limit_ui=pending_pool_limit;editor_custom=pending_editor_custom;
  for(unsigned i=0;i<NKEYS;i++)key_note[i]=-1;
  ui_redraw=1;VBARRIER();preset_state=0;editor_send_preset();editor_send_pool();editor_send_state();
 }else if(state==3){browser_status=3;browser_dirty=1;ui_redraw=1;VBARRIER();preset_state=0;}
}
static void stop(void){demo_on=0;demo_note=-1;demo_left=0;synth_panic();}
static void drain(void){unsigned r=rd,w=wr;VBARRIER();while(r!=w){VEvent e=events[r++&127];if(e.kind==0)synth_note(e.a,e.b);else if(e.kind==1)synth_param(e.a,e.b);else if(e.kind==2)stop();else if(e.kind==5)synth_native_pool_limit_set(e.a);else if(e.kind==4){int ok=preset_apply(pending_preset,PRESET_BYTES);if(ok){synth_native_pool_limit_set(pending_pool_limit);demo_on=0;demo_note=-1;demo_left=0;}VBARRIER();preset_state=ok?2:3;}else{if(demo_on)stop();else{stop();demo_on=1;demo_index=0;}}}VBARRIER();rd=r;if(panic_pending){panic_pending=0;stop();}}
void app_render(int32_t *out,uint32_t frames,uint32_t gain){
 uint32_t t=app_profile_now_us();
 drain();uint32_t pkt,received_ms;int port;
 while(plat_midi_in_timed(&pkt,&port,&received_ms)){
  unsigned st=(pkt>>8)&255,a=(pkt>>16)&127,b=(pkt>>24)&127;++midi_count;
  if(st==0xf8 || st==0xfa || st==0xfb || st==0xfc){synth_clock_event(st,port==PORT_USB?1:port==PORT_TRS?2:0,received_ms);continue;}
  if(st<0xf0 && !synth_accept_channel(st&15))continue;
  if((st&240)==0x90)synth_note(a,b);else if((st&240)==0x80)synth_note(a,0);
  else if((st&240)==0xa0)synth_note_pressure(a,b);
  else if((st&240)==0xe0)synth_pitch_bend(a|(b<<7));
  else if((st&240)==0xd0)synth_controller(2,a);
  else if((st&240)==0xb0 && (a==1 || a==2))synth_controller(a==1?0:1,b);
  else if((st&240)==0xb0 && (a==7 || a==11))synth_midi_level(a,b);
  else if((st&240)==0xb0 && a==64)synth_sustain(b>=64);
  else if((st&240)==0xb0 && a==121)synth_reset_controllers();
  else if((st&240)==0xb0 && a==120)stop();
  else if((st&240)==0xb0 && a==123){demo_on=0;demo_note=-1;demo_left=0;synth_all_notes_off();}
 }
 synth_clock_poll(plat_ms());
 {uint32_t now=app_profile_now_us();if(now)app_profile_peak(&app_event_peak_us,now-t);}
 synth_profile_block_begin();
 uint32_t synth_us=0,pcm_us=0;float target=gain/4096.f;
 while(frames){unsigned n=frames>128?128:frames;
  if(demo_on){if(!demo_left){static const unsigned seq[]={36,43,48,46};if(demo_note>=0){synth_note((unsigned)demo_note,0);demo_note=-1;demo_left=4410;}else{demo_note=(int)seq[demo_index++&3];synth_note((unsigned)demo_note,100);demo_left=11025;}}if(n>demo_left)n=demo_left;}
  t=app_profile_now_us();synth_render_stereo(pcm,pcm_right,n);{uint32_t now=app_profile_now_us();if(now)synth_us+=now-t;}
  t=app_profile_now_us();for(unsigned i=0;i<n;i++){output_gain+=(target-output_gain)*.004f;out[2*i]=pcm_output(pcm[i],output_gain);out[2*i+1]=pcm_output(pcm_right[i],output_gain);}{uint32_t now=app_profile_now_us();if(now)pcm_us+=now-t;}
  if(demo_on)demo_left-=n;
  out+=2*n;frames-=n;
 }
 synth_profile_block_end();app_profile_peak(&app_synth_peak_us,synth_us);app_profile_peak(&app_pcm_peak_us,pcm_us);
}
void app_panic(void){unsigned irq=plat_lock();rd=wr;if(preset_state==1){preset_state=0;browser_status=4;browser_dirty=1;ui_redraw=1;}stop();plat_unlock(irq);}
#ifdef PLAT_HOST
void perf_read(perf_stats_t *s){memset(s,0,sizeof(*s));s->dual=requested_dual;s->status=2;s->cpu_mhz=320;s->scan_div=1;}void perf_reset(void){}void perf_mode(int dual){(void)dual;}void perf_scan_mode(unsigned div){(void)div;}
#endif
static int submit_voice_work(void (*fn)(void *),void *arg){
 /* Main-loop mode changes cannot interrupt an in-flight audio job. */
 return requested_dual?dc_submit(fn,arg):0;
}
void app_init(void){requested_dual=1;preset_state=0;browser_index=0;browser_autoload_pending=0;native_pool_limit_ui=pending_pool_limit=8;editor_custom=pending_editor_custom=0;synth_init();synth_worker_config(submit_voice_work,dc_wait);
#ifndef PLAT_HOST
 synth_profile_config(app_profile_now_us);
#else
 synth_profile_config(0);
#endif
 memcpy(values,synth_defaults,sizeof(values));for(unsigned i=0;i<NKEYS;i++)key_note[i]=-1;perf_mode(1);app_profile_reset();}
void app_say(const char *a,const char *b){(void)a;(void)b;ui_redraw=1;}
static void input(void){preset_ui_sync();touch_poll();unsigned keys=touch_keys(),buttons=plat_buttons(),down=buttons&~buttons_before;
 for(unsigned i=0;i<NKEYS;i++){
  if((keys&(1u<<i)) && !(keys_before&(1u<<i))){key_note[i]=53+(int)i+12*octave;post(0,(unsigned)key_note[i],touch_vel(i));}
  if(!(keys&(1u<<i)) && (keys_before&(1u<<i)) && key_note[i]>=0){post(0,(unsigned)key_note[i],0);key_note[i]=-1;}
 }
 if(down&(1u<<B_HOME))page=0;
 if(down&(1u<<B_SEL)){if(page!=PRESET_PAGE){page=PRESET_PAGE;browser_dirty=1;}else if(!preset_state){if(!browser_load_selected()){browser_status=3;browser_dirty=1;}}}
 if(down&(1u<<B_EDIT))page=page==1?4:page==4?14:page==14?15:page==15?19:page==19?20:page==20?43:page==43?22:page==22?39:page==39?23:page==23?40:page==40?27:page==27?29:page==29?30:page==30?31:1;
 if(down&(1u<<B_FX))page=page==2?24:page==24?25:page==25?26:page==26?28:page==28?42:page==42?46:page==46?47:page==47?48:page==48?49:page==49?50:page==50?51:page==51?52:page==52?53:page==53?54:page==54?55:page==55?56:page==56?57:page==57?58:page==58?59:2;
 if(down&(1u<<B_ENV))page=page==3?45:page==45?9:page==9?10:page==10?11:page==11?12:page==12?16:page==16?17:page==17?18:page==18?21:page==21?44:page==44?36:page==36?41:3;
 if(down&(1u<<B_GLO))page=page==PERFORMANCE_PAGE?WORKER_PAGE:page==WORKER_PAGE?STAGE1_PAGE:page==STAGE1_PAGE?STAGE2_PAGE:PERFORMANCE_PAGE;
 if(down&(1u<<B_LFO))page=page==5?6:page==6?7:page==7?8:page==8?32:page==32?34:page==34?35:page==35?37:page==37?38:5;
 if(down&(1u<<B_OCTDN))octave=clamp(octave-1,-3,2);
 if(down&(1u<<B_OCTUP))octave=clamp(octave+1,-3,2);
 if(down&(1u<<B_PLAY))post(3,0,0);
 if(down&(1u<<B_REC))post(2,0,0);
 if(down)ui_redraw=1;
 for(int k=0;k<NE;k++){int d=plat_enc(k);if(!d)continue;
  if(k==EN_PRESET){if(page!=PRESET_PAGE){page=PRESET_PAGE;browser_dirty=1;}browser_move(d);}
  else if(k==EN_SELECT && page==PRESET_PAGE)browser_move(d);
  else if(k==EN_SELECT)page=(page+PAGE_COUNT+d%PAGE_COUNT)%PAGE_COUNT;
  else if(k==EN_K1 && page==WORKER_PAGE){requested_dual=d>0;perf_mode((int)requested_dual);app_profile_reset();}
  else if(k==EN_K2 && (page==PERFORMANCE_PAGE || page==WORKER_PAGE || page==STAGE1_PAGE || page==STAGE2_PAGE)){app_perf_reset_all();}
  else if(k==EN_K3 && page==WORKER_PAGE){perf_stats_t ps;perf_read(&ps);perf_scan_mode(ps.scan_div<=1u?4u:1u);app_profile_reset();}
  else if(k>=EN_K1 && k<=EN_K4 && page<PARAM_PAGES && !preset_state){unsigned id=ids[page][k-EN_K1];
   if(id==95 && values[127]>=3){native_pool_limit_ui=(unsigned)clamp((int)native_pool_limit_ui+d,1,8);post(5,native_pool_limit_ui,0);editor_send_pool();}
   else{int hi=id==168?8:(int)preset_param_max(id);values[id]=(unsigned)clamp((int)values[id]+d,(int)preset_param_min(id),hi);editor_custom=1;browser_dirty=1;post(1,id,values[id]);editor_send_param(id,values[id]);}
  }
  ui_redraw=1;
 }
 if(page==PRESET_PAGE && browser_autoload_pending && !preset_state){
  browser_autoload_pending=0;if(!browser_load_selected()){browser_status=3;browser_dirty=1;}
 }
 keys_before=keys;buttons_before=buttons;plat_lights(((1u<<NB)-1)&~buttons,((1u<<NKEYS)-1)&~keys,0,0,0);
}
void app_input_only(void){input();}
static unsigned drawn_page=~0u,drawn_values[SYNTH_PARAM_COUNT],drawn_perf[8],drawn_worker[8],drawn_stage1[8],drawn_stage2[8],drawn_native_pool_limit=~0u;
static int drawn_octave=99;
/* TEST22: the display no longer reserves a 240x144 framebuffer in the FX pool.
   Render the same pixels as narrow vertical strips. Drawing happens only on
   the same UI invalidations as before; audio never touches this buffer. */
static unsigned strip_h(unsigned w){unsigned h=APP_CANVAS_PX/w;return h?h:1;}
/* RC V-Station-inspired 240x240 device theme mockup.  This uses only the
   drawing primitives and fonts available on the FM1, so the simulator output
   is representative of the real panel rather than desktop concept art. */
#define VS_BLUE RGB(10,74,166)
#define VS_BLUE2 RGB(24,112,224)
#define VS_NAVY RGB(5,13,29)
#define VS_PANEL RGB(12,24,44)
#define VS_PANEL2 RGB(18,36,62)
#define VS_SILVER RGB(178,188,202)
#define VS_SILVER2 RGB(110,124,143)
#define VS_TEXT RGB(232,240,252)
static void draw_header_strips(const char *title){
 unsigned sh=strip_h(240);cv_stream_begin(0,0,240,46);
 for(unsigned y=0;y<46;y+=sh){unsigned h=46-y;if(h>sh)h=sh;
  cv_begin(240,h,VS_NAVY);
  cv_rect(0,-(int)y,240,23,VS_BLUE);
  cv_rect(0,23-(int)y,240,23,VS_SILVER);
  cv_rect(0,22-(int)y,240,2,VS_BLUE2);
  cv_text(5,3-(int)y,&FONT_XS,"NOVATION",C_WHITE);
  cv_text(91,1-(int)y,&FONT_B,"V-STATION",C_WHITE);
  cv_text(7,27-(int)y,&FONT_XS,title,RGB(12,38,88));
  cv_stream_chunk();
 }
}
static void draw_full_row(unsigned screen_y,unsigned height,const char *name,const char *text){
 unsigned sh=strip_h(240);cv_stream_begin(0,screen_y,240,height);
 for(unsigned y=0;y<height;y+=sh){unsigned h=height-y;if(h>sh)h=sh;
  cv_begin(240,h,VS_NAVY);cv_rect(3,-(int)y,234,height-2,VS_PANEL);cv_rect(3,-(int)y,4,height-2,VS_BLUE2);
  cv_text(10,1-(int)y,&FONT_XS,name,VS_SILVER);cv_text(232-text_w(&FONT_S,text),0-(int)y,&FONT_S,text,VS_TEXT);cv_stream_chunk();
 }
}
static void draw_footer_strips(const char *text,const char *oct){
 unsigned sh=strip_h(240);cv_stream_begin(0,214,240,26);
 for(unsigned y=0;y<26;y+=sh){unsigned h=26-y;if(h>sh)h=sh;
  cv_begin(240,h,VS_NAVY);cv_rect(0,-(int)y,240,2,VS_BLUE2);cv_text(5,5-(int)y,&FONT_T,"FM1  RC",VS_SILVER);cv_text(65,4-(int)y,&FONT_T,text,C_WHITE);cv_text(218,3-(int)y,&FONT_S,oct,C_HI);cv_stream_chunk();
 }
}
static void draw_param_tile(unsigned x,unsigned y,const char *label,const char *value,const felucca_font_t *vf){
 unsigned sh=strip_h(120);unsigned k=1u+(x?1u:0u)+(y>46?2u:0u);char kt[3]={'K',(char)('0'+k),0};cv_stream_begin(x,y,120,84);
 for(unsigned sy=0;sy<84;sy+=sh){unsigned h=84-sy;if(h>sh)h=sh;
  cv_begin(120,h,VS_NAVY);
  cv_rect(2,-(int)sy,116,80,VS_PANEL);
  cv_rect(2,-(int)sy,116,20,VS_SILVER);
  cv_rect(2,20-(int)sy,116,3,VS_BLUE2);
  cv_rect(2,23-(int)sy,5,57,VS_BLUE);
  cv_text(7,2-(int)sy,&FONT_XS,label,RGB(8,34,82));
  cv_text(97,3-(int)sy,&FONT_T,kt,RGB(8,34,82));
  cv_text(10,36-(int)sy,vf,value,VS_TEXT);
  cv_stream_chunk();
 }
}
static void draw_preset_strips(void){
 static const char *const status[]={"READY","LOADING","LOADED","LOAD FAILED","CANCELLED"};
 const char *name=browser_index?factory_preset_names[browser_index-1]:(editor_custom?"Custom":"Init");
 const char *category=editor_custom?"EDITED / USER PATCH":(browser_index?factory_category_names[factory_preset_categories[browser_index-1]]:"INITIAL PATCH");
 char slot[16];if(browser_index)number((int)browser_index,slot);else slot[0]=0;
 unsigned sh=strip_h(240);cv_stream_begin(0,46,240,96);
 for(unsigned y=0;y<96;y+=sh){unsigned h=96-y;if(h>sh)h=sh;
  cv_begin(240,h,VS_NAVY);cv_rect(5,6-(int)y,230,66,VS_PANEL);cv_rect(5,6-(int)y,230,3,VS_BLUE2);cv_rect(5,9-(int)y,5,63,VS_BLUE);
  cv_text(14,15-(int)y,&FONT_B,name,VS_TEXT);cv_text(14,42-(int)y,&FONT_XS,category,VS_SILVER);
  if(browser_index){cv_text(184,41-(int)y,&FONT_S,slot,C_HI);cv_text(207,42-(int)y,&FONT_T,"/175",C_HI);}
  cv_rect(5,78-(int)y,230,18,RGB(8,42,88));cv_text(10,79-(int)y,&FONT_XS,status[browser_status],C_WHITE);cv_stream_chunk();
 }
 cv_stream_begin(0,142,240,98);
 for(unsigned y=0;y<98;y+=sh){unsigned h=98-y;if(h>sh)h=sh;
  cv_begin(240,h,VS_NAVY);cv_rect(5,5-(int)y,230,82,VS_PANEL2);cv_text(10,12-(int)y,&FONT_XS,"SELECT / PRESET: BROWSE",VS_TEXT);
  cv_text(10,34-(int)y,&FONT_XS,"SEL: LOAD PATCH",C_HI);cv_text(10,56-(int)y,&FONT_XS,"175 FACTORY PATCHES",VS_SILVER);cv_text(10,76-(int)y,&FONT_XS,"HOME: EXIT",VS_SILVER);cv_stream_chunk();
 }
}
static void draw(void){
 char b[32];unsigned full=drawn_page!=page,note_mode_changed=drawn_values[127]!=values[127],native_pool_changed=drawn_native_pool_limit!=native_pool_limit_ui,curve_changed=drawn_values[73]!=values[73],pitch_curve_changed=drawn_values[84]!=values[84],pwm_changed=drawn_values[102]!=values[102];
 if(full)draw_header_strips(titles[page]);
 if(page==PRESET_PAGE){
  if(full || browser_dirty){draw_preset_strips();browser_dirty=0;}drawn_page=page;return;
 }
 if(page==WORKER_PAGE){
  perf_stats_t s;perf_read(&s);
  const char *const names[]={"CPU / SCAN","CPU1 AVG us","CPU0 WAIT us","CPU0 WORK us","CPU1 MAX us","MAX WAIT us","CPU MHz","FAULT CODE"};
  unsigned mode=s.status==3?3:(!requested_dual?0:s.dual?2:1);
  unsigned v[]={mode | (s.scan_div<<8),s.worker_us,s.wait_us,s.cpu0_us,s.worker_peak_us,s.wait_peak_us,s.cpu_mhz,dc_fault()};
  static const char *const modes[]={"SINGLE","FALLBACK","DUAL","FAULT"};
  for(unsigned i=0;i<8;i++)if(full || v[i]!=drawn_worker[i]){
   const char *text=b;if(i==0){const char *m=modes[mode];unsigned n=0;while(*m&&n<sizeof(b)-5)b[n++]=*m++;b[n++]=' ';b[n++]='S';b[n++]='1';if(s.scan_div>1)b[n-1]=(char)('0'+(s.scan_div<10?s.scan_div:9));b[n]=0;text=b;}else number((int)v[i],b);
   if(full)draw_full_row(46+20*i,i==7?28:20,names[i],text);
   else{cv_begin(96,20,C_BLACK);cv_text(90-text_w(&FONT_S,text),0,&FONT_S,text,C_WHITE);cv_blit(144,46+20*i);}
   drawn_worker[i]=v[i];
  }
 }else if(page==STAGE1_PAGE){
  SynthProfile sp;synth_profile_read(&sp);
  const char *const names[]={"EVENT/MIDI MAX","VOICES MAX","DISTORT MAX","AMP LEVEL MAX","EQ MAX","SYNTH MAX","PCM OUTPUT MAX"};
  unsigned v[]={app_event_peak_us,sp.max_us[SYNTH_PROF_VOICES],sp.max_us[SYNTH_PROF_DISTORT],sp.max_us[SYNTH_PROF_AMP],sp.max_us[SYNTH_PROF_EQ],app_synth_peak_us,app_pcm_peak_us};
  for(unsigned i=0;i<7;i++)if(full || v[i]!=drawn_stage1[i]){number((int)v[i],b);if(full)draw_full_row(46+20*i,i==6?48:20,names[i],b);else{cv_begin(96,20,C_BLACK);cv_text(90-text_w(&FONT_S,b),0,&FONT_S,b,C_WHITE);cv_blit(144,46+20*i);}drawn_stage1[i]=v[i];}
 }else if(page==STAGE2_PAGE){
  SynthProfile sp;synth_profile_read(&sp);perf_stats_t ps;perf_read(&ps);
  const char *const names[]={"PAN/ROUTE MAX","DELAY MAX","CHORUS MAX","RVB SETUP MAX","RVB MIX MAX","MIDI LEVEL MAX","CPU1 MAX","MAX WAIT"};
  unsigned v[]={sp.max_us[SYNTH_PROF_PAN],sp.max_us[SYNTH_PROF_DELAY],sp.max_us[SYNTH_PROF_CHORUS],sp.max_us[SYNTH_PROF_RVB_SETUP],sp.max_us[SYNTH_PROF_RVB_MIX],sp.max_us[SYNTH_PROF_MIDI_LEVEL],ps.worker_peak_us,ps.wait_peak_us};
  for(unsigned i=0;i<8;i++)if(full || v[i]!=drawn_stage2[i]){number((int)v[i],b);if(full)draw_full_row(46+20*i,i==7?28:20,names[i],b);else{cv_begin(96,20,C_BLACK);cv_text(90-text_w(&FONT_S,b),0,&FONT_S,b,C_WHITE);cv_blit(144,46+20*i);}drawn_stage2[i]=v[i];}
 }else if(page==PERFORMANCE_PAGE){
  perf_stats_t s;perf_read(&s);
  const char *const names[]={"AUDIO AVG %","AUDIO PEAK %","MAX WORK us","DEADLINES","UNDERRUNS","FAULTS","QUEUE LOST","MIDI MSGS"};
  unsigned v[]={s.audio_pct,s.peak_pct,s.max_us,s.deadline_misses,s.dropouts,s.faults+synth_faults,lost,midi_count};
  for(unsigned i=0;i<8;i++)if(full || v[i]!=drawn_perf[i]){
   number((int)v[i],b);
   if(full)draw_full_row(46+20*i,i==7?28:20,names[i],b);
   else{cv_begin(96,20,C_BLACK);cv_text(90-text_w(&FONT_S,b),0,&FONT_S,b,C_WHITE);cv_blit(144,46+20*i);}
   drawn_perf[i]=v[i];
  }
 }else for(unsigned c=0;c<4;c++){
  unsigned id=ids[page][c];if(!full && drawn_values[id]==values[id] && !(id==95 && (note_mode_changed || native_pool_changed)) && !((id==122 || id==123) && drawn_values[124]!=values[124]) && !(id==120 && drawn_values[121]!=values[121]) && !((id==22 || id==74 || (id>=81 && id<=83)) && curve_changed) && !((id==24 || (id>=85 && id<=90)) && pitch_curve_changed) && !((id==23 || id>=103) && pwm_changed))continue;
  const char *value=b;static const char *const waves[]={"SINE","TRIANGLE","SAW","PULSE"};
  if(id>=168)number((int)values[id],b);
  else if(id==167)value=values[id]?"ON":"OFF";else if(id==164){static const char *const rooms[]={"CHAMBER","SMALL ROOM","LARGE ROOM","SMALL HALL","LARGE HALL","GRAND HALL"};value=rooms[values[id]];}else if(id==166)number((int)values[id]-64,b);else if(id==163 || id==165)number((int)values[id],b);else if(id>=160){static const char *const starts[]={"OFF","LOW","MID","HIGH"};value=starts[values[id]];}else if(id==159)value=values[id]?"ON":"OFF";else if(id==157){static const char *const ratios[]={"1/1","4/3","3/4","3/2","2/3","2/1","1/2","3/1","1/3","4/1","1/4","1/OFF","OFF/1"};value=ratios[values[id]];}else if(id==158)number((int)values[id]-64,b);else if(id==153)number((int)((values[id]*350000u+32768u)/65536u),b);else if(id>=152 && id!=155)number((int)values[id],b);else if(id==151)value=values[id]?"ON":"OFF";else if(id==144)value=values[id]?"PHASER":"CHORUS";else if(id>=148 && id<=150)number((int)values[id]-64,b);else if(id==143 || id==145 || id==147)number((int)values[id],b);else if(id==142)value=values[id]?"ON":"OFF";else if(id==138)number((int)values[id]-64,b);else if(id==139 || id==140)number((int)values[id],b);else if(id==137)value=values[id]?"ON":"OFF";else if(id==132)number((int)values[id]-64,b);else if(id==136 || id==141 || id==146 || id==155){static const char *const divisions[]={"FREE","1/12 BEAT","1/8 BEAT","1/6 BEAT","1/4 BEAT","1/3 BEAT","3/8 BEAT","1/2 BEAT","2/3 BEAT","3/4 BEAT","1 BEAT","4/3 BEAT","3/2 BEAT","2 BEAT","8/3 BEAT","3 BEAT","4 BEAT","16/3 BEAT","6 BEAT","8 BEAT","32/3 BEAT","12 BEAT","40/3 BEAT","16 BEAT","18 BEAT","56/3 BEAT","20 BEAT","64/3 BEAT","24 BEAT","28 BEAT","30 BEAT","32 BEAT","36 BEAT","42 BEAT","48 BEAT"};value=divisions[values[id]];}else if(id>=133)number((int)values[id],b);else if(id==131)value=values[id]?"ON":"OFF";else if(id==130)number((int)values[id]-64,b);else if(id>=128)number((int)values[id],b);else if(id==127){static const char *const modes[]={"FM1","MONO","MONO AG","POLY 1","POLY 2"};value=modes[values[id]];}else if(id==126)value=values[id]?"LATCHED":"LIVE";else if(id==125)number((int)values[id],b);else if(id==124)value=values[id]?"ORIGINAL":"TEST10";else if(id==122 || id==123){if(values[124])number((int)values[id],b);else value="ORIG ONLY";}else if(id==121)value=values[id]?"ORIGINAL":"LEGACY";else if(id==120){if(values[121])number((int)values[id]-64,b);else value="ORIG ONLY";}else if(id>=117){if(values[id])number((int)values[id]-13,b);else value="SHARED";}else if(id==116)value=values[id]?"ON":"OFF";else if(id==115)number((int)values[id]-64,b);else if(id>=113)value=values[id]?"COMMON":"INDEPENDENT";else if(id>=111){static const char *const trigger[]={"SHARED","LEGATO","ALWAYS"};value=trigger[values[id]];}else if(id>=108)number((int)values[id]-64,b);else if(id>=106)value=values[id]?"ORIGINAL":"LEGACY";else if(id>=103){if(values[102])number((int)values[id]-64,b);else value="ORIG ONLY";}else if(id==102)value=values[id]?"ORIGINAL":"LEGACY";else if(id==23 && values[102])value="USE PWM";else if(id==101)value=values[id]?"ORIGINAL":"LINEAR";else if(id==100){static const char *const sources[]={"INTERNAL","USB MIDI","TRS MIDI"};value=sources[values[id]];}else if(id>=98){static const char *const divs[]={"FREE","4/1","2/1","1/1","1/2","1/4 D","1/4","1/4 T","1/8 D","1/8","1/8 T","1/16","1/32","1/12 BEAT","1/6 BEAT","3/8 BEAT","4/3 BEAT","8/3 BEAT","3 BEAT","16/3 BEAT","6 BEAT","32/3 BEAT","12 BEAT","40/3 BEAT","18 BEAT","56/3 BEAT","20 BEAT","64/3 BEAT","24 BEAT","28 BEAT","30 BEAT","32 BEAT","36 BEAT","42 BEAT","48 BEAT"};value=divs[values[id]];}else if(id==97)number((int)values[id],b);else if(id==96){if(values[id])number((int)values[id],b);else value="OMNI";}else if(id==95){if(values[127]==1 || values[127]==2)value="MONO: 1";else if(values[127]>=3){b[0]='P';b[1]='O';b[2]='O';b[3]='L';b[4]=':';number((int)native_pool_limit_ui,b+5);value=b;}else number((int)values[id],b);}else if(id>=93)value=values[id]?"B":"A";else if(id>=91)value=values[id]?"RESET":"FREE";else if(id==84)value=values[id]?"ORIGINAL":"TEST6";else if(id>=85){if(values[84])number((int)values[id]-64,b);else value="ORIG ONLY";}else if(id==24 && values[84])value="USE EDIT";else if(id==22 && values[73])value="USE FX";else if(id==73)value=values[id]?"ORIGINAL":"TEST6";else if(id>=81){if(values[73])number((int)values[id]-64,b);else value="ORIG ONLY";}else if(id>=75)number((int)values[id]-64,b);else if(id==74){if(values[73])number((int)values[id]-64,b);else value="ORIG ONLY";}else if(id==72)number((int)((values[id]*100+63)/127),b);else if(id<3)value=waves[values[id]];else if(id>=69 && id<=71){if(!values[id])value="FREE";else number(((int)values[id]-1)*12,b);}else if(id==68)number((int)portamento_time_ms(values[id]),b);else if(id==66){static const char *const gl[]={"OFF","LINEAR","EXP"};value=gl[values[id]];}else if(id==67)value=values[id]?"LEGATO":"ALWAYS";else if(id>=62 && id<=65)value=values[id]?"ALWAYS":"LEGATO";else if(id>=58 && id<=60)number((int)values[id]-2,b);else if(id==52)value=values[id]?"CUBIC":"SQUARED";else if(id==53)value=values[id]?"ORIGINAL":"TEST5";else if(id==44)value=values[id]?"ON":"OFF";else if((id>=28 && id<=30))value=values[id]?"LFO 2":"LFO 1";else if(id==19 || id==25){static const char *const lw[]={"TRIANGLE","SAW","SQUARE","S&H"};value=lw[values[id]];}else number(id==61 || (id>=54 && id<=57) || (id>=49 && id<=51) || id==46 || (id>=35 && id<=41) || id==13 || id==14 || (id>=16 && id<19)?(int)values[id]-64:id==15?(values[id]?4:2):(int)values[id],b);
  draw_param_tile(120*(c%2),46+84*(c/2),labels[id],value,text_w(&FONT_L,value)<108?&FONT_L:&FONT_B);
  drawn_values[id]=values[id];
 }
 if(full || drawn_octave!=octave){number(octave,b);draw_footer_strips(page==WORKER_PAGE?"K1 CPU  K2 RESET  K3 SCAN":(page==PERFORMANCE_PAGE || page==STAGE1_PAGE || page==STAGE2_PAGE)?"GLO: NEXT  K2: RESET":"DSP TEST / PLAY: DEMO",b);drawn_octave=octave;}
 drawn_values[73]=values[73];drawn_values[84]=values[84];drawn_values[102]=values[102];drawn_values[127]=values[127];drawn_native_pool_limit=native_pool_limit_ui;drawn_page=page;
}
void app_splash(void){
 /* RC device splash: vector-only so the V-Station artwork costs almost no flash/RAM. */
 drawn_page=~0u;lcd_fill(0,0,240,240,VS_NAVY);unsigned sh=strip_h(240);cv_stream_begin(0,0,240,240);
 for(unsigned y=0;y<240;y+=sh){unsigned h=240-y;if(h>sh)h=sh;cv_begin(240,h,VS_NAVY);
  /* blue/silver V-Station frame */
  cv_rect(0,-(int)y,240,40,VS_BLUE);cv_rect(0,38-(int)y,240,3,VS_BLUE2);
  cv_rect(0,40-(int)y,240,9,VS_SILVER);cv_rect(0,190-(int)y,240,7,VS_SILVER);
  cv_rect(0,197-(int)y,240,43,VS_BLUE);cv_rect(0,196-(int)y,240,3,VS_BLUE2);
  cv_text(45,8-(int)y,&FONT_L,"NOVATION",C_WHITE);
  /* compact synth-panel illustration, feasible on the actual 240x240 LCD */
  cv_rect(18,61-(int)y,204,112,VS_SILVER);cv_rect(22,65-(int)y,196,104,VS_PANEL);
  cv_rect(22,65-(int)y,196,17,VS_BLUE);cv_text(31,66-(int)y,&FONT_T,"FM1",C_WHITE);cv_text(91,65-(int)y,&FONT_B,"V-STATION",C_WHITE);
  cv_rect(28,88-(int)y,55,73,VS_PANEL2);cv_rect(88,88-(int)y,55,73,VS_PANEL2);cv_rect(148,88-(int)y,64,73,VS_PANEL2);
  cv_text(33,91-(int)y,&FONT_T,"OSC",VS_SILVER);cv_text(94,91-(int)y,&FONT_T,"FILTER",VS_SILVER);cv_text(154,91-(int)y,&FONT_T,"FX / ENV",VS_SILVER);
  for(unsigned k=0;k<3;k++){int yy=111+(int)k*15;cv_rect(34,yy-(int)y,9,9,VS_BLUE2);cv_rect(51,yy-(int)y,22,5,VS_SILVER2);cv_rect(96,yy-(int)y,9,9,VS_BLUE2);cv_rect(113,yy-(int)y,22,5,VS_SILVER2);cv_rect(156,yy-(int)y,9,9,VS_BLUE2);cv_rect(173,yy-(int)y,29,5,VS_SILVER2);}
  cv_text(31,204-(int)y,&FONT_L,"V-STATION",C_WHITE);cv_text(73,229-(int)y,&FONT_T,"RC  LOADING",VS_TEXT);
  cv_stream_chunk();
 }
}
void app_ui_init(void){drawn_page=~0u;drawn_octave=99;palette_set(3);touch_on(1);ui_redraw=1;}
void app_frame(void){input();unsigned now=plat_ms();if((ui_redraw && now-last_draw>=33)||((page==PERFORMANCE_PAGE || page==WORKER_PAGE || page==STAGE1_PAGE || page==STAGE2_PAGE) && now-last_draw>=250)){ui_redraw=0;last_draw=now;draw();}}
