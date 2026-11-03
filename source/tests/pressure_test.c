#include "synth.h"
#include <assert.h>
#include <stdio.h>
unsigned v0_test_pressure(void),v1_test_pressure(void);
static float out[256];
int main(void){
 synth_init();synth_param(95,2);synth_controller(2,50);synth_note(60,100);synth_note(67,100);synth_render(out,256);
 synth_note_pressure(60,100);synth_note_pressure(67,30);
 assert(v0_test_pressure()==100 && v1_test_pressure()==30);
 synth_controller(2,80);assert(v0_test_pressure()==100 && v1_test_pressure()==30);
 synth_note(60,100);assert(v0_test_pressure()==80); /* Repeated note clears its override. */
 synth_note(72,100);synth_note_pressure(72,90);
 assert(v1_test_pressure()==30); /* Pending expression must not affect outgoing note. */
 synth_note_pressure(67,127);assert(v1_test_pressure()==30); /* Stolen owner ignored. */
 synth_render(out,64);assert(v1_test_pressure()==90);
 synth_controller(2,40);assert(v0_test_pressure()==40 && v1_test_pressure()==90);
 synth_reset_controllers();assert(!v0_test_pressure() && !v1_test_pressure());
 synth_param(95,1);synth_controller(2,20);synth_note(60,100);synth_note_pressure(60,110);
 synth_note(67,100);assert(v0_test_pressure()==20);
 synth_note_pressure(60,45);assert(v0_test_pressure()==20);
 synth_note(67,0);assert(v0_test_pressure()==45); /* Mono return restores older note pressure. */
 synth_sustain(1);synth_note(60,0);synth_note_pressure(60,0);assert(!v0_test_pressure());
 synth_sustain(0);synth_controller(2,100);assert(!v0_test_pressure()); /* Release retains override. */
 synth_note(60,100);assert(v0_test_pressure()==100);
 synth_note_pressure(61,127);assert(v0_test_pressure()==100); /* Unknown note ignored. */
 synth_note_pressure(60,999);assert(v0_test_pressure()==127);
 synth_panic();synth_note_pressure(60,127);assert(!v0_test_pressure());
 synth_init();synth_param(95,2);synth_note(60,100);synth_note(67,100);synth_render(out,256);
 synth_note(72,100);synth_note_pressure(72,90);synth_reset_controllers();synth_render(out,64);
 assert(!v0_test_pressure()); /* Reset also clears delayed expression. */
 synth_note(74,100);synth_note_pressure(74,80);synth_note(74,0);synth_render(out,128);
 synth_note(76,100);assert(!v1_test_pressure()); /* Canceled steal cannot leak pressure. */
 assert(!synth_faults);
 puts("PASS: independent pressure, channel fallback, repeated-note reset, pending steal/cancel, mono return, sustain and reset");
}
