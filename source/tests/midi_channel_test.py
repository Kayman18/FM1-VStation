"""End-to-end receive-channel check through host MIDI parsing and audio output."""
import pathlib, subprocess, wave, array
r=pathlib.Path(__file__).resolve().parents[1]
out=r/'tests/host-output/channel';out.mkdir(parents=True,exist_ok=True)
def run(name,status,channel):
    script=out/(name+'.txt')
    script.write_text(f'wait 100\ntap HOME\nspin SELECT 33\nspin K2 {channel}\nwait 100\nwav {name}.wav\nmidi USB {status} 3c 64\nwait 500\nwavstop\n')
    subprocess.run([str(r/'app/build/host/vstation_host'),str(script),str(out)],check=True,capture_output=True)
    with wave.open(str(out/(name+'.wav')),'rb') as w:
        a=array.array('h',w.readframes(w.getnframes()))
    return max(map(abs,a))
assert run('wrong_channel','90',2)==0
assert run('right_channel','91',2)>0
assert run('omni_channel16','9f',0)>0
print('PASS: host MIDI channel 2 rejects channel 1, accepts channel 2; OMNI accepts channel 16')
