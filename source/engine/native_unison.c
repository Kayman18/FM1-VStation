#include "native_unison.h"
float native_unison_pitch(unsigned count,unsigned detune,unsigned voice_index){
 if(count<1)count=1;if(count>8)count=8;if(detune>127)detune=127;
 if(!voice_index || voice_index>=count)return 0.f;
 unsigned i=voice_index-1;int numerator=((int)(2*i)-(int)count)*256+64;
 int denominator=(int)(4*count+12),q=numerator/denominator;
 float d=(float)detune/127.f;return (float)q*d;
}
