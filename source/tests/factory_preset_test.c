#include "preset.h"
#include "../app/factory_presets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
void synth_param(unsigned id,unsigned value){(void)id;(void)value;}void synth_panic(void){}
static int natural_cmp(const char *a,const char *b){
 while(*a&&*b){
  if(isdigit((unsigned char)*a)&&isdigit((unsigned char)*b)){
   unsigned long va=0,vb=0;while(isdigit((unsigned char)*a)){va=va*10u+(unsigned)(*a-'0');a++;}while(isdigit((unsigned char)*b)){vb=vb*10u+(unsigned)(*b-'0');b++;}if(va<vb)return -1;if(va>vb)return 1;continue;
  }
  int ca=tolower((unsigned char)*a++),cb=tolower((unsigned char)*b++);if(ca<cb)return -1;if(ca>cb)return 1;
 }
 return *a?1:*b?-1:0;
}
int main(void){unsigned in[SYNTH_PARAM_COUNT],out[SYNTH_PARAM_COUNT];uint8_t bytes[PRESET_BYTES];unsigned maxuni=0;
 for(unsigned p=0;p<FACTORY_PRESET_COUNT;p++){
  if(p)assert(natural_cmp(factory_preset_names[p-1],factory_preset_names[p])<0);
  assert(factory_preset_names[p][0]);assert(factory_preset_categories[p]<FACTORY_CATEGORY_COUNT);
  for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++){in[i]=factory_preset_values[p][i];assert(in[i]>=preset_param_min(i)&&in[i]<=preset_param_max(i));}
  assert(in[127]>=1&&in[127]<=4);assert(in[168]>=1&&in[168]<=4);if(in[168]>maxuni)maxuni=in[168];
  assert(in[131]==(in[128]!=0));assert(in[137]==(in[132]!=64||in[134]!=0));assert(in[142]==(in[138]!=64||in[139]!=0));assert(in[151]==(in[143]!=0));assert(in[159]==(in[152]!=0));assert(in[167]==(in[163]!=0));
  assert(preset_encode(in,bytes,sizeof bytes));assert(preset_decode(bytes,sizeof bytes,out));assert(!memcmp(in,out,sizeof in));
  if(!strcmp(factory_preset_names[p],"Brass15") || !strcmp(factory_preset_names[p],"Brass18")) assert(in[127]==1);
  if(!strcmp(factory_preset_names[p],"Dance5") || !strcmp(factory_preset_names[p],"Dance17")) assert(in[168]==2);
 }
 assert(maxuni<=4);printf("PASS: %u RC factory patches encode/decode; requested mono/unison overrides valid; max unison %u\n",FACTORY_PRESET_COUNT,maxuni);
}
