# FM1 V-Station RC

FM1 V-Station RC is an independent FM1 port/reconstruction inspired by the Novation V-Station architecture. This release candidate combines the approved 240x240 device GUI with an **8-voice shared dynamic pool** and **up to 8 unison voices**.

## Highlights

- **8 physical voices** in one shared dynamic pool — POOL 1..8, default 8
- **UNISON VOICES 1..8**
- global whole-group voice stealing
- dual-core 4+4 rendering at a full eight-voice load
- transient steal/rebuild CPU optimizations for heavy 7x-8x unison cases
- 175 retained factory presets / 13 categories
- 170 active synth parameters
- native **240x240 RGB565 V-Station-inspired GUI**
- V-Station-themed vector loading splash
- USB-C Web MIDI editor with JSON preset import/export and persistent browser user library
- live FM1 CPU/deadline/underrun telemetry
- visible source, build scripts and prebuilt firmware in this repository

## Unison / voice page

`NATIVE VOICE MODE` is intentionally near the front of the ENV navigation now. From HOME, press **ENV twice**.

- **K1:** NOTE MODE
- **K2:** UNISON VOICES — 1..8
- **K3:** UNISON DETUNE
- **K4:** VOICE LIMIT / POOL — 1..8

At 1x unison the pool can provide eight notes; at 2x, four notes; at 4x, two notes; at 5x-8x, one note can occupy most or all of the pool.

## Firmware

Preferred application update:

`release/FM1-VStation-RC.fwsc`

Complete image:

`release/FM1-VStation-RC-M-Flasher.bin`

Keep your own full-device backup before using a complete M-Flasher image.

## Source

- `source/` — browsable directly on GitHub
- `release/FM1-VStation-RC-Source.zip` — matching downloadable source package

The source tree includes the app, engine, fm1-base platform snapshot, tests, web editor generator and build scripts. Large external build inputs are documented in `DEPENDENCIES.md`.

## Building

Set `JIELI_TOOLCHAIN` to the validated JieLi toolchain, or place `jieli-linux-toolchains-20260730.1.tar.xz` under `toolchain/`.

```bash
bash build.sh
```

Host simulator:

```bash
bash build.sh --host
```

The complete M-Flasher image additionally requires the pinned reference full image documented in `DEPENDENCIES.md`; the normal `.fwsc` does not.

## Web editor

Use `web-editor/index.html` from `source/` or `release/FM1-VStation-RC-Web-Editor.zip`. Connect the FM1 over USB-C with a Web MIDI/SysEx-capable Chromium browser.

The Hardware tab exposes POOL 1..8, DUAL/SINGLE mode, scan diagnostics and live performance telemetry.

## Validation / resources

Current RC target footprint:

- Application: **573,276 / 581,564 bytes**
- Main RAM: **95,056 / 98,304 bytes**
- FX/large-buffer pool: **335,692 / 344,064 bytes**
- Firmware identity: **`FM-1_15010000`**

See `RC-RELEASE-NOTES.md` and `release/SHA256SUMS.txt` for validation details and hashes.

## Licensing / provenance

The FM1 platform layer is based on `fm1-base` / Felucca BSP and retains applicable open-source licensing. No Novation proprietary source code is included. This is an independent reconstruction/port from supplied executable/data behavior and public operating information.
