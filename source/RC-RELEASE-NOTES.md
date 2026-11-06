# FM1 V-Station RC

RC promotes the expanded eight-voice/eight-unison build, the approved device-native V-Station GUI, and the transient CPU fixes into the current release candidate.

## Voice architecture

- **8 physical native voices** in one dynamically shared global pool.
- Runtime **POOL 1..8**, default 8.
- No fixed per-part reservation: any free physical voice can serve the next note/unison group.
- **UNISON VOICES 1..8**.
- Whole-group voice stealing is retained.
- At 1x unison: up to 8 simultaneous notes. At 2x: 4 notes. At 4x: 2 notes. At 5x..8x: one note can consume the pool.
- Dual-core rendering divides a full eight-voice load 4+4.

## UI fixes

- `NATIVE VOICE MODE` is now the **second ENV page**, instead of being buried deep in the ENV cycle.
- On that page: K1 = NOTE MODE, K2 = UNISON VOICES (1..8), K3 = UNISON DETUNE, K4 = VOICE LIMIT / POOL (1..8).
- 240x240 RGB565 V-Station-inspired blue/silver UI and vector startup splash retained.

## CPU / steal-path fixes

The 7x-8x unison stress case exposed transient deadline peaks during whole-group stealing even when steady-state average CPU was reasonable. RC carries the uploaded source fixes that reduce that transient work while preserving the tested sound path:

- voice rebuild no longer replays all 170 parameter setters for every stolen voice;
- shared derived-state work is re-derived only where reset state requires it;
- oscillator retune replay is collapsed to one final tune;
- zero-drive filter state stays in locals where the Q guard cannot fire;
- fade bookkeeping is skipped when no voice is fading;
- PWM shared envelope/LFO values are computed once per update;
- inaudible oscillator control-rate retunes can be skipped in POLY mode and are restored immediately if the oscillator becomes audible.

The source package includes `tests/panic_equiv_test.c` and the steal-heavy/randomized equivalence coverage used for this work.

## Factory bank / editor

- 175 retained factory presets across 13 categories.
- 170 active synth parameters.
- USB-C Web MIDI editor, JSON preset files, persistent browser user library and live telemetry retained.
- Web/editor parameter ranges now accept unison values through 8.

## Build status

The firmware published with this RC is built from the visible `source/` tree in this repository.

Target build:

- Application: **573,276 / 581,564 bytes**
- Main RAM: **95,056 / 98,304 bytes**
- FX/large-buffer pool: **335,692 / 344,064 bytes**
- FWSC: **610,108 bytes**
- Firmware identity: **`FM-1_15010000`**
- Application SHA-256: `d08aa224eeed2855dce8860dc2bfb8cceae06d2ab58bb4f6172036e5d91ace92`
- FWSC SHA-256: `b72cd34a82b834f5ddbd1ca69ac851d486417f4ee7a080289838efb7615cc229`
- M-Flasher SHA-256: `820b25f3607c338d36cdd0525029dced827fb2619b9445000fccbf4b4b7fe4a9`

The target audit reports no software-double helpers and no app-side raw register access; full-image directory CRCs and decrypted application verification pass.
