"""Compare the new engine with a separate, unchanged TEST17 source directory.
Usage: python3 tests/test17_compatibility_test.py /path/to/TEST17-source
The delay gate remains off; original engine policies must stay identical.
"""
import argparse,ctypes as C,hashlib,json,pathlib,random,tempfile
from test12_compatibility_test import library,ROOT

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('test17_source',type=pathlib.Path)
    args=parser.parse_args();old=args.test17_source/'engine'
    if '#define SYNTH_PARAM_COUNT 152' not in (old/'synth.h').read_text():
        parser.error('Expected unchanged TEST17 source with 152 controls')
    samples=events=0;scenarios=[]
    with tempfile.TemporaryDirectory(prefix='vstation-test17-compat-') as temp:
        temp=pathlib.Path(temp);a=library(old,temp/'old.so');b=library(ROOT/'engine',temp/'new.so')
        output=[(C.c_float*257)() for _ in range(4)]
        a.synth_render_stereo.argtypes=b.synth_render_stereo.argtypes=[C.POINTER(C.c_float),C.POINTER(C.c_float),C.c_uint];rng=random.Random(0x1718)
        for mode in (0,1,2):
            for cap in (1,2,3):
                for lib in (a,b):lib.synth_init();lib.synth_param(95,cap);lib.synth_param(127,mode)
                for step in range(2500):
                    kind=rng.randrange(14);note=rng.randrange(128)
                    if kind<3:call,params='synth_note',(note,rng.randrange(1,128))
                    elif kind<5:call,params='synth_note',(note,0)
                    elif kind==5:
                        param=rng.randrange(152)
                        if param==95:param=96
                        if param==127:param=128
                        call,params='synth_param',(param,rng.randrange(128))
                    elif kind==6:call,params='synth_pitch_bend',(rng.randrange(16384),)
                    elif kind==7:call,params='synth_controller',(rng.randrange(3),rng.randrange(128))
                    elif kind==8:call,params='synth_sustain',(rng.randrange(2),)
                    elif kind==9:call,params='synth_note_pressure',(note,rng.randrange(128))
                    elif kind==10:call,params='synth_all_notes_off',()
                    elif kind==11:call,params='synth_reset_controllers',()
                    elif kind==12:call,params='synth_midi_level',(rng.choice((7,11)),rng.randrange(128))
                    else:call,params='synth_panic',()
                    for lib in (a,b):getattr(lib,call)(*params)
                    # Editing new controls while bypassed must not affect old sound.
                    b.synth_param(152+rng.randrange(7),rng.randrange(128))
                    count=rng.randrange(1,258)
                    a.synth_render_stereo(output[0],output[1],count)
                    b.synth_render_stereo(output[2],output[3],count)
                    assert all(bytes(output[i])[:count*4]==bytes(output[i+2])[:count*4] for i in range(2)),(mode,cap,step,call,params)
                    assert a.synth_voice_count()==b.synth_voice_count()
                    assert C.c_uint.in_dll(a,'synth_faults').value==C.c_uint.in_dll(b,'synth_faults').value==0
                    events+=1;samples+=count
                scenarios.append(dict(note_mode=mode,voice_cap=cap,events=2500))
    report=dict(status='passed',events=events,samples=samples,audio_bit_identical=True,dsp_faults=0,
                scenarios=scenarios,reference_engine_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(old.glob('*')) if p.is_file()},
                scope='delay-bypassed TEST17 regression; not original VST audio parity or hardware timing')
    (ROOT/'tests/release-evidence/test18-test17-compatibility.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='reference_engine_hashes'}))

if __name__=='__main__':main()
