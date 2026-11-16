#!/bin/sh
# Task F12: MAP accents. With SRC = MAP, ACC with AMOD = TOP ranks trigs by the map key (level plus
# the part's chaos lift), so ACC at the share that counts them gives the original's accents.
#   1. on the host, against the original code (tools/tests/32-map-accents.cc built with the original
#      resources.cc and ReadDrumMap from the workspace's mi/eurorack/grids): pattern_generator.cc's
#      accent rule is still "level above 192"; seed_render's accents = the original's for every DENS
#      on a set of X / Y, parts, CHAOS, lengths and VARY
#   2. in the emulator, on A01 BD (a new project): SRC = MAP, DYNAMICS ACC at the share mapacc gives
#      and AMOD = TOP, YES against tools/seedref and the accent flags against the original's; again
#      with CHAOS 255; TOP with SRC = RAND still by the meter (seedref); NO; FUNC + YES keeps
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 32-map-accents

# ---- 1. the original code ----
G=$E/mi/eurorack/grids                          # E: the workspace (tools/emu.sh)
[ -f "$G/resources.cc" ] && [ -f "$G/pattern_generator.cc" ] || emu_die "no original map code in $G"
# EvaluateDrums: a step that fires is an accent when its (perturbed, clipped) level is above 192
rule=$(awk '/EvaluateDrums\(\) \{/ { on = 1 } on && /level > threshold/ { t = 1 } on && t && /if \(level > 192\)/ { a = 1 }
            on && a && /accent_bits \|= instrument_mask/ { print "ok"; exit } on && /^}/ { exit }' "$G/pattern_generator.cc")
emu_check "the original's accent: a step that fires, level above 192 (EvaluateDrums)" ok "$rule"
H=$D/mapacc; mkdir -p "$H/inc/grids"
: > "$H/inc/grids/resources.h"
awk '/^static const prog_uint8_t\* drum_map/ { on = 1 } on { print } on && /ReadDrumMap\(/ { fn = 1 } on && fn && /^}/ { exit }' \
    "$G/pattern_generator.cc" > "$H/reader.inc"
for f in "$R"/lib/*.c; do
    case $f in */test_*) continue ;; esac
    b=$(basename "$f" .c)
    cc -std=c99 -O2 -Wall -c "$f" -o "$H/$b.o" || emu_die "cc $b"
done
c++ -std=c++11 -O2 -Wall -Wno-unused-const-variable -I "$H/inc" -I "$R/lib" -DMAP_RESOURCES="\"$G/resources.cc\"" \
    -DMAP_READER="\"$H/reader.inc\"" "$T/tests/32-map-accents.cc" "$H"/*.o -o "$H/mapacc" 2> "$H/cc.log" ||
    { cat "$H/cc.log" >&2; emu_die "c++ mapacc"; }
"$H/mapacc" > "$H/out.txt"
emu_check "TOP's accents = the original's (36 X / Y, 3 parts, every DENS, 5 CHAOS, 16 / 32 / 64 steps, VARY 0 / 60)" \
    "PASS accents: 829440 takes, 30965760 steps compared, 0 differ, 0 without an ACC share" "$(grep ' accents:' "$H/out.txt")"

# accents ARGS...: the original's accents for PART DENS X Y CHAOS SEED LEN ("x . ..."); acc_of: its ACC share
accents() { "$H/mapacc" "$@" | sed -n 's/^accents //p'; }
acc_of() { "$H/mapacc" "$@" | sed -n 's/^acc //p'; }
# BD's accent flags in the emulator, one mark per step
bd_accents() { emu_dump | python3 -c "import sys; f=[l.split()[1:] for l in sys.stdin if l.startswith('flags ')][0]; print(' '.join('x' if int(v, 16) & 8 else '.' for v in f))"; }

# ---- 2. the device ----
emu_build
emu_boot
check_take "boot: the track as seedref's new track"
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 3
emu_knob RHYTHM C:map_dens 200
emu_knob RHYTHM D:shared.map_x 60
emu_knob RHYTHM E:shared.map_y 190
a=$(acc_of 0 200 60 190 0 1 16)
emu_check "the original has some accents and some plain trigs here (ACC $a %)" 1 "$([ "$a" -gt 0 ] && [ "$a" -lt 100 ] && echo 1)"
emu_page DYNAMICS
emu_knob DYNAMICS E:acc "$a"
emu_knob DYNAMICS F:amod:one 1
emu_yes
emu_shot dynamics-map-top
M="src=map map_dens=200 map_x=60 map_y=190"
check_take "MAP, ACC $a % TOP, YES" $M acc="$a" amod=top
emu_check "MAP, ACC $a % TOP: the accents are the original's" "$(accents 0 200 60 190 0 1 16)" "$(bd_accents)"
emu_page RHYTHM
emu_knob RHYTHM F:shared.map_chaos 255
a2=$(acc_of 0 200 60 190 255 1 16)
emu_page DYNAMICS
emu_knob DYNAMICS E:acc "$a2"
emu_yes
check_take "CHAOS 255, ACC $a2 % TOP, YES: seed 1" $M map_chaos=255 acc="$a2" amod=top
emu_check "CHAOS 255, ACC $a2 % TOP: the accents are the original's (with its lift)" \
    "$(accents 0 200 60 190 255 1 16)" "$(bd_accents)"
emu_page RHYTHM
emu_knob RHYTHM A:src:one 1
emu_yes
check_take "SRC = RAND, ACC $a2 % TOP: by the meter, as before" src=random map_x=60 map_y=190 map_chaos=255 \
    acc="$a2" amod=top
emu_no
check_take "NO: back to the original"
emu_knob RHYTHM A:src:one 3
emu_yes
emu_keep
check_take "SRC = MAP, YES, FUNC + YES: kept (seed $(emu_get 'recipe[0].seed'))" $M map_chaos=255 acc="$a2" amod=top \
    seed="$(emu_get 'recipe[0].seed')"
emu_done
