#!/usr/bin/env python3
from pathlib import Path
import hashlib, struct, sys
root=Path.cwd()
src=root/'source'
sys.path.insert(0,str(src/'base/tools'))
import fm1pkg_make as pkg

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def logical_fwsc(raw: bytes) -> bytes:
    if len(raw) < 20*0x30: raise SystemExit('old FWSC too short')
    out=bytearray(); pos=0
    for _ in range(20):
        out += raw[pos:pos+0x2f]; pos += 0x30
    out += raw[pos:]
    return bytes(out)

def extract_ota(raw: bytes) -> bytes:
    logical=logical_fwsc(raw)
    hdr=bytearray(logical[:0x40]); pkg.enc(hdr,0,0x40)
    n=struct.unpack_from('<H',hdr,8)[0]
    for i in range(n):
        e=bytearray(logical[0x40+i*0x50:0x40+(i+1)*0x50]); pkg.enc(e,0,0x50)
        typ,index,dcrc,_rsvd,off,size,aligned=struct.unpack_from('<HHHHIII',e,0)
        name=bytes(e[0x40:0x50]).split(b'\0',1)[0]
        if typ==100 or name==b'ota.bin':
            d=logical[off:off+size]
            if pkg.crc16(d)!=dcrc: raise SystemExit('old OTA CRC mismatch')
            return d
    raise SystemExit('ota.bin not found in old FWSC')

app=Path('/tmp/vstation.bin')
appb=app.read_bytes()
if hashlib.sha256(appb).hexdigest()!='d57646e15c451fe23bafbdffe3fd02fc881ee20d867ff1c1cab583b33a7b0f5e':
    raise SystemExit('application SHA mismatch')
oldfw=(root/'release/FM1-VStation-RC1.fwsc').read_bytes()
oldfull=(root/'release/FM1-VStation-RC1-M-Flasher.bin').read_bytes()
ota=extract_ota(oldfw)
pkg.SDK=src/'build-deps/sdk'
flash=pkg.flash_image(appb,pkg.KEY)
newfw=pkg.ufw(flash,ota,'FM-1_15010000')
(root/'release/FM1-VStation-RC1.fwsc').write_bytes(newfw)
newfull=bytearray(oldfull)
if len(newfull)!=0x100000: raise SystemExit('old full image size mismatch')
newfull[0x4000:0x93000]=flash[0x4000:0x93000]
(root/'release/FM1-VStation-RC1-M-Flasher.bin').write_bytes(newfull)
exp_fw='324c1b60b95c120e4dc171e7e23739612ba23f3ca66ec6be092fb8d80c0c0d11'
exp_full='9dd944c58d2ea155e93cee96805e6c72c86df7ff9ca998c1d35c0e78f91c5226'
if sha(root/'release/FM1-VStation-RC1.fwsc')!=exp_fw: raise SystemExit('new FWSC hash mismatch')
if sha(root/'release/FM1-VStation-RC1-M-Flasher.bin')!=exp_full: raise SystemExit('new M-Flasher hash mismatch')
print('FWSC',exp_fw)
print('M-Flasher',exp_full)
