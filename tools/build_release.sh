#!/bin/sh
# The firmware to flash: core-ar2 + seedr patched into your stock OS 1.74, verified by elekloader.
#   sh tools/build_release.sh            -> build/trig-seedr-<version>-os1.74.syx (OS version field TSxy)
# Needs ../elekloader, the stock OS (STOCK) and the m68k-elf toolchain, as tools/test_seedr.sh. Run the tests first.
# The output contains Elektron's firmware: flash it to your own unit, don't share it.
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
E=${WORKSPACE:-$(cd "$R/.." && pwd)}
STOCK=${STOCK:-"$E/Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx"}
V=$(python3 -c "import json; print(json.load(open('$R/mods/seedr-ar2/mod.json'))['version'])")
TAG=$(echo "$V" | awk -F. '{printf "TS%d%d", $1, $2}')
OUT=$R/build/trig-seedr-$V-os1.74.syx
mkdir -p $R/build
cd $E/elekloader
python3 -m elekloader.sdk.build mods/core-ar2 --stock "$STOCK" | head -1
python3 -m elekloader.sdk.build $R/mods/seedr-ar2 --stock "$STOCK" | head -1
python3 -m elekloader.patch --stock "$STOCK" --mod mods/core-ar2/out/core-0.13.elemod \
    --mod $R/mods/seedr-ar2/out/seedr-$V.elemod --out "$OUT" --lockfile "${OUT%.syx}.lock.json" --version $TAG | tail -1
if [ -f $R/mods/seedr-ar2/grooves-live.h ]; then g="your Live grooves (grooves-live.h)"; else g="the swing grooves (grooves-swing.h)"; fi
echo "built $OUT"
echo "  OS version field $TAG, seedr $V, core-ar2 0.13, with $g"
echo "  sha256 $(sha256sum "$OUT" | cut -c1-64)"
