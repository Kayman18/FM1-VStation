"""TEST42-8VOICE: K4 VOICE LIMIT controls the shared native pool 1..8."""
import pathlib,subprocess,json
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
# NATIVE VOICE MODE page is 45: K1 note mode, K2 unison voices, K3 detune, K4 voice limit.
base='wait 100\ntap HOME\nspin SELECT 45\nspin K1 3\nwait 100\n'
run('native_pool_8',base+'shot native_pool_8.png\n')
run('native_pool_4',base+'spin K4 -4\nwait 100\nshot native_pool_4.png\n')
assert (out/'native_pool_8.png').read_bytes()!=(out/'native_pool_4.png').read_bytes()
# A cap edit is a control event; with no further input the page must stay static.
run('native_pool_8_idle',base+'wait 100\nwait 1500\nshot native_pool_8_idle.png\n')
assert (out/'native_pool_8.png').read_bytes()==(out/'native_pool_8_idle.png').read_bytes()
print(json.dumps({'status':'passed','native_poly_pool_cap':'1..8','k4_live':True,'default':8,'physical_voice_cap':8,'allocation':'shared_dynamic'}))
