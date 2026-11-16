#!/bin/sh
# Task M2.3: performance and size, on the emulator's instruction counter (docs/status/M2.3.md).
#   1. size: the mod's .run and .bss, each object's text and bss, the loader's spare RAM and flash
#   2. the cost of a render (the preview's, seed_render) per source and per layer on a 64-step BD,
#      by 30-preview's method: 16 SEED turns on TAKE, each a new recipe and one render, against 16
#      turns held at SEED 0 (no change, no render); the difference divided by the renders. Since
#      M2.3 seedr draws a frame only after an event or a change, so both runs draw one frame a turn
#      and nothing between: the difference is the renders' (before M2.3 every tick drew the strip,
#      whose cost follows the take shown, so the two runs differed by more than the renders). A
#      layer's cost is RANDOM + the layer less RANDOM alone. A frame with no render compares the
#      preview's inputs in full (preview.c), one with a new recipe stops at the first byte, so
#      each figure is the render less that compare (about 6,400 instructions: TRACK shows it);
#      the layers' shares, differences of two figures, carry no such offset.
#   3. the cost of seedr's screen while nothing changes: 1000M instructions (5 s at the emulator's
#      200 M a second) open on RHYTHM, on VIEW, playing, against the same with seedr closed
#   4. the screen still follows what changes outside seedr's events (M2.3 draws a frame only when
#      something differs): a trig recorded with stock's keys, another track changed, a message
#      running out; each screen is compared with the one a full redraw (LEFT, RIGHT) gives
#   5. lock rows: those a take with both lanes at 100 % on BD uses
# Results: build/emu/42-perf/perf.txt.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 42-perf
emu_build
OUT=$D/perf.txt
: > "$OUT"
note() { echo "$*" >> "$OUT"; }

# ---- 1. size ----
_b=$(grep '^BUILT' "$W/mod-seedr/build.log" | tail -1 | sed 's/.*: //')
note "size: $_b"
note "loader: $(grep '^linked' "$W/mod-seedr/build.log" | sed 's/.*: RAM //')"
note "patched OS: $(grep '^verified' "$W/mod-seedr/build.log" | sed 's/.*flash ends //')"
note "objects (text data bss):"
${ELEKLOADER_CROSS}size "$W"/mod-seedr/seedr.work/*.c.o | awk 'NR > 1 { n = $6; sub(".*/", "", n); sub(".c.o$", "", n);
    printf "  %-14s %6d %4d %6d\n", n, $1, $2, $3 }' | sort -k2 -n -r >> "$OUT"
emu_check "size: built, with a .run and a .bss" 1 "$(echo "$_b" | grep -c '^.run [0-9]*, .fast 0, .bss [0-9]* bytes$')"

# ---- helpers ----
executed() { sed -n 's/.*instructions (\([0-9]*\) executed.*/\1/p' "${EMU_SNAP%.snap}.log"; }
renders() { emu_get pv_renders; }
NEW=; emu_has scr_always && NEW=1             # a build with M2.3's redrawing (the "before" build has none)
always() { [ -n "$NEW" ] && emu_poke scr_always "$1"; return 0; }
screen() { "$DTPEEK" "$EMU_SNAP" fb | cksum; }
# full_redraw: LEFT then RIGHT, back on the same page: a frame drawn in full
full_redraw() { emu_keys LEFT RIGHT; }

