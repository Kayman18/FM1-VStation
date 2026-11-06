#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
export AC79_SDK="${AC79_SDK:-$PWD/build-deps/sdk}"
export X0X_FONTSET=barlow
t="$PWD/build-toolchain/jieli-linux-toolchains-20260730.1"
if [[ -n "${JIELI_TOOLCHAIN:-}" && -x "${JIELI_TOOLCHAIN}/pi32v2/bin/clang" ]]; then
 :
elif [[ -x "$t/pi32v2/bin/clang" ]]; then
 export JIELI_TOOLCHAIN="$t"
elif [[ -f toolchain/jieli-linux-toolchains-20260730.1.tar.xz ]]; then
 mkdir -p build-toolchain
 tar --no-same-owner -xJf toolchain/jieli-linux-toolchains-20260730.1.tar.xz -C build-toolchain
 export JIELI_TOOLCHAIN="$t"
else
 echo "Missing JieLi toolchain. Set JIELI_TOOLCHAIN to jieli-linux-toolchains-20260730.1 or place its .tar.xz under toolchain/." >&2
 exit 2
fi
python3 base/tools/build.py app --release 1.0.0 --display-version "RC" "$@"
if [[ " $* " == *" --host "* || " $* " == *" --web "* ]]; then exit 0; fi
mkdir -p release
cp app/build/vstation-1.0.0.fwsc release/FM1-VStation-RC.fwsc
if [[ -f reference/FM1-PolyWavetable-TEST81-M-Flasher.bin ]]; then
 python3 tools/make_mflasher.py reference/FM1-PolyWavetable-TEST81-M-Flasher.bin release/FM1-VStation-RC-M-Flasher.bin --app app/build/vstation.bin
else
 echo "Reference full image not present; built FWSC only. The prebuilt RC M-Flasher image is under release/." >&2
fi
