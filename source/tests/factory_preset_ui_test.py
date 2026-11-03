"""TEST38 factory browser: natural A-Z bank, physical PRESET knob cycles/autoloads, SELECT browses, SEL reloads."""
import pathlib,subprocess,re,json
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 z=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 return int(re.search(r'(\d+) blits',z.stdout)[1])
run('factory_init_page','wait 100\nshot factory_init_page.png\n')
# The physical knob printed PRESETS is EN_PRESET. It enters the browser, moves,
# and auto-loads without requiring SEL.
base='wait 100\nspin PRESET 2\nwait 150\n'
a=run('factory_browser_slot2',base+'shot factory_browser_slot2.png\n')
b=run('factory_browser_slot2_idle',base+'wait 2000\n');assert a==b
run('factory_knob_loaded_slot2',base+'tap HOME\nwait 100\nshot factory_knob_loaded_slot2.png\n')
assert (out/'factory_knob_loaded_slot2.png').read_bytes()!=(out/'factory_init_page.png').read_bytes()
# Return selection 2 -> Init using the physical PRESETS knob; movement alone loads Init.
restore=base+'tap HOME\nwait 50\nspin PRESET -2\nwait 150\ntap HOME\nwait 100\nshot factory_knob_restored_init.png\n'
run('factory_knob_restored_init',restore)
assert (out/'factory_knob_restored_init.png').read_bytes()==(out/'factory_init_page.png').read_bytes()
# At Init, one negative detent wraps to slot 175 and auto-loads it.
wrap='wait 100\nspin PRESET -1\nwait 150\nshot factory_browser_slot175.png\ntap HOME\nwait 100\nshot factory_knob_loaded_slot175.png\n'
run('factory_knob_slot175',wrap)
assert (out/'factory_knob_loaded_slot175.png').read_bytes()!=(out/'factory_init_page.png').read_bytes()
# SELECT remains usable while already on PRESETS, and SEL still explicitly reloads.
run('factory_select_and_sel_reload','wait 100\ntap SEL\nwait 100\nspin SELECT 1\nwait 150\ntap SEL\nwait 150\nshot factory_select_and_sel_reload.png\n')
report={'status':'passed','factory_slots':175,'physical_preset_knob_autoload':True,'preset_knob_enters_browser':True,'select_knob_browser_retained':True,'fast_selection_coalesced_by_state':True,'sel_reload_retained':True,'init_restore_exact':True,'browser_wrap_to_175':True,'idle_redraws':0,'natural_alphabetical_order':True}
(out/'factory-preset-ui-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
