# FM1 V-Station RC1

RC1 promotes the tested eight-voice shared-pool build to the first release candidate.

## RC1 engine

- Eight physical native voices in one dynamically shared global pool.
- Runtime POOL selector 1..8, default 8.
- No fixed per-part reservation: any free physical voice can service the next note/unison group.
- Existing unison remains 1x..4x; two independent 4x-unison notes can consume all eight physical voices.
- Whole-group voice stealing and the existing 64-sample steal crossfade are retained.
- Dual-core rendering uses the existing worker architecture and naturally divides a full pool as 4+4.
- 175 retained factory presets, 13 categories, with the prior Arpeggio/Motion cleanup retained.
- 170 active synth parameters; USB-C web editor, JSON patch files, persistent browser user library and live telemetry retained.

## Resource use

The eight-voice TEST42 target immediately preceding RC1 used:

- Application: 561,552 / 581,564 bytes.
- Main RAM: 94,928 / 98,304 bytes.
- FX/large-buffer pool: 335,692 / 344,064 bytes.
- FWSC: 610,108 bytes.

RC1 changes only release/version identity and editor labeling from that tested engine; DSP/polyphony behavior is unchanged.

## Validation

- Eight independent one-voice note groups allocate simultaneously; the ninth note invokes global oldest-group stealing.
- Two 4x-unison groups occupy all eight voices; the next allocation steals a whole group.
- Runtime POOL 1..8 regression passes.
- Serial vs real-thread dual-core output is bit-exact for POOL 8 and 4x unison.
- Native mono return, pressure, sustain, duplicate FIFO and glide regressions pass.
- 175/175 retained factory presets rendered under ASan/UBSan/float-cast-overflow with finite stereo output and zero synth faults.
- Clean packaged-source rebuild of the eight-voice baseline was byte-identical.

## Hardware status

The user completed an initial physical FM1 test of the eight-voice shared pool and reported that it **seems good**. No failure was reported in that first test. RC1 keeps the same audio/polyphony implementation.