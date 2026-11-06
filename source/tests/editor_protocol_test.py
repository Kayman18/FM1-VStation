#!/usr/bin/env python3
from pathlib import Path
import re,json
root=Path(__file__).resolve().parents[1]
app=(root/'app/app.c').read_text(); hdr=(root/'app/app.h').read_text(); web=(root/'web-editor/index.html').read_text()
assert '#define APP_SYSEX' in hdr
assert 'int app_sysex(' in app
for token in ['ED_HELLO=1','ED_GET_STATE=2','ED_SET_PARAM=3','ED_LOAD_PRESET=4','ED_PANIC=5','ED_SET_POOL=6','ED_SET_STATE=7','ED_GET_PERF=8','ED_RESET_PERF=9','ED_SET_DUAL=10','ED_SET_SCAN=11','ED_R_PERF=0x47']:
    assert token in app, token
m=re.search(r'static uint8_t editor_tx\[(\d+)\]',app); assert m and int(m.group(1))>=351
assert 'if(n<9u+2u*SYNTH_PARAM_COUNT' in app
assert 'editor_put_u28' in app
assert 'editor_tx[n++]=(uint8_t)(editor_custom!=0)' in app
pm=re.search(r'const PARAMS=(\[.*?\]);\nconst PRESETS=',web,re.S); assert pm
params=json.loads(pm.group(1)); assert len(params)==170
pr=re.search(r'const PRESETS=(\[.*?\]);\nconst ENUMS=',web,re.S); assert pr
presets=json.loads(pr.group(1)); assert len(presets)==175
assert all('motion' not in p['name'].lower() for p in presets)
assert all('arp' not in p['name'].lower() for p in presets)
assert not re.search(r'ARPE?G|ARP CTRL|ARP ENABLE|ARP PATTERN',web,re.I)
assert 'navigator.requestMIDIAccess({sysex:true})' in web
assert "firmware===2?'RC':firmware===1?'RC1':firmware===42?'TEST42 8V':'TEST'+firmware" in web
for phrase in ['FM1-VStation-Preset','SAVE CURRENT JSON','LOAD JSON TO FM1','presetSearch','categoryFilter','COMPARE','REVERT','RANDOMIZE','FM1 Hardware','liveTelemetry','RESET COUNTERS','User Preset Library','SAVE CURRENT TO LIBRARY','UPDATE SELECTED','EXPORT LIBRARY','IMPORT LIBRARY','localStorage']:
    assert phrase in web, phrase
assert "sysex(3,[...enc14(id),...enc14(v)])" in web
assert "sysex(4,enc14(slot))" in web
assert "sysex(7,stateArgs" in web
assert "sysex(8)" in web
assert "sysex(9)" in web
assert "sysex(10,[+$('#dualMode').value])" in web
assert "sysex(11,[+$('#scanMode').value])" in web
assert "data.values.length!==170&&data.values.length!==177" in web
assert "slice(0,170)" in web
state_frame_bytes=351
bulk_frame_bytes=351
perf_fields=36
perf_frame_bytes=6 + perf_fields*4 + 1
assert perf_frame_bytes <= int(m.group(1))
print({'status':'passed','protocol_version':1,'firmware':'RC','parameters':170,'presets':175,'state_frame_bytes':state_frame_bytes,'bulk_state_frame_bytes':bulk_frame_bytes,'perf_fields':perf_fields,'perf_frame_bytes':perf_frame_bytes,'json_presets':True,'legacy_test40_json_import':True,'persistent_user_library':True,'searchable_factory_presets':True,'compare_revert_randomize':True,'live_telemetry':True,'arp_ui_removed':True,'shared_pool':'1..8'})
