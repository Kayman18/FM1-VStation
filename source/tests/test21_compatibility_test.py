"""TEST22 reverb-bypassed regression against unmodified TEST21, including real threads.
Usage: python3 tests/test21_compatibility_test.py /path/to/TEST21-source
No original plugin, sound card, or target hardware required.
"""
from __future__ import annotations
import argparse,ctypes as C,hashlib,json,pathlib,random,subprocess,tempfile
from test12_compatibility_test import library,ROOT

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('reference',type=pathlib.Path);args=ap.parse_args()
    old=args.reference/'engine'
    assert '#define SYNTH_PARAM_COUNT 163' in (old/'synth.h').read_text()
    assert 'synth_worker_config' in (old/'synth.h').read_text(), 'Reference must be unmodified TEST21'
    events=samples=0;cases=[];rng=random.Random(0x2122)
    with tempfile.TemporaryDirectory(prefix='vstation-test22-') as td:
        td=pathlib.Path(td);a=library(old,td/'old.so')
        subprocess.run(['gcc','-O2','-g','-shared','-fPIC','-Wl,-Bsymbolic','-ffp-contract=off','-DVSTATION_TARGET','-I'+str(ROOT/'engine'),*map(str,sorted((ROOT/'engine').glob('*.c'))),str(ROOT/'tests/worker_backend.c'),'-lm','-pthread','-o',str(td/'new.so')],check=True)
        b=C.CDLL(str(td/'new.so'))
        for lib in (a,b):
            lib.synth_render_stereo.argtypes=[C.POINTER(C.c_float),C.POINTER(C.c_float),C.c_uint]
            lib.synth_render.argtypes=[C.POINTER(C.c_float),C.c_uint]
        output=[(C.c_float*513)() for _ in range(4)]
        def check(count,mono=False):
            nonlocal samples
            if mono:
                a.synth_render(output[0],count);b.synth_render(output[2],count);channels=(0,)
            else:
                a.synth_render_stereo(output[0],output[1],count);b.synth_render_stereo(output[2],output[3],count);channels=(0,1)
            assert all(bytes(output[i])[:count*4]==bytes(output[i+2])[:count*4] for i in channels), context
            assert a.synth_voice_count()==b.synth_voice_count(),context
            assert C.c_uint.in_dll(a,'synth_faults').value==C.c_uint.in_dll(b,'synth_faults').value==0,context
            samples+=count
        # All modes/caps; randomized effects, controllers, panic, clocks and events.
        for backend in (1,2,3):
            for mode in (0,1,2):
                for cap in (1,2,3):
                    for lib in (a,b):lib.synth_init();lib.synth_param(95,cap);lib.synth_param(127,mode)
                    b.worker_backend_install(backend)
                    # Every existing effect is active, then gates/settings vary.
                    for lib in (a,b):
                        for i,v in ((131,1),(137,1),(142,1),(151,1),(159,1),(143,64),(147,32),(152,55),(154,72)):
                            lib.synth_param(i,v)
                    # TEST22's new reverb controls must be completely inert while its local gate is OFF.
                    for rid,rv in ((163,97),(164,5),(165,111),(166,9),(167,0)): b.synth_param(rid,rv)
                    before=b.worker_backend_count(0)
                    for step in range(1800):
                        note=rng.randrange(128);kind=rng.randrange(18)
                        if kind<4:call,params='synth_note',(note,rng.randrange(1,128))
                        elif kind<7:call,params='synth_note',(note,0)
                        elif kind==7:
                            p=rng.choice([i for i in range(163) if i not in (95,127)])
                            call,params='synth_param',(p,rng.randrange(128))
                        elif kind==8:call,params='synth_pitch_bend',(rng.randrange(16384),)
                        elif kind==9:call,params='synth_controller',(rng.randrange(3),rng.randrange(128))
                        elif kind==10:call,params='synth_sustain',(rng.randrange(2),)
                        elif kind==11:call,params='synth_note_pressure',(note,rng.randrange(128))
                        elif kind==12:call,params='synth_all_notes_off',()
                        elif kind==13:call,params='synth_reset_controllers',()
                        elif kind==14:call,params='synth_midi_level',(rng.choice((7,11)),rng.randrange(128))
                        elif kind==15:call,params='synth_clock_event',(rng.choice((0xf8,0xfa,0xfb,0xfc)),rng.randrange(1,3),step*10)
                        elif kind==16:call,params='synth_panic',()
                        else:call,params='synth_clock_poll',(step*10,)
                        context=(backend,mode,cap,step,call,params)
                        for lib in (a,b):getattr(lib,call)(*params)
                        if step%97==0:
                            b.synth_param(163,rng.randrange(128)); b.synth_param(164,rng.randrange(6)); b.synth_param(165,rng.randrange(128)); b.synth_param(166,rng.randrange(128)); b.synth_param(167,0)
                        check(rng.randrange(1,258),mono=step%7==0);events+=1
                    jobs=b.worker_backend_count(0)-before
                    assert (jobs>0)==(mode==0 and cap>1)
                    cases.append(dict(backend=backend,mode=mode,cap=cap,events=1800,jobs=jobs))
        # Force each shared-LFO configuration at both polyphony caps and every
        # chorus type, with long effect tails and deliberately odd boundaries.
        for mask in range(4):
            for cap in (2,3):
                for ctype in (0,1):
                    for lib in (a,b):
                        lib.synth_init()
                        for i,v in ((95,cap),(113,mask&1),(114,(mask>>1)&1),(20,71),(26,87),(22,40),(24,31),(131,1),(128,42),(137,1),(132,90),(134,62),(135,48),(142,1),(139,77),(140,71),(151,1),(143,64),(144,ctype),(145,70),(147,32),(149,100),(159,1),(152,55),(154,111),(157,5)):
                            lib.synth_param(i,v)
                    b.worker_backend_install(3)
                    for step in range(400):
                        if step<220:
                            note=48+(step%17)
                            for lib in (a,b):lib.synth_note(note,100 if step%3 else 0)
                            events+=1
                        elif step==220:
                            for lib in (a,b):lib.synth_all_notes_off()
                            events+=1
                        context=('shared',mask,cap,ctype,step)
                        check((1,7,15,16,17,31,32,33,64,127,128,129,256,513)[step%14],mono=step%11==0)
                    cases.append(dict(shared_lfo_mask=mask,cap=cap,chorus_type=ctype))
        # Hook refusals and malformed hook pairs must be serial-equivalent.
        for backend in (0,6,7):
            for lib in (a,b):lib.synth_init();lib.synth_note(60,100)
            b.worker_backend_install(0)
            if backend>0:b.worker_backend_bad_pair(backend==6)
            for step in range(20):context=('fallback',backend,step);check(257)
        # Mode/cap changes, Init and arbitrary rendering partitions.
        for step in range(1000):
            if step%73==0:
                for lib in (a,b):lib.synth_init()
                b.worker_backend_install(2)
            for lib in (a,b):
                if step%5==0:lib.synth_param(95,1+(step//5)%3)
                if step%11==0:lib.synth_param(127,(step//11)%3)
                lib.synth_note(40+step%50,0 if step%7==0 else 100)
            context=('mode-change',step);check(1+step%257);events+=1
        counters={name:b.worker_backend_count(i) for i,name in enumerate(('submitted','executed','joined','refused','attempts'))}
        assert counters['submitted']==counters['executed']==counters['joined']
        assert counters['refused']>0
        b.worker_backend_close()
    report=dict(status='passed',events=events,samples=samples,both_channels_bit_identical=True,mono_api_bit_identical=True,dsp_faults=0,worker_counters=counters,cases=cases,reference_engine_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(old.glob('*')) if p.is_file()},scope='TEST21 regression with TEST22 reverb gate OFF and actual pthread concurrency; not hardware startup/timing or original VST parity')
    dest=ROOT/'tests/release-evidence/test22-test21-compatibility.json';dest.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('cases','reference_engine_hashes')}))
if __name__=='__main__':main()
