"""TEST16 pan UI/idle transfers, Init and stereo output through the actual app."""
from __future__ import annotations
import array, json, pathlib, re, subprocess, sys, wave
r=pathlib.Path(__file__).resolve().parents[1]
out=r/'tests/ui-output';out.mkdir(exist_ok=True)

def run(name: str, script: str) -> int:
    p=out/(name+'.txt');p.write_text(script)
    result=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
    match=re.search(r'(\d+) blits',result.stdout)
    assert match, result.stdout
    return int(match[1])

for page,name,steps in [(49,'pan',[(1,1),(2,-32),(3,60),(4,80)]),(50,'pan_clock',[(1,27),(2,15),(3,1),(4,80)])]:
    base=f'wait 100\ntap HOME\nspin SELECT {page}\nwait 100\n'
    before=run(name+'_idle',base)
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

# Verify the FX cycle really reaches both pages and wraps back to filter.
run('pan_fx_cycle','wait 100\n'+('tap FX\n'*10)+'wait 100\nshot pan_fx_cycle.png\ntap FX\nwait 100\nshot pan_clock_fx_cycle.png\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\ntap FX\nwait 100\nshot pan_fx_wrap.png\n')
for name in ['pan','pan_clock']:
    assert (out/f'{name}_fx_cycle.png').read_bytes()==(out/f'{name}_reference.png').read_bytes()
run('pan_filter_reference','wait 100\ntap FX\nwait 100\nshot pan_filter_reference.png\n')
assert (out/'pan_fx_wrap.png').read_bytes()==(out/'pan_filter_reference.png').read_bytes()

# Test the final application interleaving, not just the synth's planar buffers.
peaks={}
for name,enable,position in [('bypass',0,-64),('center',1,0),('left',1,-64),('right',1,63)]:
    name='pan_audio_'+name
    run(name,f'wait 100\nmaster 2048\ntap HOME\nspin SELECT 49\nspin K1 {enable}\nspin K2 {position}\nmidi USB 90 3c 64\nwait 250\nwav {name}.wav\nwait 500\nwavstop\n')
    with wave.open(str(out/(name+'.wav')),'rb') as w:
        assert w.getnchannels()==2 and w.getsampwidth()==2
        samples=array.array('h',w.readframes(w.getnframes()))
        if sys.byteorder!='little':samples.byteswap()
    left,right=samples[::2],samples[1::2]
    lpeak,rpeak=max(map(abs,left)),max(map(abs,right));peaks[name]={'left':lpeak,'right':rpeak}
    assert max(lpeak,rpeak)>0 and max(lpeak,rpeak)<32767
    if name.endswith(('bypass','center')): assert left==right
    elif name.endswith('left'): assert rpeak==0 and lpeak>0
    else: assert lpeak>0 and rpeak>100*lpeak
result={'status':'passed','pages':[49,50],'each_knob_extra_blits':1,
        'idle_and_playing_extra_blits':0,'init_restore_pixel_identical':True,
        'fx_cycle_and_wrap_verified':True,'app_stereo_output':peaks,
        'scope':'Host app frame transfers and PCM channel routing; not FM1 hardware timing or audio capture'}
(r/'tests/release-evidence/test22-pan-ui-validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