# cost LABEL RECIPE...: instructions a render on BD with RECIPE, as the header says (TAKE showing).
# Sets COST.
cost() {
    _c_l=$1; shift
    _c_s=$EMU_SNAP
    # shellcheck disable=SC2086
    emu_recipe 0 "$@" seed=0
    emu_run -s 600M                                 # the recipe's own render, before the count
    _c_r=$(renders)
    emu_run -s 1700M --nudge 0:-1@+600Mx16/80M
    _c_b=$(executed) _c_rb=$(($(renders) - _c_r))
    emu_at "$_c_s"
    # shellcheck disable=SC2086
    emu_recipe 0 "$@" seed=100
    emu_run -s 600M
    _c_r=$(renders)
    emu_run -s 1700M --nudge 0:1@+600Mx16/80M
    _c_t=$(executed) _c_rt=$(($(renders) - _c_r)) _c_d=$(($(emu_get 'recipe[0].seed') - 100))
    emu_at "$_c_s"
    COST=
    emu_check "cost, $_c_l: SEED held at 0, no render" 0 "$_c_rb"
    emu_check "cost, $_c_l: a render for each SEED value" "$_c_d" "$_c_rt"
    [ "$_c_rt" -gt 0 ] || return 0
    COST=$(((_c_t - _c_b) / _c_rt))
}
# row LABEL COST [BASE]: a line of the table; with BASE, also COST - BASE
row() {
    if [ -n "$3" ]; then note "$(printf '  %-34s %9d  %+9d  %6d us' "$1" "$2" $(($2 - $3)) $((($2 - $3) / 200)))"
    else note "$(printf '  %-34s %9d  %6d us' "$1" "$2" $(($2 / 200)))"; fi
}

# ---- 2. render cost: BD 64 steps (record +0x2c5), from seed 2 kept; SD has a new project's trigs ----
emu_fixture kept2 fix_kept2
emu_open
emu_run --poke 407ddba8=40000200
save_orig len64
emu_page TAKE
emu_check "BD is 64 steps long" 64 "$(head -1 "$D/len64.txt" | cut -d' ' -f2)"

