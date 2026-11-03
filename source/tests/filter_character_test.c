#include "synth.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
float v0_test_filter(unsigned),v1_test_filter(unsigned);
static float a[2048],b[2048];
static void check(unsigned drive,unsigned q,unsigned resonance){
 float d=1.f-drive/127.f;d=1.f-d*d;
 float strength=q/127.f;strength*=127.f;strength*=.0078125f;
 float (*get[2])(unsigned)={v0_test_filter,v1_test_filter};
 for(unsigned i=0;i<2;i++){
  assert(get[i](14)==d && get[i](13)==1.f-d*.75f);
  assert(get[i](7)==strength);
  assert(get[i](8)==1.f-strength*(resonance/127.f));
 }
}
int main(void){
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);synth_render(a,2048);
 synth_init();synth_param(95,2);synth_param(122,127);synth_param(123,127);
 synth_note(60,100);synth_note(67,100);synth_render(b,2048);
 assert(!memcmp(a,b,sizeof(a))); /* Legacy mode ignores saved new controls. */
 for(unsigned poles=0;poles<2;poles++)for(unsigned v=0;v<128;v++){
  synth_init();synth_param(95,2);synth_param(15,poles);synth_param(124,1);
  synth_param(122,v);synth_param(123,127-v);synth_param(7,v);
  check(v,127-v,v);
  synth_param(123,v);check(v,v,v); /* Q edit with unchanged resonance. */
  synth_panic();check(v,v,v);
  synth_note(60,100);synth_note(67,100);synth_render(a,2048);
  for(unsigned j=0;j<2048;j++)assert(isfinite(a[j]));
  assert(!synth_faults);
  synth_param(124,0);
  assert(v0_test_filter(14)==.1f && v1_test_filter(13)==1.f);
  assert(v0_test_filter(8)==1.f && v1_test_filter(7)==0.f);
  synth_param(124,1);check(v,v,v);
 }
 puts("PASS: filter character legacy bypass, both voices/poles, controls, resonance refresh, panic and finite audio");
}
