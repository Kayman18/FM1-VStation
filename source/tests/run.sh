#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -Iengine tests/synth_test.c engine/*.c -lm -o tests/synth_test
# No dynamic allocation in this engine; leak detector is unavailable under some hosts.
ASAN_OPTIONS=detect_leaks=0 tests/synth_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/poly_test.c engine/*.c -lm -o tests/poly_test
ASAN_OPTIONS=detect_leaks=0 tests/poly_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/tempo_test.c engine/*.c -lm -o tests/tempo_test
ASAN_OPTIONS=detect_leaks=0 tests/tempo_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/clock_test.c engine/*.c -lm -o tests/clock_test
ASAN_OPTIONS=detect_leaks=0 tests/clock_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/pressure_test.c engine/*.c -lm -o tests/pressure_test
ASAN_OPTIONS=detect_leaks=0 tests/pressure_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/midi_level_test.c engine/*.c -lm -o tests/midi_level_test
ASAN_OPTIONS=detect_leaks=0 tests/midi_level_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/pitch_env_curve_test.c engine/*.c -lm -o tests/pitch_env_curve_test
ASAN_OPTIONS=detect_leaks=0 tests/pitch_env_curve_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/pwm_depth_test.c engine/*.c -lm -o tests/pwm_depth_test
ASAN_OPTIONS=detect_leaks=0 tests/pwm_depth_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/fm_scale_test.c engine/*.c -lm -o tests/fm_scale_test
ASAN_OPTIONS=detect_leaks=0 tests/fm_scale_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/mixer_scale_test.c engine/*.c -lm -o tests/mixer_scale_test
ASAN_OPTIONS=detect_leaks=0 tests/mixer_scale_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/controller_amp_test.c engine/*.c -lm -o tests/controller_amp_test
ASAN_OPTIONS=detect_leaks=0 tests/controller_amp_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/lfo_trigger_test.c engine/*.c -lm -o tests/lfo_trigger_test
ASAN_OPTIONS=detect_leaks=0 tests/lfo_trigger_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/shared_lfo_test.c engine/*.c -lm -o tests/shared_lfo_test
ASAN_OPTIONS=detect_leaks=0 tests/shared_lfo_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/preglide_test.c engine/*.c -lm -o tests/preglide_test
ASAN_OPTIONS=detect_leaks=0 tests/preglide_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/osc_bend_test.c engine/*.c -lm -o tests/osc_bend_test
ASAN_OPTIONS=detect_leaks=0 tests/osc_bend_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -Iengine tests/amp_level_test.c engine/amp_level.c -lm -o tests/amp_level_test
ASAN_OPTIONS=detect_leaks=0 tests/amp_level_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -Iengine tests/amp_level_integration_test.c engine/*.c -lm -o tests/amp_level_integration_test
ASAN_OPTIONS=detect_leaks=0 tests/amp_level_integration_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/filter_character_test.c engine/*.c -lm -o tests/filter_character_test
ASAN_OPTIONS=detect_leaks=0 tests/filter_character_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/osc_drift_test.c engine/*.c -lm -o tests/osc_drift_test
ASAN_OPTIONS=detect_leaks=0 tests/osc_drift_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/note_return_test.c engine/*.c -lm -o tests/note_return_test
ASAN_OPTIONS=detect_leaks=0 tests/note_return_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/three_voice_test.c engine/*.c -lm -o tests/three_voice_test
ASAN_OPTIONS=detect_leaks=0 tests/three_voice_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/native_mono_test.c engine/*.c -lm -o tests/native_mono_test
ASAN_OPTIONS=detect_leaks=0 tests/native_mono_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/distortion_test.c engine/*.c -lm -o tests/distortion_test
ASAN_OPTIONS=detect_leaks=0 tests/distortion_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/eq_test.c engine/*.c -lm -o tests/eq_test
ASAN_OPTIONS=detect_leaks=0 tests/eq_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/pan_test.c engine/*.c -lm -o tests/pan_test
ASAN_OPTIONS=detect_leaks=0 tests/pan_test
gcc -O2 -g -fsanitize=address,undefined,float-cast-overflow -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/chorus_test.c engine/*.c -lm -o tests/chorus_test
ASAN_OPTIONS=detect_leaks=0 tests/chorus_test
gcc -O2 -g -fsanitize=address,undefined,float-cast-overflow -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/delay_test.c engine/*.c -lm -o tests/delay_test
ASAN_OPTIONS=detect_leaks=0 tests/delay_test
gcc -O2 -g -fsanitize=address,undefined,float-cast-overflow -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/reverb_test.c engine/*.c -lm -o tests/reverb_test
ASAN_OPTIONS=detect_leaks=0 tests/reverb_test
gcc -O2 -g -fsanitize=address,undefined,float-cast-overflow -ffp-contract=off -Iengine tests/pcm_output_test.c -lm -o tests/pcm_output_test
ASAN_OPTIONS=detect_leaks=0 tests/pcm_output_test
bash build.sh --host
mkdir -p tests/host-output
app/build/host/vstation_host tests/hardware_smoke.txt tests/host-output
for script in midi_bend_smoke midi_sustain_smoke midi_filter_controllers_smoke midi_pitch_controllers_smoke midi_filter_lfo_smoke midi_vibrato_smoke; do
 mkdir -p "tests/host-output/$script"
 app/build/host/vstation_host "tests/$script.txt" "tests/host-output/$script"
done
python3 tests/ui_updates_test.py
python3 tests/pan_ui_test.py
python3 tests/chorus_ui_test.py
python3 tests/delay_ui_test.py
python3 tests/reverb_ui_test.py
python3 tests/midi_channel_test.py
python3 tests/cache_equivalence_test.py

gcc -O2 -g -fsanitize=address,undefined -Iengine tests/preset_test.c engine/preset.c -o tests/preset_test
ASAN_OPTIONS=detect_leaks=0 tests/preset_test

python3 tests/preset_ui_test.py

python3 tests/native_mono_model_test.py

# TEST19 actual thread ownership, failure-latch and bounded render batching.
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -Iengine tests/worker_test.c tests/worker_backend.c engine/*.c -lm -pthread -o tests/worker_test
ASAN_OPTIONS=detect_leaks=0 tests/worker_test
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/reverb_worker_test.c tests/worker_backend.c engine/*.c -lm -pthread -o tests/reverb_worker_test
ASAN_OPTIONS=detect_leaks=0 tests/reverb_worker_test
python3 tests/worker_ui_test.py

# TEST20 event-only effect phrase starts; all existing DSP/sample loops retained.
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/fx_phase_test.c engine/*.c -lm -o tests/fx_phase_test
ASAN_OPTIONS=detect_leaks=0 tests/fx_phase_test
python3 tests/fx_phase_ui_test.py

# TEST42-8VOICE native engine: shared eight-voice pool, 1..4 unison and ARP-free embedded factory bank.
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/native_poly_unison_test.c engine/*.c -lm -o tests/native_poly_unison_test
ASAN_OPTIONS=detect_leaks=0 tests/native_poly_unison_test
gcc -O2 -g -fsanitize=address,undefined -Iengine -Iapp tests/factory_preset_test.c engine/preset.c -o tests/factory_preset_test
ASAN_OPTIONS=detect_leaks=0 tests/factory_preset_test
python3 tests/factory_preset_ui_test.py
python3 tests/native_pool_ui_test.py
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/native_worker_test.c tests/worker_backend.c engine/*.c -lm -pthread -o tests/native_worker_test
ASAN_OPTIONS=detect_leaks=0 tests/native_worker_test
gcc -O2 -g -fsanitize=address,undefined,float-cast-overflow -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine -Iapp tests/factory_render_smoke_test.c engine/*.c -lm -o tests/factory_render_smoke_test
ASAN_OPTIONS=detect_leaks=0 tests/factory_render_smoke_test

gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -Iengine tests/test24_fx_block_test.c engine/native_eq.c engine/native_eq_tables.c engine/lfo.c engine/native_delay.c engine/native_chorus.c engine/native_reverb.c -lm -o tests/test24_fx_block_test
ASAN_OPTIONS=detect_leaks=0 tests/test24_fx_block_test

# TEST26 diagnostic profiling: per-half stage maxima must accumulate and reset.
gcc -O2 -g -fsanitize=address,undefined -ffp-contract=off -DVSTATION_TARGET -DVSTATION_TEST -Iengine tests/profile_test.c engine/*.c -lm -o tests/profile_test
ASAN_OPTIONS=detect_leaks=0 tests/profile_test
python3 tests/source_consistency.py

# TEST41 USB-MIDI SysEx editor: 170 ARP-free parameters, preset manager, hardware controls and telemetry.
python3 tests/editor_protocol_test.py
