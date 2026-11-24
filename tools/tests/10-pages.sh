#!/bin/sh
# Task 0.3, the page framework, in analog-emu: PAGE and the TRIG keys left to stock while seedr is
# open, LEFT / RIGHT over the pages that are built, YES / NO / FUNC + YES on a recipe (SOURCE =
# TRACK with every layer at 0 leaves the track byte-identical), the TRIG cycle, TOOLS' ROTATE ALL,
# VIEW's knob cursor, and a screenshot of every page (a SEEDR_SHOW_ALL build shows the pages whose
# features are not built yet, as their knob tables stand).
# Everything it writes goes under build/emu/10-pages/ of this checkout; the two builds (the mod,
# and SEEDR_SHOW_ALL) are the harness's (tools/emu.sh emu_build).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"
emu_init 10-pages
EMU_QUIET=1 emu_build showall -DSEEDR_SHOW_ALL; FW_ALL=$FW MAP_ALL=$MAP
EMU_QUIET=1 emu_build; FW_MOD=$FW MAP_MOD=$MAP
W=$D; mkdir -p $W; cd $W

# ---- helpers. Each snapshot's clock (in M instructions) is kept beside it in SNAP.clk; `from SNAP`
# sets CLK to it, and `at N` is the clock N M later, for a run that starts from SNAP.
FW=$FW_MOD MAP=$MAP_MOD
from() { CLK=$(cat $1.clk); }
at() { echo "$((CLK + $1))M"; }
run() {   # in out N args: N M instructions from snapshot in
    i=$1; o=$2; n=$3; shift 3
    $B/dtboot $FW --load $i --instr ${n}M "$@" --save $o > /dev/null
    echo $(($(cat $i.clk) + n)) > $o.clk
}
key() {   # in out KEY: one press 100 M into a 300 M run
    from $1; run $1 $2 300 --press $3@$(at 100); }
fkey() {  # in out KEY: FUNC held, KEY pressed
    from $1; run $1 $2 600 --press FUNC@$(at 100):400M --press $3@$(at 250); }
open_() { # in out: TRIG held + PAGE, the shortcut
    from $1; run $1 $2 600 --press TRIG@$(at 100):400M --press PAGE@$(at 250); }
png() { DTEMU_FW=$FW $B/dtpeek $1 fb | python3 $T/fb2png.py $W/$2.png 4 > /dev/null; }
screen() { DTEMU_FW=$FW $B/dtpeek $1 fb | cksum | cut -d' ' -f1; }
steps() { python3 $T/emucmp.py $B/dtpeek $FW $1 32 ${2:-0} | head -1; }
# BD's whole record (0x395 bytes) and lock store (0x162a) in A01
bytes() { DTEMU_FW=$FW $B/dtpeek $1 mem 407dd8e3 917 | cksum; DTEMU_FW=$FW $B/dtpeek $1 mem 407e0774 5674 | cksum; }
tled() { DTEMU_FW=$FW $B/dtpeek $1 leds | grep -E '^led ([0-9]|1[0-5]):'; }   # the TRIG keys' LEDs
same() { emu_check "$3" "$1" "$2"; }
# hknob in out PAGE KNOB VALUE: tools/emu.sh's emu_knob (PAGE showing) from snapshot in; this script
# keeps clocks in M instructions, tools/emu.sh in instructions. FW and MAP: the build in use.
hknob() {
    export DTEMU_FW=$FW
    cp $1 $W/hk.snap; echo $(($(cat $1.clk) * 1000000)) > $W/hk.snap.clk
    emu_at $W/hk.snap; emu_knob $3 $4 $5
    cp "$EMU_SNAP" $2; echo $(($(cat "$EMU_SNAP.clk") / 1000000)) > $2.clk
}
differ() { if [ "$1" != "$2" ]; then emu_check "$3" 1 1; else emu_check "$3" "not $1" "$2"; fi; }
boot() {
    $B/dtcard $W/card.img format > /dev/null
    $B/dtboot $FW --instr 5.6G --card $W/card.img --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G --save $1 > /dev/null
    echo 5600 > $1.clk
}
# A01 BD made 32 steps long (record +0x2c5; +0x2c7, the speed, stays 2), then on the TRIG page with
# grid recording: T1, T5, PAGE, T1, T9 = steps 1, 5, 17, 25, from CLK on
LEN32=407ddba8=20000200
keys32() { echo "--press RECORD@$(at 100) --press T1@$(at 200) --press T5@$(at 300) --press PAGE@$(at 400) \
    --press T1@$(at 600) --press T9@$(at 700) --press RECORD@$(at 900)"; }

