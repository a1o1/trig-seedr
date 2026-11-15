#!/bin/sh
# The host side (no emulator): the host tests and ColdFire build checks of lib/, device/ and the
# mod's page tables (gate rule 1); seedref's recipe layout is the cross compiler's (so emu_recipe
# writes where the mod reads), --fields names real fields, SOURCE = TRACK identity on the golden
# original, and the golden vectors (tools/golden.sh).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"
EMU_NAME=00-host; mkdir -p "$W"
# 0. make: host tests (ASan / UBSan) and the ColdFire checks (no undefined symbols)
for t in "lib test" "lib coldfire" "device test" "device coldfire" "mods/seedr-ar2 test"; do
    # shellcheck disable=SC2086
    set -- $t
    if out=$(make -s -C "$R/$1" "$2" 2>&1); then rc=0; else rc=$?; fi
    echo "$out" | grep -E 'checks|undefined|FAIL|rror' | sed 's/^/  /'
    emu_check "make -C $1 $2" 0 "$rc"
    make -s -C "$R/$1" clean > /dev/null 2>&1
done
# 1. every recipe field at the same offset and size on the host and on ColdFire
host=$(seedref --layout)
paths=$(echo "$host" | awk '$1 != "sizeof" { print $1 }')
# shellcheck disable=SC2086
cf=$(emu_py layout "${ELEKLOADER_CROSS}gcc" "$R/lib/seed.h" seed_recipe $paths)
emu_check "seed_recipe layout: host = ColdFire ($(echo "$paths" | wc -l | tr -d ' ') fields)" "$host" "$cf"
# 2. --fields: the C paths and values a recipe sets
emu_check "seedref --fields" "seed 7
src 1
vel.lo 60
vel.hi 120
lane[0].pct 100
lane[0].val.lo 1
lane[0].val.hi 8" "$(seedref --fields seed=7 src=random vel=60:120 lane1.pct=100 lane1=1:8)"
# 3. identity on the golden original, and its dump read back unchanged
emu_check "SOURCE = TRACK identity on orig-a" "$(cat "$R/lib/golden/orig-a.txt")" \
    "$(cd "$R/lib/golden" && seedref orig=orig-a.txt seed=9 vary=40)"
# 4. the golden vectors
out=$(sh "$T/golden.sh" 2>&1); rc=$?
echo "$out" | grep -v '^golden:' | sed 's/^/  /'
emu_check "golden vectors: $(echo "$out" | tail -1 | sed 's/^golden: //')" 0 $rc
emu_done
