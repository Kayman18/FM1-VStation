#include "synth.h"
#include "native_unison.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
unsigned synth_test_owner(unsigned);
unsigned synth_test_pending(unsigned);
int main(void){
 /* The recovered pitch helper remains valid through the original 1..8 range.
    The FM1 TEST42 shared pool is eight voices; unison itself remains 1..4. */
 for(unsigned c=1;c<=8;c++)for(unsigned d=0;d<=127;d++)for(unsigned i=0;i<c;i++){
  int num=((int)(2*(i?i-1:0))-(int)c)*256+64,den=(int)(4*c+12);float dd=(float)d/127.f;float e=i?(float)(num/den)*dd:0.f;
  assert(native_unison_pitch(c,d,i)==e);
 }
 float out[128];
 /* POLY, one voice per note: all eight physical voices are shared globally,
    then the ninth note steals the oldest complete group. */
 synth_init();assert(synth_native_pool_limit()==8);synth_param(127,3);synth_param(168,1);
 for(unsigned n=0;n<8;n++)synth_note(48+n,100);assert(synth_voice_count()==8);
 synth_note(72,100);assert(synth_voice_count()==8);synth_render(out,64);
 unsigned seen72=0;for(unsigned i=0;i<8;i++)seen72+=synth_test_owner(i)==72;assert(seen72==1);assert(!synth_faults);
 /* 4x stacked POLY: two notes consume the eight shared voices; a third steals
    one whole four-voice group, never a partial stack. */
 synth_panic();synth_param(127,3);synth_param(168,4);synth_note(48,100);assert(synth_voice_count()==4);
 synth_note(55,100);assert(synth_voice_count()==8);synth_note(62,100);assert(synth_voice_count()==8);synth_render(out,64);
 unsigned b=0;for(unsigned i=0;i<8;i++)b+=synth_test_owner(i)==62;assert(b==4);assert(!synth_faults);
 /* Legacy unison values above four still clamp to four; pool size is independent. */
 synth_panic();synth_param(127,1);synth_param(168,8);synth_note(60,100);assert(synth_voice_count()==4);synth_render(out,128);synth_note(60,0);synth_render(out,128);assert(!synth_faults);
 /* Runtime pool limit is independently selectable from 1..8. */
 synth_panic();synth_param(127,3);synth_param(168,1);synth_native_pool_limit_set(2);assert(synth_native_pool_limit()==2);
 for(unsigned n=0;n<4;n++)synth_note(40+n,100);assert(synth_voice_count()==2);synth_render(out,64);assert(!synth_faults);
 synth_native_pool_limit_set(6);assert(synth_native_pool_limit()==6);for(unsigned n=0;n<8;n++)synth_note(50+n,100);assert(synth_voice_count()==6);synth_render(out,64);assert(!synth_faults);
 synth_native_pool_limit_set(99);assert(synth_native_pool_limit()==8);
 synth_panic();synth_param(127,3);synth_param(168,4);synth_native_pool_limit_set(8);synth_note(60,100);synth_note(67,100);assert(synth_voice_count()==8);synth_render(out,64);assert(!synth_faults);
 puts("PASS: shared eight-voice pool, pool cap 1..8, 4x-unison two-group allocation and whole-group stealing");
}
