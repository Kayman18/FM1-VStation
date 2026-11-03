# FM1 V-Station RC1

RC1 is the tested eight-voice shared-pool V-Station port with the approved on-device V-Station-inspired GUI and startup splash.

## Engine

- Eight physical native voices in one dynamically shared global pool.
- Runtime POOL selector 1..8, default 8.
- No fixed per-part reservation: any free physical voice can service the next note/unison group.
- Unison remains 1x..4x; two independent 4x-unison notes can consume all eight physical voices.
- Whole-group voice stealing and the existing steal crossfade are retained.
- Dual-core rendering naturally divides a full eight-voice load 4+4.
- 175 retained factory presets across 13 categories.
- 170 active synth parameters and USB-C Web MIDI editor retained.

## On-device GUI

- Native FM1 display size: 240x240 RGB565.
- V-Station-inspired blue/silver header and dark-blue parameter panels.
- Four-parameter pages retain direct K1..K4 mapping.
- K1..K4 labels are rendered on the parameter tiles.
- Preset browser uses the same device-native visual theme.
- Performance/CPU diagnostic pages retain live counters.
- New vector startup splash: NOVATION at the top, stylized V-Station panel in the center, V-STATION / RC1 at the bottom.
- The splash is drawn from FM1 primitives rather than storing a full 240x240 bitmap, preserving flash headroom.

## Target build

- Firmware identity: `FM-1_15010000`
- Display version: `RC1`
- Application: 566,400 / 581,564 bytes
- Main RAM: 94,928 / 98,304 bytes
- FX/large-buffer pool: 335,692 / 344,064 bytes
- FWSC: 610,108 bytes

The target audit passes: no software-double helpers, no app-side raw register access, valid M-Flasher directory CRCs, and exact decrypted application verification.

## Validation status

The eight-voice baseline previously passed allocator, whole-unison-group stealing, dual-core worker, sanitizer, and all-175-factory-preset render regressions. The user reported the first physical eight-voice test **seems good**.

For this GUI build, source-consistency, POOL 1..8 UI, preset-browser and USB editor protocol regressions pass, as does the FM1 target link/audit. The long all-sanitizer suite was not fully re-run after the cosmetic-only GUI change; DSP source is unchanged from the tested eight-voice baseline.