# ================= the mod as built (features that are not built yet are hidden)
boot $W/boot.snap
# 1. stock alone, then the same keys with seedr open (V10)
from $W/boot.snap; run $W/boot.snap $W/k0.snap 1600 --poke $LEN32 $(keys32)
from $W/boot.snap; run $W/boot.snap $W/k1.snap 600 --poke $LEN32 --press TRIG@$(at 100):400M --press PAGE@$(at 250)
from $W/k1.snap; run $W/k1.snap $W/k2.snap 1600 $(keys32)
png $W/k2.snap seedr-take
same "xff .ff .ff .ff xff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff xff .ff .ff .ff .ff .ff .ff .ff xff .ff .ff .ff .ff .ff .ff .ff " \
    "$(steps $W/k0.snap)" "stock: grid recording with PAGE puts trigs on 1 5 17 25"
same "$(steps $W/k0.snap)" "$(steps $W/k2.snap)" "seedr open: RECORD, the TRIG keys and PAGE do the same"
same "$(tled $W/k0.snap)" "$(tled $W/k2.snap)" "seedr open: the TRIG keys' LEDs as stock's (page 2 of 2)"
# (the strip is the preview of BD's recipe, RANDOM on the empty track (V6): the recorded trigs do not change it)
same "$(DTEMU_FW=$FW python3 $T/emu.py title $W/k1.snap)" "$(DTEMU_FW=$FW python3 $T/emu.py title $W/k2.snap)" \
    "seedr's screen stays up"

# 2. LEFT / RIGHT: with 0.2's layers every page is built: RIGHT visits eleven different pages (TURING,
#    0.9, the ring's end) and wraps to TAKE; LEFT from TAKE is TURING
prev=$W/k2.snap; seen=""; pg=0
for name in take rhythm place dynamics note lock trig time view tools turing; do   # (not i: run() sets i)
    pg=$((pg + 1)); png $prev seedr-$name
    key $prev $W/p$pg.snap RIGHT; seen="$seen $(screen $prev)"; prev=$W/p$pg.snap
done
same 11 "$(echo $seen | tr ' ' '\n' | sort -u | wc -l | tr -d ' ')" "RIGHT: eleven different pages"
same "$(screen $W/k2.snap)" "$(screen $W/p11.snap)" "RIGHT x11: back on TAKE"
key $W/p11.snap $W/p4.snap LEFT
same "$(screen $W/p10.snap)" "$(screen $W/p4.snap)" "LEFT from TAKE: TURING"

# 3. VIEW: knob A moves the step cursor
key $W/p4.snap $W/p4t.snap LEFT                      # TOOLS
key $W/p4t.snap $W/v0.snap LEFT
from $W/v0.snap; run $W/v0.snap $W/v1.snap 600 --nudge 0:1@$(at 100) --nudge 0:1@$(at 300)
png $W/v1.snap seedr-view-cursor
differ "$(screen $W/v0.snap)" "$(screen $W/v1.snap)" "VIEW: knob A moves the cursor"
same "$(bytes $W/v0.snap)" "$(bytes $W/v1.snap)" "VIEW: the track is not touched"

# 4. YES with SOURCE = TRACK and every layer at 0: byte-identical (I1 through the device), an audition;
#    NO; YES again (the next seed); FUNC + YES keeps; NO closes
key $W/v1.snap $W/y0.snap RIGHT                      # TOOLS
key $W/y0.snap $W/y0t.snap RIGHT                     # TURING
key $W/y0t.snap $W/ya.snap RIGHT                     # TAKE
key $W/ya.snap $W/yb.snap RIGHT                      # RHYTHM
# BD's recipe was made when seedr opened on the empty BD (section 1), so it is RANDOM (V6), and it
# stays so after the trigs were recorded: SOURCE = TRACK by hand
hknob $W/yb.snap $W/yc.snap RHYTHM A:src:one 0
key $W/yc.snap $W/ycr.snap RECORD                    # grid recording on: generating needs it
key $W/ycr.snap $W/y1.snap LEFT                      # TAKE
key $W/y1.snap $W/y2.snap YES; png $W/y2.snap seedr-take-audition
same "$(bytes $W/y1.snap)" "$(bytes $W/y2.snap)" "YES, SOURCE = TRACK, layers at 0: record and locks byte-identical"
differ "$(screen $W/y1.snap)" "$(screen $W/y2.snap)" "YES: the title shows the audition"
from $W/y2.snap; run $W/y2.snap $W/y3.snap 700 --press NO@$(at 100)    # past the 2 s message
same "$(screen $W/y1.snap)" "$(screen $W/y3.snap)" "NO: back to the original, no audition"
key $W/y3.snap $W/y4.snap YES
key $W/y4.snap $W/y5.snap YES; png $W/y5.snap seedr-take-seed2
differ "$(screen $W/y4.snap)" "$(screen $W/y5.snap)" "YES with no knob turned: the next seed (SEED 2, then 3)"
fkey $W/y5.snap $W/y6.snap YES
same "$(bytes $W/y1.snap)" "$(bytes $W/y6.snap)" "FUNC + YES: kept, track unchanged"
key $W/y6.snap $W/y7.snap NO
differ "$(screen $W/y6.snap)" "$(screen $W/y7.snap)" "NO with nothing auditioned: seedr closes"

