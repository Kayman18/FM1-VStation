/* Worker ownership/failure tests. Real pthread execution; no target startup claim. */
#include "synth.h"
#include "preset.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void worker_backend_install(unsigned);
void worker_backend_fail_after(unsigned);
unsigned worker_backend_count(unsigned);
void worker_backend_close(void);
static void fixture(unsigned mask){
 synth_init();synth_param(113,mask&1);synth_param(114,(mask>>1)&1);
 unsigned p[][2]={{131,1},{128,72},{137,1},{132,91},{134,64},{135,73},
 {142,1},{138,40},{139,60},{140,77},{151,1},{143,60},{145,71},{147,32},{149,92},
 {159,1},{152,60},{154,108},{157,5},{160,1},{161,2},{162,3}};
 for(unsigned i=0;i<sizeof(p)/sizeof(p[0]);i++)synth_param(p[i][0],p[i][1]);
 synth_note(48,100);synth_note(55,110);synth_note(60,120);
}
static void silence(const float *a,unsigned n){for(unsigned i=0;i<n;i++)assert(a[i]==0.f);}
int main(void){
 float left[515],right[515];unsigned cases=0;
 uint8_t init[PRESET_BYTES];assert(preset_encode(synth_defaults,init,sizeof(init)));
 for(unsigned mask=0;mask<4;mask++)for(unsigned mode=4;mode<=5;mode++)for(unsigned after=1;after<=3;after++){
  fixture(mask);worker_backend_install(2);
  for(unsigned i=0;i<500;i++)synth_render_stereo(left,right,128); /* Wet history. */
  assert(!synth_worker_failures());
  worker_backend_install(mode);worker_backend_fail_after(after);
  unsigned before=worker_backend_count(0),joined=worker_backend_count(2);
  for(unsigned i=0;i<515;i++){left[i]=123.25f;right[i]=-123.25f;}
  synth_render_stereo(left+1,right+1,513);
  assert(left[0]==123.25f && left[514]==123.25f && right[0]==-123.25f && right[514]==-123.25f);
  silence(left+1,513);silence(right+1,513);
  assert(worker_backend_count(0)-before==after && worker_backend_count(2)-joined==after);
  assert(synth_worker_failures()==1 && synth_faults==1 && synth_voice_count()==0);
  unsigned attempts=worker_backend_count(4);
  worker_backend_install(2); /* Reconfiguration does not clear a latched failure. */
  synth_panic();assert(preset_apply(init,sizeof(init)));synth_note(64,100);
  synth_render_stereo(left,right,513);
  assert(worker_backend_count(4)==attempts && synth_worker_failures()==1 && synth_faults==1);
  assert(synth_voice_count()==1);
  synth_init();assert(synth_worker_failures()==0);synth_note(60,100);
  synth_render_stereo(left,right,513);assert(worker_backend_count(4)==attempts); /* init removes hooks. */
  worker_backend_install(2);synth_render_stereo(left,right,513);
  assert(worker_backend_count(4)>attempts && !synth_worker_failures() && !synth_faults);
  ++cases;
 }
 /* Exact scheduler bounds: no work for zero frames or monophonic policies. */
 for(unsigned mode=0;mode<3;mode++)for(unsigned cap=1;cap<=3;cap++){
  synth_init();synth_param(95,cap);synth_param(127,mode);worker_backend_install(2);
  unsigned before=worker_backend_count(4);synth_render_stereo(left,right,0);
  assert(worker_backend_count(4)==before);
  synth_note(60,100);synth_render_stereo(left,right,128);
  assert((worker_backend_count(4)>before)==(mode==0 && cap>1));
 }
 /* Unshared/no-steal 256 frames needs two handoffs, not the prior eight. */
 synth_init();worker_backend_install(2);unsigned before=worker_backend_count(0);
 synth_render_stereo(left,right,256);assert(worker_backend_count(0)-before==2);
 for(unsigned mask=1;mask<4;mask++){
  synth_init();synth_param(113,mask&1);synth_param(114,(mask>>1)&1);worker_backend_install(2);
  before=worker_backend_count(0);synth_render_stereo(left,right,256);
  assert(worker_backend_count(0)-before==16); /* Original shared-LFO boundaries. */
 }
 worker_backend_close();printf("PASS: %u completed/unexecuted/mid-render fault cases, silent current buffer, guards, latched fallback, Init/panic preservation, recovery and batching\n",cases);
 return 0;
}
