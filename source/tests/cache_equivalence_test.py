"""Pending compiled regression; invoke only when a build is requested.
Compare production width/pitch caches against an uncached reference variant.
"""
import ctypes as C,json,pathlib,random,subprocess
root=pathlib.Path(__file__).resolve().parents[1];out=root/'tests/host-output';out.mkdir(exist_ok=True)
libs=[]
for name,extra in [('cached',[]),('uncached',['-DVSTATION_UNCACHED_MOD'])]:
 path=out/(name+'.so')
 subprocess.run(['gcc','-shared','-fPIC','-O2','-ffp-contract=off','-DVSTATION_TARGET',*extra,*map(str,(root/'engine').glob('*.c')),'-lm','-o',str(path)],check=True)
 lib=C.CDLL(str(path));lib.synth_init();libs.append(lib)
a=(C.c_float*256)();b=(C.c_float*256)();rng=random.Random(0x45);samples=0
for i in range(5000):
 kind=rng.randrange(11)
 if kind==0:fn='synth_param';args=(rng.randrange(132),rng.randrange(132))
 elif kind==1:fn='synth_note';args=(rng.randrange(132),rng.randrange(1,128))
 elif kind==2:fn='synth_note';args=(rng.randrange(132),0)
 elif kind==3:fn='synth_pitch_bend';args=(rng.randrange(16384),)
 elif kind==4:fn='synth_sustain';args=(rng.randrange(2),)
 elif kind==5:fn='synth_controller';args=(rng.randrange(3),rng.randrange(132))
 elif kind==6:fn='synth_reset_controllers';args=()
 elif kind==7:fn='synth_note_pressure';args=(rng.randrange(132),rng.randrange(132))
 elif kind==8:fn='synth_midi_level';args=(rng.choice((7,11)),rng.randrange(132))
 elif kind==9:fn='synth_all_notes_off';args=()
 else:fn='synth_panic';args=()
 for lib in libs:getattr(lib,fn)(*args)
 n=rng.randrange(1,257);libs[0].synth_render(a,n);libs[1].synth_render(b,n)
 assert bytes(a)[:4*n]==bytes(b)[:4*n],i;samples+=n
 assert C.c_uint32.in_dll(libs[0],'synth_faults').value==0
 assert C.c_uint32.in_dll(libs[1],'synth_faults').value==0
result={'events':5000,'samples':samples,'cached_vs_uncached_bit_identical':True}
(out/'cache_equivalence_results.json').write_text(json.dumps(result,indent=2));print(result)
