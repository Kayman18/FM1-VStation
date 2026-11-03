#include <stdint.h>
/* Original mix 0x37915..0x37997; noise bias supplied from inspected image.
   Target float version is deliberately not bit-equivalent to desktop double. */
float voice_mix_float(float a,float b,float c,const float *g,float noise,float bias){
 float n=(g[3]+bias)*noise;
 float x=g[1]*b+g[0]*a;x+=g[2]*c;x+=n;x+=(b*a)*g[4];return x;
}
#ifndef VSTATION_TARGET
float voice_mix_double(float a,float b,float c,const float *g,float noise,float bias){
 float n=(g[3]+bias)*noise;
 double x=(double)g[1]*b+(double)g[0]*a;
 x+=(double)g[2]*c;x+=n;x+=((double)b*a)*g[4];return (float)x;
}
typedef struct {void **vtable;float captured;} Capture;
static float __attribute__((ms_abi)) capture(Capture *c,float value){c->captured=value;return value;}
static void *capture_table[2]={0,(void*)capture};
void capture_init(Capture *c){c->vtable=capture_table;c->captured=0;}
typedef void (__attribute__((ms_abi))*VoiceFn)(void*,float*);
void reference_voice(void *fn,void *voice){float out=0;((VoiceFn)fn)(voice,&out);}
#endif
