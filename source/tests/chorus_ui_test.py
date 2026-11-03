"""TEST17 chorus UI/idle transfers, Init and stereo PCM through the host app."""
from __future__ import annotations
import array,json,pathlib,re,subprocess,sys,wave
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 proc=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 match=re.search(r'(\d+) blits',proc.stdout);assert match,proc.stdout
 return int(match[1])
for page,name,steps in [(51,'chorus',[(1,1),(2,1),(3,50),(4,24)]),(52,'chorus_shape',[(1,32),(2,12),(3,-12),(4,80)]),(53,'chorus_clock',[(1,27),(2,15),(3,1),(4,80)])]:
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
run('chorus_fx_cycle','wait 100\n'+('tap FX\n'*12)+'wait 100\nshot chorus_fx_cycle.png\ntap FX\nwait 100\nshot chorus_shape_fx_cycle.png\ntap FX\nwait 100\nshot chorus_clock_fx_cycle.png\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\nwait 100\nshot chorus_fx_wrap.png\n')
for name in ('chorus','chorus_shape','chorus_clock'):
 assert (out/f'{name}_fx_cycle.png').read_bytes()==(out/f'{name}_reference.png').read_bytes(),name
run('chorus_filter_reference','wait 100\ntap FX\nwait 100\nshot chorus_filter_reference.png\n')
assert (out/'chorus_fx_wrap.png').read_bytes()==(out/'chorus_filter_reference.png').read_bytes()
peaks={};audio={}
for name,enable,kind in [('bypass',0,0),('chorus',1,0),('phaser',1,1)]:
 run('chorus_audio_'+name,f'wait 100\nmaster 2048\ntap HOME\nspin SELECT 51\nspin K1 {enable}\nspin K2 {kind}\nspin K3 50\nspin SELECT 1\nspin K1 32\nspin K4 80\nmidi USB 90 3c 64\nwait 250\nwav chorus_audio_{name}.wav\nwait 500\nwavstop\n')
 with wave.open(str(out/f'chorus_audio_{name}.wav'),'rb') as w:
  assert w.getnchannels()==2 and w.getsampwidth()==2
  a=array.array('h',w.readframes(w.getnframes()))
  if sys.byteorder!='little':a.byteswap()
 l,rr=a[::2],a[1::2];lp,rp=max(map(abs,l)),max(map(abs,rr));assert 0<max(lp,rp)<32767
 if name=='bypass':assert l==rr
 else:assert l!=rr
 peaks[name]={'left':lp,'right':rp};audio[name]=a
assert audio['bypass']!=audio['chorus'] and audio['chorus']!=audio['phaser']
report=dict(status='passed',pages=[51,52,53],each_knob_extra_blits=1,idle_and_playing_extra_blits=0,init_restore_pixel_identical=True,fx_cycle_and_wrap_verified=True,app_stereo_output=peaks,scope='Host app frame transfers and stereo PCM; not hardware timing or listening')
(r/'tests/release-evidence/test22-chorus-ui-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
