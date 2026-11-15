#!/bin/sh
# Task F3: the MAP source.
#   1. on the host, against the original code (tools/tests/23-map.cc built with the original
#      resources.cc and ReadDrumMap from the workspace's mi/eurorack/grids): every level, the source's
#      trigs with DENS and CHAOS, and the hashes lib/test_src_map.c holds
#   2. in the emulator, on A01 BD (a new project): SRC = MAP with the RHYTHM page's knobs (PART,
#      DENS, and X, Y, CHAOS, which are the pattern's: the mod's `shared`, not BD's recipe), YES
#      against tools/seedref; CHAOS, and YES with no knob turned (the next seed); SHAPE = MAP on
#      DYNAMICS; NO; FUNC + YES keeps; the title bar's source: MAP below CHAOS 4, the seed from 4
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 23-map

# ---- 1. the original code ----
G=$E/mi/eurorack/grids                          # E: the workspace (tools/emu.sh)
[ -f "$G/resources.cc" ] && [ -f "$G/pattern_generator.cc" ] || emu_die "no original map code in $G"
H=$D/mapcmp; mkdir -p "$H/inc/grids"
: > "$H/inc/grids/resources.h"                  # resources.cc's own include; 23-map.cc has the types
# drum_map[5][5] to the end of ReadDrumMap, unchanged
awk '/^static const prog_uint8_t\* drum_map/ { on = 1 } on { print } on && /ReadDrumMap\(/ { fn = 1 } on && fn && /^}/ { exit }' \
    "$G/pattern_generator.cc" > "$H/reader.inc"
for f in "$R"/lib/*.c; do
    case $f in */test_*) continue ;; esac
    b=$(basename "$f" .c)
    cc -std=c99 -O2 -Wall -c "$f" -o "$H/$b.o" || emu_die "cc $b"
done
c++ -std=c++11 -O2 -Wall -Wno-unused-const-variable -I "$H/inc" -I "$R/lib" -DMAP_RESOURCES="\"$G/resources.cc\"" \
    -DMAP_READER="\"$H/reader.inc\"" "$T/tests/23-map.cc" "$H"/*.o -o "$H/mapcmp" 2> "$H/cc.log" ||
    { cat "$H/cc.log" >&2; emu_die "c++ mapcmp"; }
"$H/mapcmp" > "$H/out.txt"
emu_check "every level (X, Y 0..255, 3 parts, 64 steps) = ReadDrumMap's" "PASS levels: 12582912 compared, 0 differ" \
    "$(grep ' levels:' "$H/out.txt")"
emu_check "the source's trigs (25 X / Y, 5 CHAOS, 3 parts, every DENS, 64 steps) = the original's rule" \
    "PASS trigs: 6144000 steps compared, 0 differ" "$(grep ' trigs:' "$H/out.txt")"
# lib/test_src_map.c's hashes, in 23-map.cc's order
held=$(sed -n '/^static const uint32_t hash_all/,/^};/p' "$R/lib/test_src_map.c" | grep -o '0x[0-9a-f]*' | tr '\n' ' ')
emu_check "lib/test_src_map.c holds the original's hashes" "$held" "$(grep '^hash' "$H/out.txt" | awk '{ print $NF }' | tr '\n' ' ')"

# ---- 2. the device ----
emu_build
emu_boot
check_take "boot: the track as seedref's new track"
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 3
emu_check "RHYTHM: SRC = MAP" 3 "$(emu_get 'recipe[0].src')"
emu_shot rhythm-map-defaults
# the title's source (design section 8): MAP while CHAOS draws no random numbers (below 4), S and the
# seed from CHAOS 4 on (emu.py title: a hash of the title bar)
t0=$(emu_py title "$EMU_SNAP")
emu_knob RHYTHM F:shared.map_chaos 3
emu_check "title: CHAOS 3 shows the same source as CHAOS 0 (MAP)" "$t0" "$(emu_py title "$EMU_SNAP")"
emu_knob RHYTHM F:shared.map_chaos 4
emu_shot rhythm-map-chaos4
t4=$(emu_py title "$EMU_SNAP")
emu_check "title: CHAOS 4 shows the seed (S1), not MAP" 1 "$([ "$t4" != "$t0" ] && echo 1 || echo "same $t4")"
emu_knob RHYTHM F:shared.map_chaos 0
emu_check "defaults: PART 1, DENS 128 (BD's recipe); X 128, Y 128, CHAOS 0 (the pattern's)" "0 128 128 128 0" \
    "$(emu_get 'recipe[0].map_part') $(emu_get 'recipe[0].map_dens') $(emu_get shared.map_x) $(emu_get shared.map_y) $(emu_get shared.map_chaos)"
emu_yes
check_take "YES: MAP at its defaults" src=map map_x=128 map_y=128
emu_knob RHYTHM B:map_part 1
emu_knob RHYTHM C:map_dens 200
emu_knob RHYTHM D:shared.map_x 100
emu_knob RHYTHM E:shared.map_y 200
emu_check "X and Y set the pattern's settings; BD's recipe has no X / Y" "100 200" "$(emu_get shared.map_x) $(emu_get shared.map_y)"
emu_yes
emu_shot rhythm-map
check_take "PART 2, DENS 200, X 100, Y 200, YES" src=map map_part=1 map_dens=200 map_x=100 map_y=200
emu_knob RHYTHM F:shared.map_chaos 255
emu_yes
check_take "CHAOS 255, YES: seed 1" src=map map_part=1 map_dens=200 map_x=100 map_y=200 map_chaos=255
emu_yes
check_take "YES with no knob turned: seed 2" src=map map_part=1 map_dens=200 map_x=100 map_y=200 map_chaos=255 seed=2
emu_page DYNAMICS
emu_knob DYNAMICS B:shape:one 9
emu_knob DYNAMICS C:amt 100
emu_yes
emu_shot dynamics-map
check_take "DYNAMICS SHAPE = MAP, AMT 100: the levels as velocity" src=map map_part=1 map_dens=200 map_x=100 map_y=200 \
    map_chaos=255 seed=2 shape=map amt=100
emu_no
check_take "NO: back to the original"
emu_yes
emu_keep
check_take "YES, FUNC + YES: kept (seed $(emu_get 'recipe[0].seed'))" src=map map_part=1 map_dens=200 map_x=100 map_y=200 \
    map_chaos=255 seed="$(emu_get 'recipe[0].seed')" shape=map amt=100
emu_done
