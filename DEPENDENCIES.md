# External build dependencies

The repository contains the V-Station RC1 source, FM1 SDK subset, build scripts, web editor and prebuilt release firmware.

Two large local build inputs from the validated all-in-one source kit are intentionally not duplicated in the browsable source tree:

- `toolchain/jieli-linux-toolchains-20260730.1.tar.xz`
- `reference/FM1-PolyWavetable-TEST81-M-Flasher.bin` (only needed to regenerate a complete M-Flasher image; not required for the FWSC application package)

The build accepts an unpacked toolchain via `JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1`.

Validated SHA-256 values:

`a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`  jieli-linux-toolchains-20260730.1.tar.xz

`ead915f71caff82d92e090da0b6912e910774fff950153fd4976f935937e6940`  FM1-PolyWavetable-TEST81-M-Flasher.bin