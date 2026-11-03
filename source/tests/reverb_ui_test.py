"""TEST22 reverb UI, streamed-frame transfers, Init restoration and host stereo PCM."""
from __future__ import annotations
import array,json,pathlib,re,subprocess,sys,wave
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True);ev=r/'tests/release-evidence';ev.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 proc=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 match=re.search(r'(\d+) blits',proc.stdout);assert match,proc.stdout
 return int(match[1])
for page,name,steps in [(58,'reverb',[(1,1),(2,50),(3,4),(4,20)]),(59,'reverb_mod',[(1,16),(2,50),(3,5),(4,-20)])]:
 base=f'wait 100\ntap HOME\nspin SELECT {page}\nwait 100\n';before=run(name+'_idle',base)
 for knob,step in steps:
  script=base+f'spin K{knob} {step}\nwait 100\n'
  assert run(f'{name}_k{knob}',script)==before+1
  assert run(f'{name}_k{knob}_wait',script+'wait 2000\n')==before+1
 run(name+'_reference',base+f'shot {name}_reference.png\n')
 active=base+''.join(f'spin K{k} {n}\n' for k,n in steps)+'wait 100\n'
 run(name+'_active',active+f'shot {name}.png\n')
 run(name+'_restore',active+f'tap SEL\ntap SEL\nwait 100\ntap HOME\nspin SELECT {page}\nwait 100\nshot {name}_restored.png\n')
 assert (out/f'{name}_reference.png').read_bytes()==(out/f'{name}_restored.png').read_bytes()
 playing=active+'midi USB 90 3c 64\nwait 200\n'
 assert run(name+'_playing',playing)==run(name+'_playing_idle',playing+'wait 2000\n')
run('reverb_fx_cycle','wait 100\n'+('tap FX\n'*19)+'wait 100\nshot reverb_fx_cycle.png\ntap FX\nwait 100\nshot reverb_mod_fx_cycle.png\ntap FX\nwait 100\nshot reverb_fx_wrap.png\n')
assert (out/'reverb_fx_cycle.png').read_bytes()==(out/'reverb_reference.png').read_bytes()
assert (out/'reverb_mod_fx_cycle.png').read_bytes()==(out/'reverb_mod_reference.png').read_bytes()
run('reverb_filter_reference','wait 100\ntap FX\nwait 100\nshot reverb_filter_reference.png\n')
assert (out/'reverb_fx_wrap.png').read_bytes()==(out/'reverb_filter_reference.png').read_bytes()
peaks={};audio={}
for name,enable,room,decay in [('bypass',0,0,64),('chamber',1,0,80),('grand',1,5,105)]:
 run('reverb_audio_'+name,f'wait 100\nmaster 1536\ntap HOME\nspin SELECT 58\nspin K1 {enable}\nspin K2 72\nspin K3 {room}\nspin K4 {decay-64}\nmidi USB 90 3c 64\nwait 250\nwav reverb_audio_{name}.wav\nwait 900\nwavstop\n')
 with wave.open(str(out/f'reverb_audio_{name}.wav'),'rb') as w:
  assert w.getnchannels()==2 and w.getsampwidth()==2
  a=array.array('h',w.readframes(w.getnframes()))
  if sys.byteorder!='little':a.byteswap()
 l,rr=a[::2],a[1::2];lp,rp=max(map(abs,l)),max(map(abs,rr));assert 0<max(lp,rp)<32767
 if name=='bypass':assert l==rr
 else:assert l!=rr
 peaks[name]={'left':lp,'right':rp};audio[name]=a
assert audio['bypass']!=audio['chamber'] and audio['chamber']!=audio['grand']
report=dict(status='passed',pages=[58,59],each_knob_extra_blits=1,idle_and_playing_extra_blits=0,init_restore_pixel_identical=True,fx_cycle_and_wrap_verified=True,app_stereo_output=peaks,scope='Host app frame transfers and stereo PCM; not hardware timing or listening')
(ev/'test22-reverb-ui-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