# 5. The TRIG cycle (D17), closed on stock's TRIG page: TRIG -> EUCLIDEAN, TRIG -> seedr, TRIG -> stock's
#    TRIG, TRIG -> EUCLIDEAN, TRIG -> seedr; a parameter page key (SRC) closes it
key $W/y7.snap $W/c1.snap TRIG; key $W/c1.snap $W/c2.snap TRIG; key $W/c2.snap $W/c3.snap TRIG
key $W/c3.snap $W/c4.snap TRIG; key $W/c4.snap $W/c5.snap TRIG; key $W/c5.snap $W/c6.snap SRC
c1=$(screen $W/c1.snap); c2=$(screen $W/c2.snap); c3=$(screen $W/c3.snap); c4=$(screen $W/c4.snap); c5=$(screen $W/c5.snap)
png $W/c1.snap stock-euclidean; png $W/c2.snap seedr-from-trig-cycle
differ "$c1" "$c2" "TRIG on EUCLIDEAN opens seedr"
differ "$c2" "$c3" "TRIG on seedr: stock's TRIG page"
if [ "$c1" = "$c4" ] && [ "$c3" != "$c4" ]; then echo "PASS TRIG: EUCLIDEAN again"; else echo "FAIL TRIG: EUCLIDEAN again"; fi
same "$c2" "$c5" "TRIG on EUCLIDEAN: seedr again"
differ "$c5" "$(screen $W/c6.snap)" "SRC closes seedr"

# 6. TOOLS, ROTATE ALL +1 over every track: BD 1 5 17 25 -> 2 6 18 26; NO puts it back; FUNC + YES keeps
open_ $W/y7.snap $W/r0.snap
key $W/r0.snap $W/r0t.snap LEFT                      # TURING, from TAKE (the page stays across closing)
key $W/r0t.snap $W/r1.snap LEFT                      # TOOLS
png $W/r1.snap seedr-tools
key $W/r1.snap $W/r2.snap YES; png $W/r2.snap seedr-tools-rotated
want=$(python3 -c "import sys; v=sys.argv[1].split(); print(' '.join(v[(i-1)%32] for i in range(32))+' ')" "$(steps $W/r1.snap)")
same "$want" "$(steps $W/r2.snap)" "ROTATE ALL +1: BD moved one step"
key $W/r2.snap $W/r3.snap NO
same "$(bytes $W/r1.snap)" "$(bytes $W/r3.snap)" "ROTATE ALL, NO: BD byte-identical again"
key $W/r3.snap $W/r4.snap YES
fkey $W/r4.snap $W/r5.snap YES
same "$want" "$(steps $W/r5.snap)" "ROTATE ALL, FUNC + YES: kept"

# ================= every page shown (SEEDR_SHOW_ALL): screenshots
FW=$FW_ALL MAP=$MAP_ALL
boot $W/a0.snap
open_ $W/a0.snap $W/a1.snap
prev=$W/a1.snap; seen=""
for name in take rhythm place dynamics note lock trig time view tools turing; do
    png $prev all-$name
    seen="$seen $(screen $prev)"
    key $prev $W/a-$name.snap RIGHT
    prev=$W/a-$name.snap
done
uniq=$(echo $seen | tr ' ' '\n' | sort -u | wc -l | tr -d ' ')
same 11 "$uniq" "SHOW_ALL: RIGHT visits eleven different pages"
same "$(screen $W/a1.snap)" "$(screen $prev)" "SHOW_ALL: and wraps to TAKE"
# (Until M1 ended, a layer not built yet was turned on here and YES checked to write nothing and say
# NOT AVAILABLE YET. Every layer is built since F9; a refused YES is checked by 06-norows.)
emu_done
