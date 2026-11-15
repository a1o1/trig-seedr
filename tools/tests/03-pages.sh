#!/bin/sh
# Task 0.3, the page framework, in analog-emu: PAGE and the TRIG keys left to stock while seedr is
# open, LEFT / RIGHT over the pages that are built, YES / NO / FUNC + YES on a recipe (SOURCE =
# TRACK with every layer at 0 leaves the track byte-identical), the TRIG cycle, TOOLS' ROTATE ALL,
# VIEW's knob cursor, and a screenshot of every page (a SEEDR_SHOW_ALL build shows the pages whose
# features are not built yet, as their knob tables stand).
# Everything it writes (the two builds' .syx, snapshots, the card, PNGs) goes under build/pages/ of
# this checkout, so it does not touch analog-emu/ or another worktree's files. It uses core-ar2's
# built core-0.13.elemod and does not rebuild it (tools/test_seedr.sh PART=1 builds it).
# Needs WORKSPACE (the folder holding elekloader/ and analog-emu/; set it in a worktree), B (dtboot,
# dtpeek, dtcard), the stock OS (STOCK) and ELEKLOADER_CROSS. Run: sh tools/tests/03-pages.sh
set -e
R=$(cd "$(dirname "$0")/../.." && pwd)
E=${WORKSPACE:-$(cd "$R/.." && pwd)}
T=$R/tools; W=$R/build/pages; mkdir -p $W
STOCK=${STOCK:-"$E/Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx"}
B=${B:-$CARGO_TARGET_DIR/release}
CORE=$E/elekloader/mods/core-ar2/out/core-0.13.elemod
if [ ! -f "$CORE" ]; then echo "FAIL no $CORE: build it first (tools/test_seedr.sh PART=1)"; exit 1; fi

# ---- two builds: the mod as it is, and every page shown (its own folder under build/, sources from here)
M=$W/showall; mkdir -p $M
python3 - "$R/mods/seedr-ar2" "$M" <<'PY'
import json, os, sys
src, dst = sys.argv[1], sys.argv[2]
j = json.load(open(os.path.join(src, 'mod.json')))
j['sources'] = [os.path.normpath(os.path.join(src, s)) for s in j['sources']]
j['cflags'] = ['-DSEEDR_SHOW_ALL', '-I' + src]
json.dump(j, open(os.path.join(dst, 'mod.json'), 'w'), indent=1)
PY
cd $E/elekloader
for m in "$R/mods/seedr-ar2 seedr" "$M showall"; do
    set -- $m
    python3 -m elekloader.sdk.build $1 --stock "$STOCK" | tail -1
    python3 -m elekloader.patch --stock "$STOCK" --mod $CORE --mod $1/out/seedr-0.3.elemod \
        --out $W/$2.syx --lockfile $W/$2.lock.json --version GR01 | tail -1
done
cd $W

# ---- helpers. Each snapshot's clock (in M instructions) is kept beside it in SNAP.clk; `from SNAP`
# sets CLK to it, and `at N` is the clock N M later, for a run that starts from SNAP.
FW=$W/seedr.syx
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
same() { if [ "$1" = "$2" ]; then echo "PASS $3"; else echo "FAIL $3"; echo "  want $1"; echo "  got  $2"; fi; }
differ() { if [ "$1" != "$2" ]; then echo "PASS $3"; else echo "FAIL $3 (both $1)"; fi; }
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
differ "$(screen $W/k1.snap)" "$(screen $W/k2.snap)" "seedr's screen stays up and its strip follows the track"

