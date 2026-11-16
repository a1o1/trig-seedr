#!/bin/sh
# Task M2.1: interplay of the sources with PLACE, DYNAMICS and the TAKE knobs on the device, each take
# against tools/seedref (the host tests' breadth: mods/seedr-ar2/test_interplay.c).
#   1. 16 steps (kept2, A01 BD): the rows hidden while they cannot change the take (PHRS on a page,
#      CVAR and QUAL while the trig positions draw no random numbers, ROLE at BIAS 0, AMOD at ACC 0)
#      and shown again; SOURCE = TRACK with ROT, SYNC, QUAL, SHAPE STRONG and ACC TOP by the knobs.
#   2. 16 steps, recipes written into the store: EUCLID with MASK ONLY, RANK and TOP; COPY with ROT,
#      SKIP, SYNC, EVRY and CVAR; MAP with CHAOS, SYNC, SHAPE MAP, TOP and CVAR AAAB; RAND with GRID,
#      BIAS, ROLE, QUAL, SYNC, MASK AVOID, NOISE, VARY and CVAR 1:2.
#   3. 64 steps: PHRS shown; EUCLID 13 / 5 with PHRS ABAB keeps its cycle along the track; RAND
#      with PHRS ABCD, SYNC, SKIP, QUAL and WEAK; MAP with PHRS AAAB, ROT and SHAPE MAP.
#   4. 24 steps: SOURCE = TRACK with PHRS ABAB, ROT and SYNC keeps every trig of the last, short
#      page; NO restores the track.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 40-interplay
emu_build

BD=0x407dd8e3                                   # A01 BD's record (tools/emu.py REC0)
LENW=$(printf '%x' $((BD + 0x2c5)))             # its length byte, in the word with the speed (31-score)
trigs() { sed -n 2p; }
ntrigs() { trigs | tr ' ' '\n' | grep -c x; }
# moves ENCODER FIELD: 1 when one turn of the knob changes the field (a row is shown there), else 0;
# a change is turned back
moves() {
    _mv_a=$(emu_get "$2")
    emu_run -s 300M --nudge "$1:1@+600M"
    _mv_b=$(emu_get "$2")
    [ "$_mv_a" = "$_mv_b" ] && { echo 0; return 0; }
    emu_run -s 300M --nudge "$1:-1@+600M"
    echo 1
}
# take LABEL SEED KEY=VALUE...: the recipe written into BD's store at seed SEED - 1, SEED turned to
# SEED (a knob turn, so YES renders it), YES, against seedref with the context in $K $O
take() {
    _tk_l=$1 _tk_s=$2; shift 2
    emu_recipe 0 seed=$((_tk_s - 1)) "$@"
    emu_run -s 100M                             # the writes land before a knob reads its value
    emu_page TAKE
    emu_knob TAKE A:seed "$_tk_s"
    emu_yes
    check_take "$_tk_l" seed="$_tk_s" "$@" $K $O $M
}

# ---- 1. 16 steps: hidden and shown rows, and a SOURCE = TRACK take by the knobs ----
emu_fixture kept2 fix_kept2
save_orig kept2
emu_dump 1 > "$D/sd.txt"
K="orig=$D/kept2.txt" O="other=1:$D/sd.txt" M=
emu_open
emu_check "16 steps: TAKE PHRS hidden" 0 "$(moves 2 'recipe[0].phrs')"
emu_check "RAND: TAKE CVAR and QUAL shown" "1 1" "$(moves 3 'recipe[0].cvar') $(moves 6 'recipe[0].qual')"
emu_page RHYTHM
emu_check "RAND, BIAS 0: RHYTHM ROLE hidden" 0 "$(moves 4 'recipe[0].role')"
emu_knob RHYTHM D:bias 40
emu_check "BIAS 40: ROLE shown" 1 "$(moves 4 'recipe[0].role')"
emu_knob RHYTHM D:bias 0
emu_knob RHYTHM A:src:one 0
emu_page TAKE
emu_shot take-track
emu_check "SRC TRACK: CVAR and QUAL hidden" "0 0" "$(moves 3 'recipe[0].cvar') $(moves 6 'recipe[0].qual')"
emu_page PLACE
emu_knob PLACE A:rot 3
emu_knob PLACE B:sync 50
emu_page TAKE
emu_check "SRC TRACK, SYNC 50: CVAR and QUAL shown" "1 1" "$(moves 3 'recipe[0].cvar') $(moves 6 'recipe[0].qual')"
emu_knob TAKE G:qual 4
emu_page DYNAMICS
emu_check "ACC 0: DYNAMICS AMOD hidden" 0 "$(moves 5 'recipe[0].amod')"
emu_knob DYNAMICS B:shape:one 1
emu_knob DYNAMICS C:amt 100
emu_knob DYNAMICS E:acc 40
emu_knob DYNAMICS F:amod:one 1
emu_shot dynamics-track
emu_yes
check_take "TRACK, ROT 3, SYNC 50, QUAL 4, STRONG 100 %, ACC 40 % TOP" seed=2 src=track rot=3 sync=50 qual=4 shape=strong \
    amt=100 acc=40 amod=top "$K"
