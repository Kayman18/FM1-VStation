"""TEST20 phase-page edits, event-only redraws, migration/Init, actual PCM."""
import array,json,pathlib,re,subprocess,wave
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 proc=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 return int(re.search(r'(\d+) blits',proc.stdout)[1])
base='wait 100\ntap HOME\nspin SELECT 57\nwait 100\n';initial=run('fx_phase_reference',base+'shot fx_phase_reference.png\n')
for knob,delta in ((1,1),(2,2),(3,3),(4,-1)):
 s=base+f'spin K{knob} {delta}\nwait 100\n'
 assert run(f'fx_phase_edit_{knob}',s)==initial+1
 assert run(f'fx_phase_idle_{knob}',s+'wait 2000\n')==initial+1
active=base+'spin K1 1\nspin K2 2\nspin K3 3\nwait 100\n'
run('fx_phase_active',active+'shot fx_phase_active.png\n')
playing=active+'midi USB 90 3c 64\nwait 100\n'
assert run('fx_phase_playing',playing)==run('fx_phase_playing_idle',playing+'wait 2000\n')
run('fx_phase_restore',active+'tap SEL\ntap SEL\nwait 100\ntap HOME\nspin SELECT 57\nwait 100\nshot fx_phase_restored.png\n')
assert (out/'fx_phase_restored.png').read_bytes()==(out/'fx_phase_reference.png').read_bytes()
run('fx_phase_fx_cycle','wait 100\n'+('tap FX\n'*18)+'wait 100\nshot fx_phase_fx_cycle.png\ntap FX\ntap FX\ntap FX\nwait 100\nshot fx_phase_fx_wrap.png\n')
assert (out/'fx_phase_fx_cycle.png').read_bytes()==(out/'fx_phase_reference.png').read_bytes()
run('fx_phase_filter_ref','wait 100\ntap FX\nwait 100\nshot fx_phase_filter_ref.png\n')
assert (out/'fx_phase_fx_wrap.png').read_bytes()==(out/'fx_phase_filter_ref.png').read_bytes()
audio={}
for name,selector in [('off',0),('low',1),('mid',2),('high',3)]:
 script=f'wait 100\nmaster 1024\ntap HOME\nspin SELECT 49\nspin K1 1\nspin K3 90\nspin K4 75\nwait 100\ntap HOME\nspin SELECT 57\nspin K3 {selector}\nwait 100\nwav fx_phase_{name}.wav\nmidi USB 90 3c 64\nwait 300\nmidi USB 90 40 64\nwait 100\nmidi USB 80 3c 00\nmidi USB 80 40 00\nwait 100\nmidi USB 90 3c 64\nwait 300\nwavstop\n'
 run('fx_phase_audio_'+name,script)
 with wave.open(str(out/f'fx_phase_{name}.wav'),'rb') as w:
  assert w.getnchannels()==2;audio[name]=w.readframes(w.getnframes())
 assert any(audio[name])
assert len(set(audio.values()))==4
report=dict(status='passed',page=57,each_knob_extra_blits=1,idle_and_held_note_extra_blits=0,init_restore_pixel_identical=True,fx_cycle_verified=True,distinct_four_start_positions_in_actual_app_pcm=True,scope='Host UI transfers and PCM, not hardware CPU or listening')
(r/'tests/release-evidence/test22-fx-phase-ui-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
