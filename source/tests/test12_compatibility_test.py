"""Bitwise default/legacy-policy regression against the user's TEST12 source kit.
Usage: python3 tests/test12_compatibility_test.py /path/to/TEST12-kit.zip
Only engine files are extracted; this test requires a C compiler, not the VST.
"""
from __future__ import annotations
import argparse, ctypes as C, hashlib, json, pathlib, random, subprocess, tempfile, zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]

def library(engine: pathlib.Path, target: pathlib.Path):
    subprocess.run(['gcc', '-O2', '-shared', '-fPIC', '-Wl,-Bsymbolic',
                    '-ffp-contract=off', '-DVSTATION_TARGET', '-I'+str(engine),
                    *map(str, sorted(engine.glob('*.c'))), '-lm', '-o', str(target)], check=True)
    lib = C.CDLL(str(target))
    for name in ('synth_note', 'synth_param', 'synth_controller', 'synth_note_pressure'):
        getattr(lib, name).argtypes = [C.c_uint, C.c_uint]
    for name in ('synth_sustain', 'synth_pitch_bend', 'synth_midi_level'):
        getattr(lib, name).argtypes = [C.c_uint] if name != 'synth_midi_level' else [C.c_uint,C.c_uint]
    lib.synth_render.argtypes = [C.POINTER(C.c_float), C.c_uint]
    return lib

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('test12_zip', type=pathlib.Path)
    args=parser.parse_args()
    digest=hashlib.sha256(args.test12_zip.read_bytes()).hexdigest()
    samples=events=0
    with tempfile.TemporaryDirectory(prefix='vstation-compat-') as directory:
        temp=pathlib.Path(directory);old=temp/'engine';old.mkdir()
        with zipfile.ZipFile(args.test12_zip) as archive:
            members=[m for m in archive.namelist() if '/engine/' in m and not m.endswith('/')]
            for member in members:
                name=pathlib.PurePosixPath(member).name
                (old/name).write_bytes(archive.read(member))
        if '#define SYNTH_PARAM_COUNT 127' not in (old/'synth.h').read_text():
            raise RuntimeError('Reference archive is not the expected 127-control baseline')
        a=library(old,temp/'old.so');b=library(ROOT/'engine',temp/'new.so')
        output=[(C.c_float*257)(),(C.c_float*257)()]
        rng=random.Random(0x1213)
        for voices in (1,2,3):
            for lib in (a,b):lib.synth_init();lib.synth_param(95,voices)
            for step in range(5000):
                kind=rng.randrange(12);note=rng.randrange(128)
                if kind<3:call,args2='synth_note',(note,rng.randrange(1,128))
                elif kind<5:call,args2='synth_note',(note,0)
                elif kind==5:
                    param=rng.randrange(127) # New note mode remains OFF.
                    if param==95:param=96 # Keep each comparison's voice cap.
                    call,args2='synth_param',(param,rng.randrange(128))
                elif kind==6:call,args2='synth_pitch_bend',(rng.randrange(16384),)
                elif kind==7:call,args2='synth_controller',(rng.randrange(3),rng.randrange(128))
                elif kind==8:call,args2='synth_sustain',(rng.randrange(2),)
                elif kind==9:call,args2='synth_note_pressure',(note,rng.randrange(128))
                elif kind==10:call,args2='synth_all_notes_off',()
                else:call,args2='synth_reset_controllers',()
                for lib in (a,b):getattr(lib,call)(*args2)
                count=rng.randrange(1,258)
                for lib,buf in zip((a,b),output):lib.synth_render(buf,count)
                assert bytes(output[0])[:count*4]==bytes(output[1])[:count*4],(voices,step,call,args2)
                assert a.synth_voice_count()==b.synth_voice_count()
                assert C.c_uint.in_dll(a,'synth_faults').value==C.c_uint.in_dll(b,'synth_faults').value==0
                samples+=count;events+=1
    report=dict(status='passed',test12_archive_sha256=digest,events=events,samples=samples,
                voice_limits=[1,2,3],note_mode=0,audio_bit_identical=True,dsp_faults=0,
                scope='Legacy/default-policy regression; not original VST audio parity or hardware timing')
    (ROOT/'tests/release-evidence/test18-test12-compatibility.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
