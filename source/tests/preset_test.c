#include "preset.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned calls,panics,last[SYNTH_PARAM_COUNT];
void synth_param(unsigned id,unsigned value){assert(id<SYNTH_PARAM_COUNT);last[id]=value;++calls;}
void synth_panic(void){++panics;}
static uint32_t crc32(const uint8_t *p,unsigned n){uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return ~c;}
static void checksum(uint8_t *p,unsigned n){uint32_t c=crc32(p,n-4);for(unsigned i=0;i<4;i++)p[n-4+i]=(uint8_t)(c>>(8*i));}
int main(void){
 unsigned p[SYNTH_PARAM_COUNT],out[SYNTH_PARAM_COUNT];uint8_t bytes[PRESET_BYTES],bad[PRESET_TEST40_BYTES];
 for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++)p[i]=preset_param_min(i);
 assert(preset_encode(p,bytes,sizeof bytes));assert(preset_decode(bytes,sizeof bytes,out));assert(!memcmp(p,out,sizeof p));
 assert(preset_apply(bytes,sizeof bytes));assert(calls==SYNTH_PARAM_COUNT && panics==2);assert(!memcmp(p,last,sizeof p));
 for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++){unsigned save=p[i];p[i]=preset_param_max(i);assert(preset_encode(p,bytes,sizeof bytes));assert(preset_decode(bytes,sizeof bytes,out));assert(out[i]==p[i]);p[i]=preset_param_max(i)+1;assert(!preset_encode(p,bytes,sizeof bytes));p[i]=save;}
 /* TEST40 177-value snapshots remain importable; trailing ARP values are ignored. */
 memset(bad,0,sizeof bad);memcpy(bad,"FVS1",4);bad[4]=1;bad[5]=177;
 for(unsigned i=0;i<SYNTH_PARAM_COUNT;i++)bad[8+i]=(uint8_t)p[i];
 for(unsigned i=170;i<177;i++)bad[8+i]=(uint8_t)(17+i); /* deliberately arbitrary removed fields */
 checksum(bad,sizeof bad);memset(out,0,sizeof out);assert(preset_decode(bad,sizeof bad,out));assert(!memcmp(p,out,sizeof p));
 calls=panics=0;assert(preset_apply(bad,sizeof bad));assert(calls==SYNTH_PARAM_COUNT && panics==2);assert(!memcmp(p,last,sizeof p));
 /* TEST22 168-value layout initializes the current unison tail. */
 {unsigned n=PRESET_TEST22_BYTES;memset(bad,0,n);memcpy(bad,"FVS1",4);bad[4]=1;bad[5]=168;for(unsigned i=0;i<168;i++)bad[8+i]=(uint8_t)(i<SYNTH_PARAM_COUNT?p[i]:0);checksum(bad,n);assert(preset_decode(bad,n,out));assert(out[168]==1&&out[169]==0);}
 /* CRC and bounds are enforced. */
 assert(preset_encode(p,bytes,sizeof bytes));memcpy(bad,bytes,sizeof bytes);bad[20]^=1;assert(!preset_decode(bad,sizeof bytes,out));
 memcpy(bad,bytes,sizeof bytes);bad[8+168]=5;checksum(bad,sizeof bytes);assert(!preset_decode(bad,sizeof bytes,out));
 assert(!preset_decode(0,PRESET_BYTES,out));assert(!preset_encode(p,0,PRESET_BYTES));
 puts("PASS: 170-control ARP-free presets, TEST40 177-value import, TEST22 migration, CRC and bounds");
 return 0;
}
