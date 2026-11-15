#!/bin/sh
# The firmware to flash: core-ar2 + seedr patched into your stock OS 1.74, verified by elekloader.
#   sh tools/build_release.sh            -> build/trig-seedr-<version>-os1.74.syx (OS version field TSxy),
#                                           and the GPL version 3 text beside it: the mod includes
#                                           lib/map_data.c, so the build is GPL-3.0-or-later (README, License)
# It builds through tools/emu.sh, the same build the emulator checks use (core-ar2 into this
# checkout's build/emu/core, with -std=gnu17 on gcc 15 or later), and stops when a build fails.
# Needs WORKSPACE, the stock OS (STOCK) and ELEKLOADER_CROSS. Run
# tools/run_tests.sh first. Releases are built on the Mac (docs/dev-environment.md).
# The output contains Elektron's firmware: flash it to your own unit, don't share it.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu.sh"
EMU_NAME=build_release
V=$(python3 -c "import json; print(json.load(open('$R/mods/seedr-ar2/mod.json'))['version'])")
TAG=$(echo "$V" | awk -F. '{printf "TS%d%d", $1, $2}')
OUT=$R/build/trig-seedr-$V-os1.74.syx
EMU_VERSION=$TAG emu_build "release-$TAG"
cp "$FW" "$OUT"
cp "$W/release-$TAG.lock.json" "${OUT%.syx}.lock.json"
cp "$R/LICENSES/GPL-3.0-or-later.txt" "${OUT%.syx}.LICENSE.txt"
if [ -f "$R/mods/seedr-ar2/grooves-live.h" ]; then g="your Live grooves (grooves-live.h)"; else g="the swing grooves (grooves-swing.h)"; fi
sha=$( (sha256sum "$OUT" 2>/dev/null || shasum -a 256 "$OUT") | cut -c1-64)
echo "built $OUT"
echo "  OS version field $TAG, seedr $V, core-ar2 0.13, $(${ELEKLOADER_CROSS}gcc -dumpfullversion), with $g"
echo "  sha256 $sha"
echo "  licence GPL-3.0-or-later: $(basename "${OUT%.syx}").LICENSE.txt"
