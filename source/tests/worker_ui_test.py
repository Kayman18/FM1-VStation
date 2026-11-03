"""TEST19 CPU-worker controls/navigation and exact actual-app PCM in both modes."""
from __future__ import annotations
import json,pathlib,re,subprocess,wave
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
app_src=(r/'app/app.c').read_text(); m=re.search(r'static const unsigned ids\[(\d+)\]\[4\]',app_src); assert m
worker_page=int(m.group(1))+2
stage1_page=int(m.group(1))+3
stage2_page=int(m.group(1))+4
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 proc=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 match=re.search(r'(\d+) blits',proc.stdout);assert match,proc.stdout
 return int(match[1])
base='wait 100\ntap GLO\ntap GLO\nwait 100\n'
initial=run('worker_dual',base+'shot worker_dual.png\n')
assert run('worker_idle',base+'wait 2000\n')==initial
assert run('worker_single',base+'spin K1 -1\nwait 100\nshot worker_single.png\n')==initial+1
assert run('worker_single_idle',base+'spin K1 -1\nwait 2100\n')==initial+1
assert run('worker_back_dual',base+'spin K1 -1\nwait 100\nspin K1 1\nwait 100\nshot worker_back_dual.png\n')==initial+2
assert (out/'worker_dual.png').read_bytes()==(out/'worker_back_dual.png').read_bytes()
run('worker_select','wait 100\ntap HOME\nspin SELECT '+str(worker_page)+'\nwait 100\nshot worker_select.png\n')
assert (out/'worker_select.png').read_bytes()==(out/'worker_dual.png').read_bytes()
run('worker_stage1',base+'tap GLO\nwait 100\nshot worker_stage1.png\n')
run('worker_stage2',base+'tap GLO\ntap GLO\nwait 100\nshot worker_stage2.png\n')
run('worker_glo_wrap',base+'tap GLO\ntap GLO\ntap GLO\nwait 100\nshot worker_glo_wrap.png\n')
run('worker_performance','wait 100\ntap GLO\nwait 100\nshot worker_performance.png\n')
assert (out/'worker_glo_wrap.png').read_bytes()==(out/'worker_performance.png').read_bytes()
assert (out/'worker_stage1.png').read_bytes()!=(out/'worker_stage2.png').read_bytes()
run('worker_stage1_select','wait 100\ntap HOME\nspin SELECT '+str(stage1_page)+'\nwait 100\nshot worker_stage1_select.png\n')
run('worker_stage2_select','wait 100\ntap HOME\nspin SELECT '+str(stage2_page)+'\nwait 100\nshot worker_stage2_select.png\n')
assert (out/'worker_stage1_select.png').read_bytes()==(out/'worker_stage1.png').read_bytes()
assert (out/'worker_stage2_select.png').read_bytes()==(out/'worker_stage2.png').read_bytes()
run('worker_init_retains_mode',base+'spin K1 -1\ntap SEL\ntap SEL\nwait 100\ntap GLO\ntap GLO\nwait 100\nshot worker_init_retains_mode.png\n')
assert (out/'worker_init_retains_mode.png').read_bytes()==(out/'worker_single.png').read_bytes()
# Same event timings, two real host app render paths, with all effects active.
audio={}
for name,delta in [('single',-1),('dual',1)]:
 script=base+f'spin K1 {delta}\nwait 100\ntap HOME\nmaster 1024\n'
 for page,steps in [(46,[(1,1),(2,50)]),(47,[(1,1),(2,20),(4,32)]),(48,[(1,72)]),(49,[(1,1),(3,60),(4,72)]),(51,[(1,1),(3,50)]),(52,[(1,32),(4,72)]),(54,[(1,1),(2,50),(4,75)]),(58,[(1,1),(2,72),(3,5),(4,32)])]:
  script+='tap HOME\nspin SELECT '+str(page)+'\n'+''.join(f'spin K{k} {v}\n' for k,v in steps)
 script+='midi USB 90 30 64\nmidi USB 90 37 64\nmidi USB 90 3c 64\nwait 250\n'
 script+=f'wav worker_{name}.wav\nwait 300\nmidi USB b0 01 65\nwait 300\nmidi USB b0 7b 00\nwait 400\nwavstop\n'
 run('worker_audio_'+name,script)
 with wave.open(str(out/f'worker_{name}.wav'),'rb') as w:
  assert w.getnchannels()==2;audio[name]=w.readframes(w.getnframes())
assert audio['single']==audio['dual'] and any(audio['dual'])
report=dict(status='passed',worker_page=worker_page,stage1_page=stage1_page,stage2_page=stage2_page,glo_navigation=True,mode_switch_extra_blits=1,unchanged_host_counters_extra_blits=0,init_retains_cpu_mode=True,app_single_dual_pcm_identical=True,pcm_bytes=len(audio['dual']),scope='Host app uses a synchronous worker; actual pthread tests are separate. Hardware counters and CPU speed not measured.')
(r/'tests/release-evidence/test26-worker-ui.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
