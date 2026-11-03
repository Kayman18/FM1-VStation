#include "velocity.h"
/* RVA 0x43f36..0x44027. Preserve float operation order and signed response.
   Positive sensitivity attenuates soft notes; negative attenuates hard notes. */
float velocity_gain(unsigned velocity,float sensitivity,unsigned cubic){
 if(velocity>127)velocity=127;
 float v=velocity/127.f,curve=v*v;
 if(cubic)curve*=v;
 float amount=sensitivity*2.f;
 if(amount>0)return (curve-1.f)*amount+1.f;
 return amount*curve+1.f;
}
/* Original conversion 0x4402e..0x44053. Caller supplies bounded gain [0,1]. */
uint32_t velocity_mod_gain(float gain){return (uint32_t)(gain*32768.f);}
