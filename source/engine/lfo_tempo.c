#include "lfo_tempo.h"
/* First twelve divisions retain their existing indices; remaining original
   periods are appended. Complete recovered range: 1/12 through 48 beats.
   LFO control runs once per 16 audio samples at 44100 Hz. Rate changes are
   control events only; no division or table lookup is added to lfo_step(). */
static uint32_t tempo_increment(float bpm,unsigned division,uint32_t free_rate){
 static const unsigned char num[]={16,8,4,2,3,1,2,3,1,1,1,1,1,1,3,4,8,3,16,6,32,12,40,18,56,20,64,24,28,30,32,36,42,48};
 static const unsigned char den[]={1,1,1,1,2,1,3,4,2,3,4,8,12,6,8,3,3,1,3,1,3,1,3,1,3,1,3,1,1,1,1,1,1,1};
 if(!division)return free_rate;
 if(division>LFO_TEMPO_DIVISIONS)division=LFO_TEMPO_DIVISIONS;
 if(bpm<40)bpm=40;if(bpm>240)bpm=240;
 unsigned i=division-1;
 float hz=bpm*(float)den[i]/(60.f*(float)num[i]);
 return (uint32_t)(hz*(4294967296.f*16.f/44100.f));
}

uint32_t lfo_tempo_increment(unsigned bpm,unsigned division,uint32_t free_rate){return tempo_increment((float)bpm,division,free_rate);}
uint32_t lfo_tempo_increment_milli(unsigned bpm,unsigned division,uint32_t free_rate){return tempo_increment((float)bpm/1000.f,division,free_rate);}