# 2. LEFT / RIGHT: TAKE, VIEW, TOOLS are the pages built; RIGHT three times comes back
key $W/k2.snap $W/p1.snap RIGHT; png $W/p1.snap seedr-view
key $W/p1.snap $W/p2.snap RIGHT
key $W/p2.snap $W/p3.snap RIGHT
key $W/p3.snap $W/p4.snap LEFT
s0=$(screen $W/k2.snap); s1=$(screen $W/p1.snap); s2=$(screen $W/p2.snap)
if [ "$s0" != "$s1" ] && [ "$s1" != "$s2" ] && [ "$s0" != "$s2" ]; then echo "PASS RIGHT: three different pages"; else echo "FAIL RIGHT: pages repeat"; fi
same "$s0" "$(screen $W/p3.snap)" "RIGHT x3: back on TAKE (only the built pages are visited)"
same "$s2" "$(screen $W/p4.snap)" "LEFT from TAKE: TOOLS"

# 3. VIEW: knob A moves the step cursor
key $W/p4.snap $W/v0.snap LEFT
from $W/v0.snap; run $W/v0.snap $W/v1.snap 600 --nudge 0:1@$(at 100) --nudge 0:1@$(at 300)
png $W/v1.snap seedr-view-cursor
differ "$(screen $W/v0.snap)" "$(screen $W/v1.snap)" "VIEW: knob A moves the cursor"
same "$(bytes $W/v0.snap)" "$(bytes $W/v1.snap)" "VIEW: the track is not touched"

# 4. YES with SOURCE = TRACK and every layer at 0: byte-identical (I1 through the device), an audition;
#    NO; YES again (the next seed); FUNC + YES keeps; NO closes
key $W/v1.snap $W/y0.snap RIGHT                      # TOOLS
key $W/y0.snap $W/y1.snap RIGHT                      # TAKE
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
key $W/r0.snap $W/r1.snap LEFT                       # TOOLS, from TAKE (the page stays across closing)
png $W/r1.snap seedr-tools
key $W/r1.snap $W/r2.snap YES; png $W/r2.snap seedr-tools-rotated
want=$(python3 -c "import sys; v=sys.argv[1].split(); print(' '.join(v[(i-1)%32] for i in range(32))+' ')" "$(steps $W/r1.snap)")
same "$want" "$(steps $W/r2.snap)" "ROTATE ALL +1: BD moved one step"
key $W/r2.snap $W/r3.snap NO
same "$(bytes $W/r1.snap)" "$(bytes $W/r3.snap)" "ROTATE ALL, NO: BD byte-identical again"
key $W/r3.snap $W/r4.snap YES
fkey $W/r4.snap $W/r5.snap YES
same "$want" "$(steps $W/r5.snap)" "ROTATE ALL, FUNC + YES: kept"

# ================= every page shown (SEEDR_SHOW_ALL): screenshots, and a layer that is not built
FW=$W/showall.syx
boot $W/a0.snap
open_ $W/a0.snap $W/a1.snap
prev=$W/a1.snap; seen=""
for name in take rhythm place dynamics note lock trig time view tools; do
    png $prev all-$name
    seen="$seen $(screen $prev)"
    key $prev $W/a-$name.snap RIGHT
    prev=$W/a-$name.snap
done
uniq=$(echo $seen | tr ' ' '\n' | sort -u | wc -l | tr -d ' ')
same 10 "$uniq" "SHOW_ALL: RIGHT visits ten different pages"
same "$(screen $W/a1.snap)" "$(screen $prev)" "SHOW_ALL: and wraps to TAKE"
# RHYTHM (a-take.snap) on the empty BD: SRC = RANDOM (V6); its stage is a stub, so YES writes nothing and says so
key $W/a-take.snap $W/n1.snap YES; png $W/n1.snap all-rhythm-not-built
same "$(bytes $W/a-take.snap)" "$(bytes $W/n1.snap)" "SHOW_ALL: YES on a layer not built writes nothing"
differ "$(screen $W/a-take.snap)" "$(screen $W/n1.snap)" "SHOW_ALL: and says so (NOT AVAILABLE YET)"
run $W/n1.snap $W/n2.snap 700
same "$(screen $W/a-take.snap)" "$(screen $W/n2.snap)" "SHOW_ALL: no audition was started (the screen as before YES)"
