import pathlib,subprocess,re,json
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 result=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 return int(re.search(r'(\d+) blits',result.stdout)[1])
run('preset_reference','wait 100\nshot preset_reference.png\n')
base='wait 100\nspin K1 -2\ntap SEL\nwait 100\n'
a=run('preset_ready',base+'shot preset_ready.png\n')
b=run('preset_ready_idle',base+'wait 2000\n');assert a==b
loaded=base+'tap SEL\nwait 100\n'
a=run('preset_loaded',loaded+'shot preset_loaded.png\n')
b=run('preset_loaded_idle',loaded+'wait 2000\n');assert a==b
run('preset_restored',loaded+'tap HOME\nwait 100\nshot preset_restored.png\n')
assert (out/'preset_reference.png').read_bytes()==(out/'preset_restored.png').read_bytes()
report=dict(status='passed',init_controls_restored=True,ready_idle_redraws=0,loaded_idle_redraws=0)
(out/'preset-ui-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