emu_check "TRACK, ROT, SYNC: every trig kept" "$(trigs < "$D/kept2.txt" | tr ' ' '\n' | grep -c x)" "$(emu_dump | ntrigs)"
emu_no
check_take "NO: back to the original" "$K"

# ---- 2. 16 steps, recipes in the store ----
take "EUCLID 5, MASK ONLY SD, RANK, ACC TOP" 3 src=euclid eu_steps=16 eu_puls=5 mask=only mtrk=1 vel=40:120 \
    shape=rank amt=100 acc=50 amod=top hum=20
take "COPY SD, ROT -2, SKIP 2, SYNC 40, EVRY 3, CVAR 1:2" 4 src=copy copy_trk=1 rot=-2 skip=2 sync=40 vel=30:110 \
    shape=evry n=3 amt=100 cvar=1:2
emu_poke shared.map_x 60
emu_poke shared.map_y 190
emu_poke shared.map_chaos 64
M="map_x=60 map_y=190 map_chaos=64"
take "MAP, CHAOS 64, SYNC 30, SHAPE MAP, ACC TOP, CVAR AAAB" 5 src=map map_dens=200 sync=30 vel=20:127 shape=map \
    amt=100 acc=30 amod=top cvar=aaab
emu_shot preview-map
M=
emu_poke shared.map_chaos 0
take "RAND, GRID 2, BIAS -50, PERC, QUAL 3, SYNC 60, MASK AVOID SD, NOISE, VARY 50, CVAR 1:2" 6 src=random dens=50 \
    grid=2 bias=-50 role=perc qual=3 sync=60 mask=avoid mtrk=1 vel=40:120 shape=noise n=4 amt=80 vary=50 cvar=1:2
emu_no
check_take "NO: back to the original" "$K"

# ---- 3. 64 steps ----
emu_run --poke "$LENW=40000200"                 # BD 64 steps long (+0x2c7, the speed, stays 2)
save_orig len64
K="orig=$D/len64.txt"
emu_check "64 steps: TAKE PHRS shown" 1 "$(moves 2 'recipe[0].phrs')"
take "64 steps, EUCLID 13 / 5, PHRS ABAB, VEL, HUM" 7 src=euclid eu_steps=13 eu_puls=5 phrs=abab vel=40:120 hum=30 \
    length=64
emu_check "EUCLID 13 / 5, PHRS ABAB: the cycle runs along the track, as with PHRS OFF" \
    "$(seedref src=euclid eu_steps=13 eu_puls=5 $K | trigs)" "$(emu_dump | trigs)"
take "64 steps, RAND, PHRS ABCD, SYNC 40, SKIP 4, QUAL 3, WEAK, ACC 25" 8 src=random dens=40 phrs=abcd sync=40 skip=4 \
    qual=3 vel=40:120 shape=weak amt=100 acc=25 length=64
emu_shot preview-64
emu_poke shared.map_chaos 64
M="map_x=60 map_y=190 map_chaos=64"
take "64 steps, MAP, PHRS AAAB, ROT 2, SHAPE MAP" 9 src=map map_dens=180 phrs=aaab rot=2 vel=30:120 shape=map amt=100 \
    length=64
M=
emu_poke shared.map_chaos 0
emu_no

# ---- 4. 24 steps: the last page is 8 steps; SOURCE = TRACK keeps its trigs through ROT and SYNC ----
emu_run --poke "$LENW=18000200"                 # BD 24 steps long
save_orig len24a
K="orig=$D/len24a.txt"
take "24 steps, RAND 60 % (to have trigs on the last page)" 10 src=random dens=60 length=24
emu_keep
save_orig len24
K="orig=$D/len24.txt"
emu_check "24 steps: trigs on steps 17-24" 1 "$([ "$(trigs < "$D/len24.txt" | cut -d' ' -f18-25 | grep -c x)" = 1 ] && echo 1)"
take "24 steps, TRACK, PHRS ABAB, ROT 3, SYNC 50, VEL" 11 src=track phrs=abab rot=3 sync=50 vel=40:120
emu_shot phrs-24
emu_check "24 steps, PHRS, ROT, SYNC: every trig kept" "$(trigs < "$D/len24.txt" | tr ' ' '\n' | grep -c x)" \
    "$(emu_dump | ntrigs)"
emu_no
check_take "NO: the 24 steps as they were" "$K"
emu_done
