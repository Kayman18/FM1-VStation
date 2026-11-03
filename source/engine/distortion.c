#include "distortion.h"
#include <string.h>
/* RVA 0x274d0, 0x2d9d0, 0x3d1e7. Keep the fifth power's multiplication/rounding order. */
static void update(Distortion *s){
 float x=s->wheel_depth*s->wheel;
 x*=2.f;x+=s->amount;
 if(x<0.f)x=0.f;else if(x>1.f)x=1.f;
 float a=1.f-x,b=a*a;b*=a;b*=a;b*=a;
 s->target=1.f-b;
}
void distortion_init(Distortion *s){memset(s,0,sizeof(*s));s->compensation=0x1.e8240cp-1f;}
void distortion_amount(Distortion *s,float x){s->amount=x;update(s);}
void distortion_depth(Distortion *s,float x){s->wheel_depth=x;update(s);}
void distortion_wheel(Distortion *s,float x){s->wheel=x;update(s);}
/* RVA 0x2d990. These are float constants, not a generic compressor curve. */
void distortion_compensation(Distortion *s,float x){s->compensation=x*0x1.378034p-2f+0x1.4c63f2p-1f;}
/* RVA 0x4300c..0x43086; distortion precedes the native master-gain multiply.
   No software doubles, lookup tables, or per-sample parameter conversion. */
float distortion_step(Distortion *s,float input){
 s->smoothed=s->target*.125f+s->smoothed*.875f;
 uint32_t bits;memcpy(&bits,&s->smoothed,sizeof(bits));
 if(!(bits&0x7f800000u) && (bits&0x007fffffu))s->smoothed=0.f;
 float magnitude;memcpy(&bits,&input,sizeof(bits));bits&=0x7fffffffu;memcpy(&magnitude,&bits,sizeof(bits));
 float numerator=(1.f-s->smoothed*s->compensation)*input;
 float denominator=(magnitude-0x1.ffff04p-1f)*s->smoothed+1.f;
 return numerator/denominator;
}
