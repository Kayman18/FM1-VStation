"""TEST18 delay UI, frame transfers, Init restoration and actual host stereo PCM."""
from __future__ import annotations
import array,json,pathlib,re,subprocess,sys,wave
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 proc=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 match=re.search(r'(\d+) blits',proc.stdout);assert match,proc.stdout
 return int(match[1])
for page,name,steps in [(54,'delay',[(1,1),(2,50),(3,-32),(4,70)]),(55,'delay_stereo',[(1,5),(2,-50),(3,12),(4,50)]),(56,'delay_clock',[(1,10),(2,15),(3,1),(4,-32)])]:
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
run('delay_fx_cycle','wait 100\n'+('tap FX\n'*15)+'wait 100\nshot delay_fx_cycle.png\ntap FX\nwait 100\nshot delay_stereo_fx_cycle.png\ntap FX\nwait 100\nshot delay_clock_fx_cycle.png\ntap FX\ntap FX\ntap FX\ntap FX\nwait 100\nshot delay_fx_wrap.png\n')
for name in ('delay','delay_stereo','delay_clock'):
 assert (out/f'{name}_fx_cycle.png').read_bytes()==(out/f'{name}_reference.png').read_bytes(),name
run('delay_filter_reference','wait 100\ntap FX\nwait 100\nshot delay_filter_reference.png\n')
assert (out/'delay_fx_wrap.png').read_bytes()==(out/'delay_filter_reference.png').read_bytes()
peaks={};audio={}
for name,enable,ratio,width in [('bypass',0,0,127),('equal',1,0,127),('stereo',1,5,127),('mono',1,5,0)]:
 run('delay_audio_'+name,f'wait 100\nmaster 2048\ntap HOME\nspin SELECT 54\nspin K1 {enable}\nspin K2 50\nspin K3 -32\nspin K4 70\nspin SELECT 1\nspin K1 {ratio}\nspin K2 {width-127}\nmidi USB 90 3c 64\nwait 250\nwav delay_audio_{name}.wav\nwait 500\nwavstop\n')
 with wave.open(str(out/f'delay_audio_{name}.wav'),'rb') as w:
  assert w.getnchannels()==2 and w.getsampwidth()==2
  a=array.array('h',w.readframes(w.getnframes()))
  if sys.byteorder!='little':a.byteswap()
 l,rr=a[::2],a[1::2];lp,rp=max(map(abs,l)),max(map(abs,rr));assert 0<max(lp,rp)<32767
 if name=='stereo':assert l!=rr
 else:assert l==rr
 peaks[name]={'left':lp,'right':rp};audio[name]=a
assert audio['bypass']!=audio['equal'] and audio['stereo']!=audio['mono']
report=dict(status='passed',pages=[54,55,56],each_knob_extra_blits=1,idle_and_playing_extra_blits=0,init_restore_pixel_identical=True,fx_cycle_and_wrap_verified=True,app_stereo_output=peaks,scope='Host application frame transfers and PCM; not hardware timing or listening')
(r/'tests/release-evidence/test22-delay-ui-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
