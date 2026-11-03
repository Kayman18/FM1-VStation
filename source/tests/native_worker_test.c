#include "synth.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void worker_backend_install(unsigned);unsigned worker_backend_count(unsigned);void worker_backend_close(void);
static float al[4096],ar[4096],bl[4096],br[4096];
static void poly_fixture(unsigned unison){
 synth_init();synth_param(127,3);synth_param(168,unison);synth_param(169,96);synth_native_pool_limit_set(8);
 if(unison==1){for(unsigned i=0;i<8;i++)synth_note(48+i,80+i*5);}
 else{synth_note(48,100);synth_note(55,90);}
}
static void compare(unsigned unison,unsigned n){
 poly_fixture(unison);synth_render_stereo(al,ar,n);assert(!synth_faults);
 poly_fixture(unison);worker_backend_install(2);unsigned before=worker_backend_count(0);synth_render_stereo(bl,br,n);assert(!synth_faults);assert(worker_backend_count(0)>before);assert(!memcmp(al,bl,n*sizeof(float)));assert(!memcmp(ar,br,n*sizeof(float)));
}
static void compare_pool3(unsigned n){
 synth_init();synth_param(127,3);synth_param(168,1);synth_native_pool_limit_set(3);synth_note(48,100);synth_note(55,100);synth_note(60,100);synth_render_stereo(al,ar,n);assert(!synth_faults);
 synth_init();synth_param(127,3);synth_param(168,1);synth_native_pool_limit_set(3);synth_note(48,100);synth_note(55,100);synth_note(60,100);worker_backend_install(2);unsigned before=worker_backend_count(0);synth_render_stereo(bl,br,n);assert(!synth_faults);assert(worker_backend_count(0)>before);assert(!memcmp(al,bl,n*sizeof(float)));assert(!memcmp(ar,br,n*sizeof(float)));
}
int main(void){compare_pool3(1025);compare(1,513);compare(4,513);worker_backend_close();puts("PASS: native POOL3/POOL8 and 4x-unison serial/real-thread renders are bit exact");return 0;}
