"""Compare the new C mono adapter to recovered, previously validated Python models.
Does not execute the original VST, simulate its complete allocator, or prove sound parity.
"""
from __future__ import annotations
import ctypes as C, json, pathlib, random, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'research'))
from note_lifecycle import NoteLedger
from native_note_policy import gate_flags, glide_setup

def main():
    rng=random.Random(0x1301);events=compared=returns=duplicates=0
    with tempfile.TemporaryDirectory(prefix='vstation-mono-') as temp:
        path=pathlib.Path(temp)/'engine.so'
        subprocess.run(['gcc','-O2','-shared','-fPIC','-Wl,-Bsymbolic','-ffp-contract=off',
                        '-DVSTATION_TARGET','-DVSTATION_TEST','-I'+str(ROOT/'engine'),
                        *map(str,sorted((ROOT/'engine').glob('*.c'))),'-lm','-o',str(path)],check=True)
        lib=C.CDLL(str(path))
        lib.v0_test_glide_offset.restype=C.c_int32
        for name in ('synth_param','synth_note'):
            getattr(lib,name).argtypes=[C.c_uint,C.c_uint]
        lib.v0_test_native_state.argtypes=[C.c_uint,C.c_uint]
        lib.synth_render.argtypes=[C.POINTER(C.c_float),C.c_uint]
        audio=(C.c_float*64)()
        def state(field,index=0):return lib.v0_test_native_state(field,index)
        for mode in (0,1):
            lib.synth_init();lib.synth_param(127,mode+1)
            ledger=NoteLedger();released=pedal=0;latched=applied=0;last=255
            mask=31;shape=1;enabled=1;duration=64;preglide=0
            def settings():
                for i in range(3):lib.synth_param(62+i,(mask>>i)&1)
                for i in range(2):lib.synth_param(111+i,2 if mask&(8<<i) else 1)
                lib.synth_param(66,shape);lib.synth_param(68,duration)
                lib.synth_param(116,enabled);lib.synth_param(115,preglide+64)
                lib.synth_param(67,1-mode) # Deliberately conflicting legacy selector.
            settings()
            for step in range(20000):
                kind=rng.randrange(10);note=rng.randrange(48,73)
                # This test avoids capacity exhaustion; the dedicated C test audits it.
                if len(ledger.notes)>80:kind=8
                if kind<4:
                    velocity=rng.randrange(1,128);overlap=bool(ledger.notes)
                    if any(item[1]==note for item in ledger.notes):duplicates+=1
                    flags=gate_flags(mode,[(mask>>i)&1 for i in (3,4,0,1,2)],overlap)
                    expected=sum(flags[j]<<i for j,i in enumerate((3,4,0,1,2)))
                    offset,_=glide_setup(mode,enabled and bool(shape),duration,preglide,
                                         overlap,last,note,int(shape==1),([1]*128,[1]*128))
                    ledger.note_on(note,velocity);lib.synth_note(note,velocity)
                    assert lib.v0_test_glide_offset()==offset,(mode,step,offset)
                    latched=applied=expected;last=note;released=0
                elif kind<7:
                    if ledger.notes and rng.randrange(4):note=rng.choice(ledger.notes)[1]
                    action=ledger.note_off(note);lib.synth_note(note,0)
                    if action['action']=='return':
                        returns+=1;last=action['resumed'][1];applied=latched;released=0
                        assert lib.v0_test_glide_offset()==0
                    elif action['action']=='release':released=1
                elif kind==7:
                    pedal=rng.randrange(2);lib.synth_sustain(pedal)
                elif kind==8:
                    ledger.notes.clear();released=1;lib.synth_all_notes_off()
                else:
                    mask=rng.randrange(32);shape=rng.randrange(3);enabled=rng.randrange(2)
                    duration=rng.choice((0,64,100));preglide=rng.choice((-12,0,12));settings()
                assert state(0)==len(ledger.notes) and state(1)==released and state(7)==pedal,(mode,step)
                assert lib.v0_test_return_gates(0)==latched and lib.v0_test_return_gates(1)==applied
                for index,(_,pitch,velocity) in enumerate(ledger.notes):
                    assert state(3,index)==pitch and state(4,index)==velocity
                    compared+=1
                assert lib.synth_note_overflows()==0
                count=rng.randrange(1,65);lib.synth_render(audio,count)
                assert C.c_uint.in_dll(lib,'synth_faults').value==0
                events+=1
    report=dict(status='passed',events=events,held_entries_compared=compared,
                duplicate_presses=duplicates,mono_returns=returns,modes=['mono','mono ag'],
                dsp_faults=0,original_machine_code_executed=False,
                scope='C adapter versus recovered note-list/gate/glide models; full VST audio parity unproven')
    (ROOT/'tests/release-evidence/test13-native-mono-model.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
