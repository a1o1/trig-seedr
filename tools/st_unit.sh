#!/bin/sh
# Task U3 (docs/plan-syntakt.md): the unit-shaped Syntakt file, checked. tools/st_build.sh seedr --unit builds
# build/emu-st/seedr-unit.syx (the stock 1.42 file rebuilt by elektron-firmware-tool with seedr's patched MAIN);
# this script checks it with the tool's own reading, apart from elekloader's syx.verify that the build ran:
#   1. the tool reads it: device Syntakt, version 1.42, checksums ok; the Syntakt has no HMAC trailer (the tool
#      reports "trailer : none" for stock too)
#   2. every section but MAIN (3) decompresses equal to stock's
#   3. MAIN decompresses equal to the patched image elekloader linked
#   4. the container is within the official file's size, the profile's flash_limit (2,029,392 bytes)
#   5. the in-place margin (the bootloader's depack over its staged stream) is at least 4096 bytes
#   6. it boots in analog-emu-st and passes tools/tests-st 01 and 13 (ST_UNIT=1)
# The file never leaves build/: NOT for a unit yet (the memory area, status/U3.md). Writes only under build/.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}     # a patched Syntakt file boots only there (status/S0.3.md)
. "$R/tools/emu_st.sh"
emu_init st-unit
W=$R/build/emu-st
EFT=$(sh "$T/eft.sh") || emu_die "tools/eft.sh"
sh "$T/st_build.sh" seedr --unit "$R/mods/seedr-st" > "$D/build.log" 2>&1 || { tail -5 "$D/build.log"; emu_die "st_build.sh --unit"; }
U=$W/seedr-unit.syx
[ -f "$U" ] || emu_die "no $U"
grep '^verified seedr-unit' "$D/build.log"

# 1. the tool's summary
"$EFT" -i "$U" > "$D/unit.txt" 2>&1
emu_check "the tool reads it: Syntakt 1.42, checksums ok" "Syntakt (0x16) 1.42 ok" \
    "$(awk '/^device/ {d = $3 " " $4} /^version/ {v = $3} /^checksums/ {c = $3} END {print d, v, c}' "$D/unit.txt")"
emu_check "no HMAC trailer, as stock" "none" "$(sed -n 's/^  trailer *: //p' "$W/seedr-unit.log" | tail -1)"

# 2, 3. every section decompressed by the tool
rm -rf "$D/stock" "$D/unit"; mkdir -p "$D/stock" "$D/unit"
"$EFT" -i "$STOCK" -o "$D/stock" > /dev/null 2>&1 || emu_die "decompress stock"
"$EFT" -i "$U" -o "$D/unit" > /dev/null 2>&1 || emu_die "decompress the unit file"
same=
for f in "$D"/stock/*; do
    b=$(basename "$f")
    case $b in section_3_*) main=$b; continue ;; esac
    cmp -s "$f" "$D/unit/$b" && same="$same $b"
done
emu_check "every section but MAIN decompresses as stock's" \
    "$(cd "$D/stock" && command ls -1 | grep -v '^section_3_' | tr '\n' ' ')" "$(echo $same | tr ' ' '\n' | grep . | tr '\n' ' ')"
emu_check "MAIN decompresses to elekloader's patched image" same \
    "$(cmp -s "$D/unit/$main" "$W/seedr-main.bin" && echo same || echo "differs ($main)")"

# 4, 5. the container and the margin
python3 - "$W/seedr-unit.json" > "$D/facts.txt" <<'PY'
import json, sys
f = json.load(open(sys.argv[1]))
print(f['container_len'], f['stock_container'], f['main']['inplace_min_gap'], f['level'])
PY
read -r CLEN SLEN GAP LEVEL < "$D/facts.txt"
emu_check "container $CLEN within the official $SLEN bytes" yes "$([ "$CLEN" -le "$SLEN" ] && echo yes)"
emu_check "in-place margin $GAP of at least 4096 (packing level $LEVEL)" yes "$([ "$GAP" -ge 4096 ] && echo yes)"

# 6. the unit file in analog-emu-st: boots, and the generate and apply files pass on it
FW=$U; export DTEMU_FW=$FW
emu_boot
emu_shot boot
emu_check "it boots to the main screen (RECORD lights track 1's trigs)" "1 4" "$(emu_keys RECORD; st_lit)"
for t in 01 13; do
    ST_UNIT=1 sh "$T/st_run_tests.sh" $t > "$D/tests-$t.txt" 2>&1
    emu_check "tests-st $t on the unit file: $(grep -E '^st[0-9]+-.*passed' "$D/tests-$t.txt" | sed 's/^st//')" \
        "failed: none" "$(grep '^failed:' "$D/tests-$t.txt")"
done
emu_done