note "render cost per source, 64 steps (instructions a render, less the compare of an unchanged frame, see the header; time at 200 M a second):"
for s in "TRACK|" "RANDOM DENS 50|src=random dens=50" "EUCLID 16 / 5|src=euclid eu_steps=16 eu_puls=5" \
         "MAP DENS 150|src=map map_dens=150" "COPY SD|src=copy copy_trk=1"; do
    # shellcheck disable=SC2086
    cost "${s%%|*}" ${s#*|}
    [ -n "$COST" ] && row "${s%%|*}" "$COST"
done

BASE="src=random dens=50"
# shellcheck disable=SC2086
cost "RANDOM alone" $BASE; C0=$COST
note "render cost per layer, 64 steps, on RANDOM DENS 50 ($C0 a render alone); the layer's share:"
for s in "PLACE ROT 3|rot=3" "PLACE SYNC 50|sync=50" "PLACE MASK AVOID SD|mask=avoid mtrk=1" \
         "PLACE SKIP 4|skip=4" "PLACE FLD ALL|fld=all frot=1" \
         "DYNAMICS VEL range|vel=40:120" "DYNAMICS SHAPE RANK|vel=40:120 shape=rank amt=100" \
         "DYNAMICS SHAPE NOISE|vel=40:120 shape=noise amt=100 n=8" "DYNAMICS ACC TOP 30|acc=30 amod=top" \
         "DYNAMICS HUM 20|hum=20" \
         "NOTE range|note=-5:7" "NOTE LIST 16 RAND|note=-12:12 note.src=list note.len=16 note.order=rand" \
         "NOTE NOISE|note=-12:12 note.src=noise note.zoom=8" "NOTE LEGATO|lmod=legato" \
         "LOCK lane 1 100 %|lane1.pct=100" "LOCK both lanes 100 %|lane1.pct=100 lane2.pct=100" \
         "TRIG COND 30|cond=30" "TRIG RTRG 20|rtrg=20" \
         "TIME RAND 20|rand=20" "TIME DRFT 50|drft=50" "TIME GRV 1|grv=1" \
         "TAKE VARY 50|vary=50" "TAKE VARY 90|vary=90" "TAKE PHRS ABAB|phrs=abab" "TAKE CVAR 1:2|cvar=1:2" \
         "TAKE QUAL 8|qual=8" \
         "three 16-item lists, VARY 90|vary=90 note.src=list note.len=16 note.order=rand lane1.pct=100 lane1=1:100 lane1.src=list lane1.len=16 lane1.order=rand lane2.pct=100 lane2=0:127 lane2.src=list lane2.len=16 lane2.order=rand"; do
    # shellcheck disable=SC2086
    cost "${s%%|*}" $BASE ${s#*|}
    [ -n "$COST" ] && row "${s%%|*}" "$COST" "$C0"
done
HEAVY="src=random seed=7 bias=-40 rot=3 skip=1 vel=40:120 shape=not1 amt=50 acc=30 hum=20 note=48:72
    lane1.pct=50 lane2.pct=50 cond=30 rtrg=20 rand=10 dens=60"
# shellcheck disable=SC2086
cost "most layers (30-preview's)" $HEAVY
[ -n "$COST" ] && row "most layers (30-preview's)" "$COST"

# ---- 3. the screen while nothing changes: RHYTHM, VIEW, playing; seedr open against closed ----
# idle LABEL: 1000M from here; sets IDLE (instructions executed) and DRAWS (frames seedr drew, in full)
idle() {
    emu_run -s 600M
    _i_d=0 _i_f=0
    [ -n "$NEW" ] && _i_d=$(emu_get scr_draws) _i_f=$(emu_get scr_full)
    emu_run -s 1000M
    IDLE=$(executed) DRAWS=n/a
    [ -n "$NEW" ] && DRAWS="$(($(emu_get scr_draws) - _i_d)) ($(($(emu_get scr_full) - _i_f)) in full)"
}
# pct A B: (A - B) over 5 s as a share of 200 M a second, to 0.01 %
pct() { _p=$((($1 - $2) / 100000)); printf '%d.%02d %%' $((_p / 100)) $((_p % 100)); }
emu_recipe 0 $BASE
emu_page RHYTHM
S_OPEN=$EMU_SNAP
note "the screen while nothing changes, 1000M instructions (5 s), 64 steps, RANDOM DENS 50:"
idle; I_RH=$IDLE; D_RH=$DRAWS
emu_page VIEW
idle; I_VW=$IDLE; D_VW=$DRAWS
emu_at "$S_OPEN"; emu_page TAKE
idle; I_TK=$IDLE; D_TK=$DRAWS
emu_at "$S_OPEN"
emu_keys NO
idle; I_CL=$IDLE
emu_keys PLAY
idle; I_PC=$IDLE
emu_at "$S_OPEN"
emu_keys PLAY
idle; I_PO=$IDLE; D_PO=$DRAWS
note "  seedr closed (stock's TRIG page): $I_CL executed"
note "  RHYTHM: $I_RH executed, seedr drew $D_RH frames; seedr's screen $(((I_RH - I_CL) / 5)) a second, $(pct "$I_RH" "$I_CL")"
note "  TAKE:   $I_TK executed, seedr drew $D_TK frames; $(((I_TK - I_CL) / 5)) a second, $(pct "$I_TK" "$I_CL")"
note "  VIEW:   $I_VW executed, seedr drew $D_VW frames; $(((I_VW - I_CL) / 5)) a second, $(pct "$I_VW" "$I_CL")"
note "  playing, closed: $I_PC executed; RHYTHM open: $I_PO, seedr drew $D_PO frames;" \
    "$(((I_PO - I_PC) / 5)) a second, $(pct "$I_PO" "$I_PC")"
if [ -n "$NEW" ]; then
    emu_at "$S_OPEN"
    always 1
    idle; I_AL=$IDLE
    note "  RHYTHM with scr_always = 1 (every tick drawn in full, as before M2.3): $I_AL executed;" \
        "$(((I_AL - I_CL) / 5)) a second, $(pct "$I_AL" "$I_CL")"
    emu_check "idle on RHYTHM: no frame drawn in full" 0 "$(echo "$D_RH" | sed 's/.*(\([0-9]*\) in full)/\1/')"
    emu_check "idle on RHYTHM: seedr's screen costs below 0.5 % of the CPU" 1 \
        "$([ $((I_RH - I_CL)) -lt 5000000 ] && echo 1)"
    emu_check "idle on RHYTHM: below a fifth of drawing every tick" 1 \
        "$([ $((5 * (I_RH - I_CL))) -lt $((I_AL - I_CL)) ] && echo 1)"
fi

# ---- 4. the screen follows changes made outside seedr's events ----
emu_at "$S_OPEN"
emu_page VIEW
emu_shot view-before
s0=$(screen)
emu_rec 1
emu_keys T5
emu_rec 0
emu_shot view-t5
s1=$(screen)
full_redraw
emu_check "VIEW, RECORD + T5 (stock's keys): the screen changed" 1 "$([ "$s0" != "$s1" ] && echo 1)"
emu_check "VIEW, RECORD + T5: the screen is what a full redraw draws" "$(screen)" "$s1"

emu_page RHYTHM
emu_knob RHYTHM A:src:one 4
emu_knob RHYTHM B:copy_trk 1
emu_run -s 600M
s0=$(screen)
emu_rec 0
emu_keys TRK+T2
emu_rec 1
emu_keys T7 T8
emu_rec 0
emu_keys TRK+T1
emu_shot rhythm-copy-sd
s1=$(screen)
full_redraw
emu_check "COPY SD, SD changed with stock's keys: BD's preview changed" 1 "$([ "$s0" != "$s1" ] && echo 1)"
emu_check "COPY SD, SD changed: the screen is what a full redraw draws" "$(screen)" "$s1"

s0=$(screen)
emu_rec 0
emu_run -s 100M --press YES@+100M
emu_check "YES without RECORD: the message" "RECORD TO GENERATE" "$(emu_msg)"
emu_check "the message replaces the strip" 1 "$([ "$(screen)" != "$s0" ] && echo 1)"
emu_run -s 600M
emu_check "2 s later: the strip is back" "$s0" "$(screen)"

emu_keys PLAY
emu_run -s 600M
s1=$(screen)
full_redraw
emu_check "playing: the screen is what a full redraw draws" "$(screen)" "$s1"
emu_keys STOP

# ---- 5. lock rows: a take with lane 1 (SMP) and lane 2 (FRQ) at 100 % on the 64 steps of BD ----
# The pattern's rows (device/rytm_ar2.h rt_rows): behind BD's track object (the device layer's
# snapshot keeps it: rt_seed_state's s, snap[0].obj) +0x2c, at +0x20; 72 rows of 66 bytes.
SNAPSYM=$(python3 -c "import json, re, sys; m = json.load(open(sys.argv[1]))
s = [k for k in m if re.fullmatch(r'seedr:s\.[0-9]+', k)]; print(s[0].split(':')[1] if len(s) == 1 else '')" "$MAP")
rows() {
    python3 - "$EMU_SNAP" "$(emu_py sym "$MAP" "$SNAPSYM")" <<'PY'
import os, sys
sys.path.insert(0, os.path.join(os.environ['T']))
import emu
snap, s = sys.argv[1], int(sys.argv[2], 0)
w = lambda a: int.from_bytes(emu.mem(snap, a, 4), 'big')
t = w(w(w(s) + 0x2c) + 0x20)
m = emu.mem(snap, t, 72 * 66)
r = [m[66 * i:66 * i + 2] for i in range(72)]
used = [x for x in r if x[0] != 0xff and x[1] != 0xff]
print(len(used), sum(1 for x in used if x[1] == 0))
PY
}
export T
emu_at "$S_OPEN"
emu_check "lock rows: the device layer's snapshot symbol" 1 "$([ -n "$SNAPSYM" ] && echo 1)"
emu_yes
r0=$(rows)
emu_no
emu_recipe 0 $BASE lane1.pct=100 lane2.pct=100
emu_yes
r1=$(rows)
emu_no
r2=$(rows)
note "lock rows (used of 72; BD's): the pattern before: ${r0}; a take with both lanes at 100 % on 64 steps: ${r1};" \
    "after NO: ${r2}"
emu_check "lock rows: the take adds a row on BD for each lane's parameter (SMP, FRQ)" 2 $((${r1#* } - ${r0#* }))
emu_check "lock rows: NO gives them back" "$r0" "$r2"

sed 's/^/  /' "$OUT"
emu_done
