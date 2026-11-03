/* Native setters 0x2b660 (drive) and 0x2b700 (Q normalization).
   Called only on control edits; no additional per-sample processing. */
void filter_character(float *s,unsigned drive,unsigned normalize,unsigned original){
 if(drive>127)drive=127;if(normalize>127)normalize=127;
 if(original){
  float d=1.f-drive/127.f;d=1.f-d*d;
  s[14]=d;s[13]=1.f-d*.75f;
  float q=normalize/127.f;q*=127.f;q*=.0078125f;
  s[7]=q;
 }else{s[14]=.1f;s[13]=1.f;s[7]=0.f;}
 /* Refresh even when resonance itself did not change. */
 s[8]=1.f-s[7]*s[6];
}
