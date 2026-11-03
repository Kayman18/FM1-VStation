"""Check display transfer counts: unchanged UI must not be repainted."""
import pathlib,subprocess,re,json
r=pathlib.Path(__file__).resolve().parents[1];out=r/'tests/ui-output';out.mkdir(exist_ok=True)
def run(name,script):
 p=out/(name+'.txt');p.write_text(script)
 result=subprocess.run([str(r/'app/build/host/vstation_host'),str(p),str(out)],check=True,capture_output=True,text=True)
 return int(re.search(r'(\d+) blits',result.stdout)[1])
initial=run('initial','wait 100\n')
idle=run('idle','wait 2100\n');assert initial==idle
changed=run('changed','wait 100\nspin K1 1\nwait 100\n');assert changed==initial+1
perf=run('perf','wait 100\ntap GLO\nwait 100\n')
perf_idle=run('perf_idle','wait 100\ntap GLO\nwait 2100\n');assert perf==perf_idle
perf_changed=run('perf_changed','wait 100\ntap GLO\nwait 100\nmidi USB 90 30 64\nwait 500\n');assert perf_changed==perf+1
run('pages','wait 100\ntap GLO\nwait 100\nshot performance.png\ntap EDIT\nwait 100\ntap EDIT\nwait 100\nshot width.png\ntap HOME\nwait 100\nshot home.png\n')
run('lfo_pages','wait 100\ntap LFO\nwait 100\nshot lfo.png\ntap LFO\nwait 100\nshot destinations.png\ntap LFO\nwait 100\nshot lfo2.png\ntap LFO\nwait 100\nshot routing.png\n')
run('extensions','wait 100\ntap EDIT\ntap EDIT\ntap EDIT\nwait 100\nshot noise_sync.png\ntap LFO\nspin K1 3\nwait 100\nshot sample_hold.png\n')
run('envelopes','wait 100\ntap ENV\nwait 100\ntap ENV\nwait 100\nshot mod_adsr.png\ntap ENV\nwait 100\nshot mod_filter.png\ntap ENV\nwait 100\nshot mod_pitch.png\ntap ENV\nwait 100\nshot mod_width.png\n')
run('fm','wait 100\ntap EDIT\ntap EDIT\ntap EDIT\ntap EDIT\nwait 100\nspin K1 100\nwait 100\nshot fm.png\n')
run('fm_env','wait 100\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\nwait 100\nspin K1 -40\nwait 100\nshot fm_envelope.png\n')
run('velocity','wait 100\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\nwait 100\nshot velocity.png\ntap ENV\nwait 100\nspin K1 1\nwait 100\nshot velocity_mode.png\n')
run('tuning','wait 100\ntap EDIT\ntap EDIT\ntap EDIT\ntap EDIT\ntap EDIT\nwait 100\nspin K1 -20\nwait 100\nshot fine_tuning.png\ntap EDIT\nwait 100\nspin K1 1\nwait 100\nshot octave_tuning.png\n')
run('retrigger','wait 100\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\ntap ENV\nwait 100\nspin K1 -1\nspin K2 -1\nspin K3 -1\nspin K4 -1\nwait 100\nshot retrigger.png\n')
run('portamento','wait 100\ntap HOME\nspin SELECT 22\nwait 100\nspin K1 2\nwait 100\nshot portamento.png\n')
run('start_phase','wait 100\ntap HOME\nspin SELECT 23\nwait 100\nspin K1 16\nspin K2 8\nwait 100\nshot start_phase.png\n')
run('tracking','wait 100\ntap FX\ntap FX\nwait 100\nspin K1 127\nwait 100\nshot filter_tracking.png\n')
run('filter_curve','wait 100\ntap FX\ntap FX\ntap FX\nwait 100\nspin K1 1\nspin K2 -32\nwait 100\nshot filter_curve.png\n')
run('filter_controllers','wait 100\ntap FX\ntap FX\ntap FX\ntap FX\nwait 100\nspin K1 24\nspin K2 -12\nspin K3 12\nwait 100\nshot filter_controllers.png\n')
run('pitch_controllers','wait 100\ntap HOME\nspin SELECT 27\nwait 100\nspin K1 12\nspin K2 -7\nspin K3 3\nwait 100\nshot pitch_controllers.png\n')
curve_page='wait 100\ntap HOME\nspin SELECT 28\nwait 100\n'
curve_idle=run('controller_lfo_idle',curve_page)
curve_switch=run('controller_lfo_switch',curve_page+'spin K4 1\nwait 100\nshot controller_filter_lfo.png\n')
assert curve_switch==curve_idle+4 # Three ORIG ONLY fields plus curve mode.
curve_switch_idle=run('controller_lfo_switch_idle',curve_page+'spin K4 1\nwait 2100\n')
assert curve_switch_idle==curve_switch
pitch_curve_page='wait 100\ntap HOME\nspin SELECT 29\nwait 100\n'
pitch_curve_idle=run('pitch_curve_idle',pitch_curve_page)
pitch_curve_switch=run('pitch_curve_switch',pitch_curve_page+'spin K1 1\nwait 100\nshot pitch_curve.png\n')
assert pitch_curve_switch==pitch_curve_idle+3
pitch_routes_page='wait 100\ntap HOME\nspin SELECT 30\nwait 100\n'
pitch_routes_idle=run('pitch_routes_idle',pitch_routes_page)
pitch_routes_switch=run('pitch_routes_switch',pitch_routes_page+'spin K4 1\nwait 100\nshot controller_vibrato.png\n')
assert pitch_routes_switch==pitch_routes_idle+4
assert run('pitch_routes_wait',pitch_routes_page+'spin K4 1\nwait 2100\n')==pitch_routes_switch
osc_depth_page='wait 100\ntap HOME\nspin SELECT 31\nwait 100\n'
osc_depth_idle=run('osc_depth_idle',osc_depth_page)
osc_depth_active=run('osc_depth_active',osc_depth_page+'spin K4 1\nwait 100\n')
assert osc_depth_active==osc_depth_idle+4
run('osc_depths',osc_depth_page+'spin K4 1\nspin K1 16\nspin K2 -16\nspin K3 6\nwait 100\nshot oscillator_pitch_lfo.png\n')
run('key_sync','wait 100\ntap HOME\nspin SELECT 32\nwait 100\nspin K1 1\nspin K3 1\nspin K4 1\nwait 100\nshot lfo_key_sync.png\n')
run('voices_midi','wait 100\ntap HOME\nspin SELECT 33\nwait 100\nshot voices_midi.png\nspin K2 2\nwait 100\nshot receive_channel.png\n')
run('lfo_tempo','wait 100\ntap HOME\nspin SELECT 34\nwait 100\nshot lfo_tempo.png\nspin K1 20\nspin K2 6\nspin K3 9\nwait 100\nshot lfo_tempo_divisions.png\n')
run('clock_source','wait 100\ntap HOME\nspin SELECT 34\nspin K4 1\nwait 100\nshot clock_usb.png\nspin K4 1\nwait 100\nshot clock_trs.png\n')
for index, name in [(35,'pwm_depths'),(36,'controller_amp'),(37,'lfo_triggers'),(38,'lfo_common')]:
 script=f'wait 100\ntap HOME\nspin SELECT {index}\nwait 100\n'
 if index==35:script+='spin K4 1\nwait 100\n'
 before=run(name+'_idle',script)
 after=run(name+'_change',script+'spin K1 1\nwait 100\nshot '+name+'.png\n')
 assert after==before+1
 assert run(name+'_wait',script+'spin K1 1\nwait 2100\n')==after
