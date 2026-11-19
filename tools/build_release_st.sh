#!/bin/sh
# The Syntakt's release file (docs/plan-syntakt.md, SD6 and S6): the minimal core and seedr-st patched into your
# stock Syntakt OS 1.42, in U1's memory area, repacked for a unit by elektron-firmware-tool (task U3).
#   sh tools/build_release_st.sh    -> build/trig-seedr-<version>-st-os1.42.syx, its facts (.json) and the GPL
#                                      version 3 text beside it (the mod includes lib/map_data.c)
# Steps, each stopping the script when it fails:
#   1. tools/st_unit.sh: builds build/emu-st/seedr-unit.syx (tools/st_build.sh seedr --unit) and runs its checks
#      (the tool reads it, every section but MAIN as stock's, MAIN as elekloader linked it, the container within
#      the official file's size, an in-place margin of at least 4096 bytes, tests-st 01 and 13 in analog-emu-st)
#   2. the OS version field, as tools/build_release.sh sets the Rytm's (TSxy for x.y, Txyz for x.y.z): the
#      checked file rebuilt byte for byte by the tool (-r) with only the version changed
#   3. the release file checked again: every section decompresses as the checked file's, elekloader's syx.verify
#      with the profile's own limits, and it boots in analog-emu-st with the trig LEDs lit as stock lights them
#   4. the facts: sizes, packing level, margin, area, the stock file's hash, the version field, the git commit
# Not checked on a unit: the version field the Syntakt shows (Synfact's profile has version_optional; the Rytm's
# TSxy has been on units since 0.5). The file contains Elektron's firmware: it stays in build/. Not for a unit
# until S6's steps (docs/flashing-st.md); never attach, upload or send it.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init build-release-st
W=$R/build/emu-st
EFT=$(sh "$T/eft.sh") || emu_die "tools/eft.sh"
V=$(python3 -c "import json; print(json.load(open('$R/mods/seedr-st/mod.json'))['version'])")
TAG=$(echo "$V" | awk -F. '{ if ($3 + 0 > 0) printf "T%d%d%d", $1, $2, $3; else printf "TS%d%d", $1, $2 }')
AREA=${ST_AREA:-47a00000-47b00000}
OUT=$R/build/trig-seedr-$V-st-os1.42.syx
rm -f "$OUT" "${OUT%.syx}.json"

# 1. the unit file and its checks
sh "$T/st_unit.sh" > "$D/st_unit.txt" 2>&1
grep -E '^(PASS|FAIL)|passed' "$D/st_unit.txt"
grep -q '^st-unit: [0-9]* passed, 0 failed$' "$D/st_unit.txt" || emu_die "tools/st_unit.sh failed: $D/st_unit.txt"
U=$W/seedr-unit.syx

# 2. the version field
"$EFT" -i "$U" -r -V "$TAG" -o "$OUT" > "$D/version.log" 2>&1 || { cat "$D/version.log"; emu_die "set the version"; }

# 3. the release file, checked
rm -rf "$D/unit" "$D/release"; mkdir -p "$D/unit" "$D/release"
"$EFT" -i "$U" -o "$D/unit" > /dev/null 2>&1 || emu_die "decompress the unit file"
"$EFT" -i "$OUT" -o "$D/release" > /dev/null 2>&1 || emu_die "decompress the release file"
emu_check "every section decompresses as the checked unit file's" same \
    "$(diff -r "$D/unit" "$D/release" > /dev/null && echo same || echo differs)"
emu_check "the tool's checksums are ok" ok "$("$EFT" -i "$OUT" | sed -n 's/^checksums : //p')"
(cd "$W/synfact" && python3 - "$STOCK" "$OUT" "$W/seedr-main.bin" "$TAG" <<'PY'
import sys
from elekloader import syx
from elekloader.devices import syntakt
stock, raw, main = syx.Syx.load(sys.argv[1]), open(sys.argv[2], 'rb').read(), open(sys.argv[3], 'rb').read()
facts = syx.verify(raw, stock, main, syntakt.DEVICE)
built = syx.Syx(raw)
assert built.version == sys.argv[4], built.version
assert facts['main']['inplace_min_gap'] >= 4096, facts['main']['inplace_min_gap']
print('verified')
PY
) > "$D/verify.txt" 2>&1
emu_check "elekloader's syx.verify with the profile's limits, version $TAG" verified "$(tail -1 "$D/verify.txt")"
FW=$OUT; export DTEMU_FW=$FW
emu_boot
emu_shot boot
emu_check "it boots in analog-emu-st (RECORD lights track 1's trigs)" "1 4" "$(emu_keys RECORD; st_lit)"
[ "$EMU_FAIL" -eq 0 ] || { rm -f "$OUT"; emu_die "the release file failed a check: removed"; }

# 4. the facts
python3 - "$W/seedr-unit.json" "$OUT" "$STOCK" "$TAG" "$V" "$AREA" "$(git -C "$R" rev-parse --short HEAD)" \
    "$(git -C "$R" status --porcelain | grep -q . && echo dirty || echo clean)" "${OUT%.syx}.json" <<'PY'
import hashlib, json, sys
u, out, stock, tag, v, area, commit, tree, js = sys.argv[1:]
f = json.load(open(u))
sha = lambda p: hashlib.sha256(open(p, 'rb').read()).hexdigest()
json.dump({
    'file': out.rsplit('/', 1)[-1], 'sha256': sha(out), 'seedr': v, 'version_field': tag,
    'stock': {'file': stock.rsplit('/', 1)[-1], 'sha256': sha(stock)},
    'container': f['container_len'], 'container_budget': f['stock_container'], 'packing_level': f['level'],
    'inplace_margin': f['main']['inplace_min_gap'], 'changed_sections': f['changed_sections'],
    'mods_area': area, 'core': 'minimal (core-st hook bus, tools/st_build.sh --core mini)',
    'git': {'commit': commit, 'tree': tree},
    'checked_on_a_unit': False,
}, open(js, 'w'), indent=1)
PY
cp "$R/LICENSES/GPL-3.0-or-later.txt" "${OUT%.syx}.LICENSE.txt"
echo "built $OUT"
echo "  OS version field $TAG, seedr $V, minimal core, area $AREA, $(${ELEKLOADER_CROSS}gcc -dumpfullversion)"
echo "  sha256 $( (sha256sum "$OUT" 2>/dev/null || shasum -a 256 "$OUT") | cut -c1-64)"
echo "  facts $(basename "${OUT%.syx}").json; licence GPL-3.0-or-later: $(basename "${OUT%.syx}").LICENSE.txt"
echo "  NOT checked on a unit: stays in build/ until docs/flashing-st.md's steps"
emu_done
