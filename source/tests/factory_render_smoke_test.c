#include "synth.h"
#include "preset.h"
#include "../app/factory_presets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void){
    unsigned values[SYNTH_PARAM_COUNT];
    uint8_t bytes[PRESET_BYTES];
    float left[128], right[128];
    unsigned unison4_count=0, fx_heavy=0;
    for(unsigned p=0;p<FACTORY_PRESET_COUNT;p++){
        synth_init();
        for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++) values[i]=factory_preset_values[p][i];
        assert(preset_encode(values,bytes,sizeof bytes));
        assert(preset_apply(bytes,sizeof bytes));
        if(values[168]==4) unison4_count++;
        if(values[131]+values[137]+values[142]+values[151]+values[159]+values[167]>=3) fx_heavy++;
        synth_note(48,100); synth_note(55,96); synth_note(60,104);
        for(unsigned block=0;block<8;block++){
            synth_render_stereo(left,right,128);
            for(unsigned i=0;i<128;i++){
                assert(isfinite(left[i]));
                assert(isfinite(right[i]));
            }
            assert(synth_faults==0);
        }
        synth_note(48,0); synth_note(55,0); synth_note(60,0);
        for(unsigned block=0;block<2;block++){
            synth_render_stereo(left,right,128);
            for(unsigned i=0;i<128;i++){
                assert(isfinite(left[i]));
                assert(isfinite(right[i]));
            }
            assert(synth_faults==0);
        }
        synth_panic();
        assert(synth_faults==0);
    }
    printf("PASS: rendered %u ARP/Motion-free factory presets (%u 4x-unison, %u with >=3 enabled FX) with finite stereo output and zero synth faults\n",
           FACTORY_PRESET_COUNT,unison4_count,fx_heavy);
    return 0;
}