glide_page='wait 100\ntap HOME\nspin SELECT 39\nwait 100\n'
glide_idle=run('glide_enable_idle',glide_page)
glide_off=run('glide_enable_off',glide_page+'spin K1 -1\nwait 100\nshot glide_enable.png\n')
assert glide_off==glide_idle+1
assert run('glide_enable_wait',glide_page+'spin K1 -1\nwait 2100\n')==glide_off
bend_page='wait 100\ntap HOME\nspin SELECT 40\nwait 100\n'
bend_idle=run('osc_bend_idle',bend_page)
bend_set=run('osc_bend_set',bend_page+'spin K1 1\nwait 100\nshot oscillator_bend.png\n')
assert bend_set==bend_idle+1
assert run('osc_bend_wait',bend_page+'spin K1 1\nwait 2100\n')==bend_set
amp_page='wait 100\ntap HOME\nspin SELECT 41\nwait 100\n'
amp_idle=run('amp_level_idle',amp_page)
amp_on=run('amp_level_on',amp_page+'spin K2 1\nwait 100\nshot amp_level.png\n')
assert amp_on==amp_idle+2
assert run('amp_level_wait',amp_page+'spin K2 1\nwait 2100\n')==amp_on
character_page='wait 100\ntap HOME\nspin SELECT 42\nwait 100\n'
character_idle=run('filter_character_idle',character_page)
character_on=run('filter_character_on',character_page+'spin K3 1\nwait 100\nshot filter_character.png\n')
assert character_on==character_idle+3
assert run('filter_character_wait',character_page+'spin K3 1\nwait 2100\n')==character_on
drift_page='wait 100\ntap HOME\nspin SELECT 43\nwait 100\n'
drift_idle=run('drift_idle',drift_page)
drift_on=run('drift_on',drift_page+'spin K1 64\nwait 100\nshot vco_drift.png\n')
assert drift_on==drift_idle+1
assert run('drift_wait',drift_page+'spin K1 64\nwait 2100\n')==drift_on
return_page='wait 100\ntap HOME\nspin SELECT 44\nwait 100\n'
return_idle=run('return_idle',return_page)
return_on=run('return_on',return_page+'spin K1 1\nwait 100\nshot note_return.png\n')
assert return_on==return_idle+1
assert run('return_wait',return_page+'spin K1 1\nwait 2100\n')==return_on
mode_page='wait 100\ntap HOME\nspin SELECT 45\nwait 100\n'
mode_idle=run('native_mono_idle',mode_page)
mode_on=run('native_mono_on',mode_page+'spin K1 1\nwait 100\nshot native_mono.png\n')
assert mode_on==mode_idle+2 # Mode and effective voice-count indicator.
assert run('native_mono_wait',mode_page+'spin K1 1\nwait 2100\n')==mode_on
run('native_mono_ag',mode_page+'spin K1 2\nwait 100\nshot native_mono_ag.png\n')
# Init loading must also restore the new mode and the retained three-voice cap.
run('native_mono_reference',mode_page+'shot native_mono_reference.png\n')
run('native_mono_restore',mode_page+'spin K1 2\ntap SEL\ntap SEL\nwait 100\ntap HOME\nspin SELECT 45\nwait 100\nshot native_mono_restored.png\n')
assert (out/'native_mono_reference.png').read_bytes()==(out/'native_mono_restored.png').read_bytes()
dist_page='wait 100\ntap HOME\nspin SELECT 46\nwait 100\n'
dist_idle=run('distortion_idle',dist_page)
for knob,step in [(1,1),(2,75),(3,-40),(4,20)]:
 script=dist_page+f'spin K{knob} {step}\nwait 100\n'
 assert run(f'distortion_k{knob}',script)==dist_idle+1
 assert run(f'distortion_k{knob}_idle',script+'wait 2000\n')==dist_idle+1
