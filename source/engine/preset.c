#include "preset.h"
#include <string.h>
static uint32_t crc(const uint8_t *p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return ~c;}
unsigned preset_param_max(unsigned id){
 if(id>=SYNTH_PARAM_COUNT)return 0;
 if(id>=168)return id==168?4:127;
 return id>=163?(id==167?1:id==164?5:127):id>=160?3:id>=152?(id==159?1:id==155?19:id==157?12:127):id>=143?((id==151 || id==144)?1:id==146?34:127):id>=138?(id==142?1:id==141?34:127):id>=132?(id==137?1:id==136?34:127):id>=128?(id==131?1:127):id==127?4:(id==124 || id==126)?1:id>=122?127:id==121?1:id==120?76:id>=117?25:id==115?76:id>=113?1:id>=111?2:id>=108?127:id>=106?1:id>=103?127:id>=101?1:id==100?2:id>=98?34:id==97?240:id==96?16:id==95?3:id>=91?1:(id>=69 && id<=71)?31:id==66?2:id==67?1:(id>=58 && id<=60)?4:id==42?24:id<3?3:(id==19 || id==25)?3:((id>=62 && id<=65) || id==15 || id==44 || id==52 || id==53 || id==73 || id==84 || (id>=28 && id<=30))?1:127;
}
unsigned preset_param_min(unsigned id){return (id==115 || id==120)?52:id==97?40:(id==95 || id==168)?1:0;}
int preset_decode(const uint8_t *data,size_t size,unsigned out[SYNTH_PARAM_COUNT]){
 if(!data || !out || (size!=PRESET_BYTES && size!=PRESET_TEST40_BYTES && size!=PRESET_TEST12_BYTES && size!=PRESET_TEST13_BYTES && size!=PRESET_TEST14_BYTES && size!=PRESET_TEST15_BYTES && size!=PRESET_TEST16_BYTES && size!=PRESET_TEST17_BYTES && size!=PRESET_TEST18_BYTES && size!=PRESET_TEST20_BYTES && size!=PRESET_TEST22_BYTES))return 0;
 if(memcmp(data,"FVS1",4) || data[4]!=1 || data[6] || data[7])return 0;
 unsigned count=data[5];
 if(!((count==SYNTH_PARAM_COUNT && size==PRESET_BYTES) ||
      (count==177 && size==PRESET_TEST40_BYTES) ||
      (count==127 && size==PRESET_TEST12_BYTES) ||
      (count==128 && size==PRESET_TEST13_BYTES) ||
      (count==132 && size==PRESET_TEST14_BYTES) ||
      (count==138 && size==PRESET_TEST15_BYTES) ||
      (count==143 && size==PRESET_TEST16_BYTES) ||
      (count==152 && size==PRESET_TEST17_BYTES) ||
      (count==160 && size==PRESET_TEST18_BYTES) ||
      (count==163 && size==PRESET_TEST20_BYTES) ||
      (count==168 && size==PRESET_TEST22_BYTES)))return 0;
 uint32_t stored=0;for(unsigned i=0;i<4;i++)stored|=(uint32_t)data[size-4+i]<<(8*i);
 if(crc(data,size-4)!=stored)return 0;
 unsigned active=count>SYNTH_PARAM_COUNT?SYNTH_PARAM_COUNT:count;
 for(unsigned id=0;id<active;id++){
  unsigned hi=preset_param_max(id),lo=preset_param_min(id);
  if(data[8+id]<lo || data[8+id]>hi)return 0;
 }
 for(unsigned id=0;id<active;id++)out[id]=data[8+id];
 /* TEST12 had no note-mode control: retain its exact legacy allocation. */
 if(count==127)out[127]=0;
 /* Exact older snapshots never silently enable a newly introduced effect. */
 if(count<132){out[128]=0;out[129]=127;out[130]=64;out[131]=0;}
 if(count<138){out[132]=64;out[133]=78;out[134]=0;out[135]=0;out[136]=0;out[137]=0;}
 if(count<143){out[138]=64;out[139]=0;out[140]=0;out[141]=0;out[142]=0;}
 if(count<152){out[143]=0;out[144]=0;out[145]=0;out[146]=0;out[147]=0;out[148]=64;out[149]=64;out[150]=64;out[151]=0;}
 if(count<160){out[152]=0;out[153]=64;out[154]=0;out[155]=0;out[156]=127;out[157]=0;out[158]=64;out[159]=0;}
 if(count<163){out[160]=0;out[161]=0;out[162]=0;}
 if(count<168){out[163]=0;out[164]=0;out[165]=64;out[166]=64;out[167]=0;}
 if(count<170){out[168]=1;out[169]=0;}
 return 1;
}
int preset_apply(const uint8_t *data,size_t size){
 unsigned values[SYNTH_PARAM_COUNT];if(!preset_decode(data,size,values))return 0;
 synth_panic();
 for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++)synth_param(id,values[id]);
 synth_panic();return 1;
}

int preset_encode(const unsigned values[SYNTH_PARAM_COUNT],uint8_t *out,size_t size){
 if(!values || !out || size!=PRESET_BYTES)return 0;
 for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++)if(values[id]<preset_param_min(id) || values[id]>preset_param_max(id))return 0;
 uint8_t tmp[PRESET_BYTES]={0};memcpy(tmp,"FVS1",4);tmp[4]=1;tmp[5]=SYNTH_PARAM_COUNT;
 for(unsigned id=0;id<SYNTH_PARAM_COUNT;id++)tmp[8+id]=(uint8_t)values[id];
 uint32_t c=crc(tmp,PRESET_BYTES-4);for(unsigned i=0;i<4;i++)tmp[PRESET_BYTES-4+i]=(uint8_t)(c>>(8*i));
 memcpy(out,tmp,PRESET_BYTES);return 1;
}
