# FM1 V-Station RC1

An FM1 port/reconstruction inspired by the Novation V-Station architecture. RC1 combines the tested **8-voice shared dynamic pool** with the approved device-native V-Station-style 240x240 GUI and startup splash.

## RC1 highlights

- **8 physical voices in one shared dynamic pool** — POOL 1..8, default 8
- global allocation and whole-unison-group voice stealing
- unison 1x..4x
- dual-core 4+4 rendering at a full eight-voice load
- 175 retained factory presets / 13 categories
- 170 active synth parameters
- device-native **240x240 RGB565 V-Station-inspired GUI**
- blue/silver header, dark parameter panels and explicit K1..K4 mappings
- V-Station-themed vector loading splash with NOVATION / V-STATION branding
- USB-C Web MIDI editor with JSON preset import/export and persistent browser user library
- live FM1 CPU/deadline/underrun telemetry
- browsable source tree, build scripts and prebuilt firmware included

At unison 1x, POOL 8 provides eight simultaneous physical voices; at 2x it provides four notes; at 4x it provides two notes. The voices are not statically reserved per part.

## Firmware

Preferred update:

`release/FM1-VStation-RC1.fwsc`

Complete image:

`release/FM1-VStation-RC1-M-Flasher.bin`

Keep your own full-device backup before using a complete M-Flasher image because non-application regions come from the pinned project reference image.

## Source

The source is available two ways:

- `source/` — browsable directly on GitHub
- `release/FM1-VStation-RC1-Source.zip` — the matching downloadable source package

The source tree includes the application, engine, fm1-base platform snapshot, tests, web editor generator, build scripts and generated font/word headers. Large external build inputs are intentionally excluded; see `DEPENDENCIES.md`.

## Building

Set `JIELI_TOOLCHAIN` to the validated JieLi toolchain, or place `jieli-linux-toolchains-20260730.1.tar.xz` under `toolchain/`.

```bash
bash build.sh
```

Host simulator:

```bash
bash build.sh --host
```

The full M-Flasher image additionally requires the pinned reference image listed in `DEPENDENCIES.md`. Without it, the normal `.fwsc` still builds.

## Web editor

Use `web-editor/index.html` from `source/` or the packaged `release/FM1-VStation-RC1-Web-Editor.zip`. Connect the FM1 over USB-C in a Web MIDI/SysEx-capable Chromium browser.

The hardware page exposes POOL 1..8, DUAL/SINGLE CPU mode, scan diagnostics and live performance telemetry.

## RC1 target footprint

- Application: **566,400 / 581,564 bytes**
- Main RAM: **94,928 / 98,304 bytes**
- FX/large-buffer pool: **335,692 / 344,064 bytes**
- Firmware identity: **`FM-1_15010000`**

See `RC1-RELEASE-NOTES.md` for validation details.

## Licensing / provenance

The FM1 platform layer is based on `fm1-base` / Felucca BSP and retains its applicable open-source licensing. No Novation proprietary source code is included. This is an independent reconstruction/port from supplied executable/data behavior and public operating information.
