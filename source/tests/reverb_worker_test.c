/* TEST22: CPU1 reverb handoff runs after the voice job and remains bit exact. */
#include "synth.h"
#include "native_reverb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void worker_backend_install(unsigned);
void worker_backend_fail_after(unsigned);
unsigned worker_backend_count(unsigned);
void worker_backend_close(void);
extern const NativeReverb *synth_test_reverb(void);
static float sl[513],sr[513],dl[513],dr[513];
static void fixture(unsigned mono){
 synth_init();if(mono)synth_param(95,1);
 unsigned p[][2]={{142,1},{138,75},{139,95},{140,97},{151,1},{143,69},{145,97},{147,49},{149,53},{159,1},{152,65},{153,1},{154,93},{157,3},{163,67},{164,5},{165,87},{166,81},{167,1}};
 for(unsigned i=0;i<sizeof(p)/sizeof(p[0]);i++)synth_param(p[i][0],p[i][1]);synth_controller(0,99);synth_note(48,100);if(!mono){synth_note(55,90);synth_note(60,110);}
}
static void all_zero(const float *p,unsigned n){for(unsigned i=0;i<n;i++)assert(p[i]==0.f);}
int main(void){
 fixture(0);synth_render_stereo(sl,sr,513);
 fixture(0);worker_backend_install(2);unsigned before=worker_backend_count(0);synth_render_stereo(dl,dr,513);assert(!memcmp(sl,dl,sizeof sl)&&!memcmp(sr,dr,sizeof sr));assert(worker_backend_count(0)-before==10);assert(!synth_worker_failures());
 fixture(0);worker_backend_install(3);before=worker_backend_count(4);synth_render_stereo(dl,dr,513);assert(!memcmp(sl,dl,sizeof sl)&&!memcmp(sr,dr,sizeof sr));assert(worker_backend_count(4)-before==10);assert(!synth_worker_failures());
 /* With one voice, the only CPU1 job is reverb: exercise executed and unexecuted wait failures. */
 for(unsigned mode=4;mode<=5;mode++){
  fixture(1);worker_backend_install(mode);worker_backend_fail_after(1);for(unsigned i=0;i<513;i++){dl[i]=17.f;dr[i]=-17.f;}synth_render_stereo(dl,dr,513);all_zero(dl,513);all_zero(dr,513);assert(synth_worker_failures()==1&&synth_faults==1&&synth_voice_count()==0);for(unsigned i=0;i<NATIVE_REVERB_LINES;i++)assert(!synth_test_reverb()->valid[i]);
 }
 worker_backend_close();puts("PASS: serial/real-thread/refused reverb renders are bit exact; CPU1 reverb failures silence the current buffer and reset state");return 0;
}
