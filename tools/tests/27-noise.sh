#!/bin/sh
# Task F7: NOISE on the device. From kept2 (seed 2 kept), BD's recipe is written with the ranges
# (NOTE -7..+7, VEL 20..120; emu_recipe), then F7's knobs are turned by hand, each take against
# tools/seedref:
#   1. NOTE: NSRC = NOISE, ZOOM 3
#   2. LOCK lane 1 (SMP): % 100, SRC = NOISE, ZOOM 8
#   3. DYNAMICS: SHAPE = NOISE, AMT 100, N 6
#   4. NO back to seed 2; VARY 100: seed 3 gives seed 2's contours (P3); kept, SAVE + RELOAD PATTERN
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 27-noise
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_recipe 0 src=random seed=2 note=-7:7 vel=20:120
emu_open
emu_check "the injected ranges are the mod's" "-7 7 20 120" \
    "$(emu_get 'recipe[0].note.lo') $(emu_get 'recipe[0].note.hi') $(emu_get 'recipe[0].vel.lo') $(emu_get 'recipe[0].vel.hi')"

# ---- 1. NOTE ----
emu_page NOTE
emu_knob NOTE D:note.src:one 2
emu_knob NOTE G:note.zoom 3
emu_yes
emu_shot note
N="note=-7:7 note.src=noise note.zoom=3 vel=20:120"
# shellcheck disable=SC2086
check_take "NOTE NOISE -7..+7, ZOOM 3" src=random seed=2 $N "$K"

# ---- 2. LOCK lane 1 ----
emu_page LOCK
emu_knob LOCK B:lane1.pct 100
emu_knob LOCK D:lane1.src:one 2
emu_knob LOCK G:lane1.zoom 8
emu_yes
emu_shot lock
L="lane1.pct=100 lane1.src=noise lane1.zoom=8"
# shellcheck disable=SC2086
check_take "LOCK SMP 100 %, NOISE 1..8, ZOOM 8" src=random seed=2 $N $L "$K"

# ---- 3. DYNAMICS ----
emu_page DYNAMICS
emu_knob DYNAMICS B:shape:one 7
emu_knob DYNAMICS C:amt 100
emu_knob DYNAMICS D:n 6
emu_yes
emu_shot dynamics
S="shape=noise amt=100 n=6"
# shellcheck disable=SC2086
check_take "SHAPE NOISE 100 %, N 6" src=random seed=2 $N $L $S "$K"

# ---- 4. NO, VARY, keep ----
emu_no
check_take "NO: seed 2" src=random seed=2 "$K"
emu_page TAKE
emu_knob TAKE B:vary 100
emu_yes
# shellcheck disable=SC2086
check_take "VARY 100, seed 2" src=random seed=2 vary=100 $N $L $S "$K"
V2=$(emu_dump)
emu_knob TAKE A:seed 3
emu_yes
emu_check "VARY 100: seed 3 is seed 2's take, contours included (P3)" "$V2" "$(emu_dump)"
emu_keep
emu_no
emu_keys YES+SCNE w:300 NO+SCNE
# shellcheck disable=SC2086
check_take "kept, SAVE + RELOAD PATTERN" src=random seed=3 vary=100 $N $L $S "$K"
emu_done
