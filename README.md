# FM1 V-Station RC1

An FM1 firmware port/reconstruction inspired by the Novation V-Station architecture, packaged as **RC1** after a successful initial physical-device test of the new eight-voice engine.

## Highlights

- **8 physical voices in one shared dynamic pool** (POOL 1..8, default 8)
- global allocation and whole-unison-group voice stealing
- unison 1x..4x
- dual-core 4+4 rendering at a full eight-voice load
- 175 retained factory presets across 13 categories
- USB-C Web MIDI editor with full-state SysEx
- JSON preset import/export and persistent browser user-preset library
- live FM1 CPU/deadline/underrun telemetry
- source, build scripts and prebuilt RC1 firmware included

The shared pool is global: voices are not statically reserved per part or note group. At unison 1x the pool can provide eight simultaneous physical voices; at 2x, four notes; at 4x, two notes.

## Firmware

The preferred application update is:

`release/FM1-VStation-RC1.fwsc`

A complete M-Flasher image is also provided for recovery/full-image use:

`release/FM1-VStation-RC1-M-Flasher.bin`

Keep your own full-device backup before using a complete M-Flasher image because non-application regions come from the pinned project reference image.

## Building

Requirements: Linux, Python 3, the JieLi AC79 toolchain and the included SDK/build dependencies.

The repository contains the source and SDK/build subset. Supply the validated JieLi toolchain via `JIELI_TOOLCHAIN` (or place the archive under `toolchain/`). The reference full image is only required to regenerate the M-Flasher image; it is not required to build the `.fwsc`. See `DEPENDENCIES.md`.

Build target firmware:

```bash
bash build.sh
```

Build host simulator/tests where supported:

```bash
bash build.sh --host
bash tests/run.sh
```

## Web editor

Open `web-editor/index.html` through the included local launcher (`start-editor.sh` on Linux or `start-editor.bat` on Windows), connect the FM1 by USB-C, then use a Web MIDI/SysEx-capable Chromium browser.

The hardware page exposes POOL 1..8, DUAL/SINGLE CPU mode, panel scan diagnostics and live performance counters.

## RC1 validation

The eight-voice baseline passed allocator, unison-group, dual-core worker, sanitizer and all-175-factory-preset regression tests. The user also reported the first physical FM1 test of the eight-voice shared pool **seems good**.

See `RC1-RELEASE-NOTES.md` for details.

## Licensing / provenance

The FM1 platform layer is based on `fm1-base` / Felucca BSP and retains their applicable open-source licensing. No Novation proprietary source code is included. This project is an independent reconstruction/port from supplied executable/data behavior and public operating information.