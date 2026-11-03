#include "synth.h"
#include <assert.h>
#include <stdio.h>
extern float v0_test_controller_amp(unsigned),v1_test_controller_amp(unsigned);
int main(void){
 synth_init();synth_param(95,2);synth_note(48,100);synth_note(60,100);
 assert(v0_test_controller_amp(1)==1.f);
 synth_param(108,0);synth_controller(0,127);
 assert(v0_test_controller_amp(1)==0.f && v1_test_controller_amp(1)==0.f);
 assert(v0_test_controller_amp(0)==1.f); /* event must not jump audio gain */
 float out[128];synth_render(out,128);
 assert(v0_test_controller_amp(0)>0.f && v0_test_controller_amp(0)<1.f);
 synth_reset_controllers();assert(v0_test_controller_amp(1)==1.f);
 assert(v0_test_controller_amp(0)<1.f); /* reset returns smoothly */
 synth_param(108,127);synth_param(109,127);
 synth_controller(0,127);synth_controller(1,127);
 assert(v1_test_controller_amp(1)==2.f);
 synth_param(108,64);synth_param(109,64);synth_param(110,0);
 synth_note_pressure(48,127);
 assert(v0_test_controller_amp(1)==0.f && v1_test_controller_amp(1)==1.f);
 synth_panic();assert(v0_test_controller_amp(0)==1.f);
 puts("controller amplitude: clamp, smooth reset, both voices and poly pressure passed");
}