run('distortion_reference',dist_page+'shot distortion_reference.png\n')
run('distortion_active',dist_page+'spin K1 1\nspin K2 75\nspin K3 -40\nspin K4 20\nwait 100\nshot distortion.png\n')
run('distortion_restore',dist_page+'spin K1 1\nspin K2 75\nspin K3 -40\nspin K4 20\ntap SEL\ntap SEL\nwait 100\ntap HOME\nspin SELECT 46\nwait 100\nshot distortion_restored.png\n')
assert (out/'distortion_reference.png').read_bytes()==(out/'distortion_restored.png').read_bytes()
for page,name,steps in [(47,'eq',[(1,1),(2,32),(3,-24),(4,60)]), (48,'eq_clock',[(1,75),(2,10),(3,15),(4,1)])]:
 base=f'wait 100\ntap HOME\nspin SELECT {page}\nwait 100\n'
 before=run(name+'_idle',base)
 for knob,step in steps:
  script=base+f'spin K{knob} {step}\nwait 100\n'
  assert run(f'{name}_k{knob}',script)==before+1
  assert run(f'{name}_k{knob}_wait',script+'wait 2000\n')==before+1
 run(name+'_reference',base+f'shot {name}_reference.png\n')
 active=base+''.join(f'spin K{k} {n}\n' for k,n in steps)+'wait 100\n'
 run(name+'_active',active+f'shot {name}.png\n')
 run(name+'_restore',active+f'tap SEL\ntap SEL\nwait 100\ntap HOME\nspin SELECT {page}\nwait 100\nshot {name}_restored.png\n')
 assert (out/f'{name}_reference.png').read_bytes()==(out/f'{name}_restored.png').read_bytes()
result=dict(initial_blits=initial,idle_blits=idle,one_knob_extra_blits=changed-initial,
 unchanged_performance_extra_blits=perf_idle-perf,one_counter_extra_blits=perf_changed-perf)
(r/'tests/ui_results.json').write_text(json.dumps(result,indent=2));print(result)
