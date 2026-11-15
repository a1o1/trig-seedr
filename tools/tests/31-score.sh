#!/bin/sh
# Task F11: QUAL (candidate scoring) on the device.
#   1. kept2's RANDOM take (seed 2): QUAL 4 on TAKE, YES, against tools/seedref; BIAS 60 with QUAL 4;
#      ACC and COND with QUAL 4 leave the trigs where they were (the score reads only positions); NO
#   2. a 64-step recipe with most layers on written into the store, QUAL turned to 8: YES against seedref
#   3. the cost of a render at QUAL 1, 4 and 8 on that 64-step recipe, from the instructions the
#      emulator executes (30-preview's method; build/emu/31-score/cost.txt)
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 31-score
emu_build

# trigs: the trig row of BD's dump
trigs() { emu_dump | sed -n 2p; }

# ---- 1. QUAL on kept2 ----
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_knob TAKE G:qual 4
emu_shot take
emu_yes
check_take "QUAL 4" src=random seed=2 qual=4 "$K"
emu_check "QUAL 4: another take than QUAL 1" 1 \
    "$([ "$(seedref src=random seed=2 "$K" | sed -n 2p)" != "$(trigs)" ] && echo 1)"
q4=$(trigs)
emu_page RHYTHM
emu_knob RHYTHM D:bias 60
emu_yes
check_take "QUAL 4, BIAS 60" src=random seed=2 bias=60 qual=4 "$K"
emu_knob RHYTHM D:bias 0
emu_page DYNAMICS
emu_knob DYNAMICS E:acc 30
emu_page TRIG
emu_knob TRIG A:cond 40
emu_yes
check_take "QUAL 4, ACC 30, COND 40" src=random seed=2 qual=4 acc=30 cond=40 "$K"
emu_check "QUAL 4, ACC 30, COND 40: the trigs of QUAL 4 alone" "$q4" "$(trigs)"
emu_no
emu_check "NO: back to the original" "$(cat "$D/kept2.txt")" "$(emu_dump)"

# ---- 2. 64 steps, most layers on, QUAL 8 ----
HEAVY="src=random seed=7 bias=-40 rot=3 skip=1 vel=40:120 shape=not1 amt=50 acc=30 hum=20 note=48:72
    lane1.pct=50 lane2.pct=50 cond=30 rtrg=20 rand=10"
emu_run --poke 407ddba8=40000200          # BD 64 steps long (record +0x2c5; +0x2c7, the speed, stays 2)
save_orig len64
# shellcheck disable=SC2086
emu_recipe 0 $HEAVY dens=60 qual=7
emu_page TAKE
emu_knob TAKE G:qual 8                    # a knob turned: YES renders this seed, not the next (V2)
emu_yes
emu_shot take-64
# shellcheck disable=SC2086
check_take "64 steps, most layers, QUAL 8" $HEAVY dens=60 qual=8 "orig=$D/len64.txt"
emu_no

# ---- 3. the cost. A run of 16 DENS turns, each a new value and a render, against the same turns
# with DENS held at 0 (no change, no render): the difference is the renders'. ----
executed() { sed -n 's/.*instructions (\([0-9]*\) executed.*/\1/p' "${EMU_SNAP%.snap}.log"; }
renders() { emu_get pv_renders; }
emu_page RHYTHM
# cost QUAL RECIPE...: on BD as it is, with RECIPE, QUAL and DENS
cost() {
    _c_q=$1; shift
    _c_s=$EMU_SNAP
    emu_recipe 0 "$@" qual="$_c_q" dens=0
    emu_run -s 600M                                 # the recipe's own render, before the count
    _c_r=$(renders)
    emu_run -s 1700M --nudge 2:-1@+600Mx16/80M
    _c_b=$(executed) _c_rb=$(($(renders) - _c_r))
    emu_at "$_c_s"
    emu_recipe 0 "$@" qual="$_c_q" dens=40
    emu_run -s 600M
    _c_r=$(renders)
    emu_run -s 1700M --nudge 2:1@+600Mx16/80M
    _c_t=$(executed) _c_rt=$(($(renders) - _c_r)) _c_d=$(($(emu_get 'recipe[0].dens') - 40))
    emu_check "cost, QUAL $_c_q: DENS held at 0, no render" 0 "$_c_rb"
    emu_check "cost, QUAL $_c_q: a render for each DENS value" "$_c_d" "$_c_rt"
    emu_at "$_c_s"
    [ "$_c_rt" -gt 0 ] || return 0
    _c_e=$(((_c_t - _c_b) / _c_rt))
    echo "64 steps, most layers on, QUAL $_c_q: $_c_rt renders, $_c_e instructions each" \
        "($((_c_e / 200)) us at the emulator's 200 M a second)" >> "$D/cost.txt"
}
: > "$D/cost.txt"
for q in 1 4 8; do
    # shellcheck disable=SC2086
    cost $q $HEAVY
done
sed 's/^/  /' "$D/cost.txt"
emu_done
