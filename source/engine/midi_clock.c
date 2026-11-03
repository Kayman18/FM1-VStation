#include "midi_clock.h"
#include <string.h>
void midi_clock_reset(MidiClock *c){memset(c,0,sizeof(*c));}
void midi_clock_poll(MidiClock *c,uint32_t ms){
 if(c->seen && (uint32_t)(ms-c->last)>500){c->seen=c->locked=c->ticks=c->count=c->cursor=0;c->sum=0;}
}
void midi_clock_event(MidiClock *c,unsigned status,uint32_t ms){
 if(status==0xfc){c->stopped=1;c->locked=c->seen=c->ticks=c->count=c->cursor=0;c->sum=0;return;}
 if(status==0xfa || status==0xfb){midi_clock_reset(c);return;}
 if(status!=0xf8 || c->stopped)return;
 midi_clock_poll(c,ms);
 if(!c->seen){c->seen=1;c->first=c->last=ms;c->ticks=0;return;}
 if(ms==c->last)return; /* Duplicate timestamp, not another measurable tick. */
 c->last=ms;
 if(++c->ticks<24)return;
 uint32_t span=ms-c->first;c->first=ms;c->ticks=0;
 /* One quarter-note window at 24 PPQN. +/-1 ms boundary tolerance. */
 if(span<249 || span>1501){c->locked=c->count=c->cursor=0;c->sum=0;return;}
 /* Average up to four adjacent beats. A >8% period change starts a new
    window immediately, avoiding a four-beat ramp after a large tempo jump. */
 if(c->count){uint32_t mean=c->sum/c->count,diff=span>mean?span-mean:mean-span;
  if(diff*100u>mean*8u){c->count=c->cursor=0;c->sum=0;}
 }
 if(c->count==4)c->sum-=c->spans[c->cursor];else c->count++;
 c->spans[c->cursor]=span;c->sum+=span;c->cursor=(c->cursor+1)&3u;
 c->bpm_milli=(60000000u*c->count+c->sum/2)/c->sum;
 if(c->bpm_milli<40000)c->bpm_milli=40000;if(c->bpm_milli>240000)c->bpm_milli=240000;
 c->locked=1;
}
