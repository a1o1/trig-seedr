#!/bin/sh
# Task F6: value lists (NSRC / SRC = LIST with ORDR and NLEN / LLEN) and LMOD = LEGATO on the device.
#   1. NOTE: from kept2 (RANDOM seed 2), NOTE -5..+7 as a LIST: UP of 4, then PING of 5, then LMOD =
#      LEGATO; each take against tools/seedref
#   2. LOCK: lane 1 (SMP) at 100 % as a LIST, ODD of 3; lane 2 (DOWN; FRQ) at 60 %, RAND of 6, read
#      from the lock store (the dump shows lane 1 only)
#   3. NO: the original; YES with no knob turned is seed 3 (V2); kept, closed, SAVE and RELOAD
#      PATTERN: the take
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 26-lists

LK=$((0x407dd8e3 + 0x2e91))          # A01 BD's lock store (tools/emu.py LOCKS): 88 bytes a step
FRQ=30                               # lane 2's default parameter, FRQ: sound index 30 (device/rytm_params.h)
# lane2: BD's FRQ locks as seedref's lane2 row (16 steps)
lane2() {
    emu_py peek "$EMU_SNAP" $LK 2 $((44 * 16)) -s | python3 -c \
        "import sys; w=[int(x) for x in sys.stdin.read().split()]; print('lane2 ' + ' '.join('-' if v < 0 else str(v >> 8) for v in w[$FRQ::44]))"
}
# check_lane2 LABEL [SEEDREF-ARG...]: BD's lane 2 is seedref's for those settings
check_lane2() {
    _cl_l=$1; shift
    emu_check "$_cl_l" "$(seedref all=1 "$@" | grep '^lane2')" "$(lane2)"
}

# ---- 1. NOTE ----
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_page NOTE
emu_run -s 300M --nudge 2:1@+600M                     # NOTE leaves DEF: 0..0
emu_knob NOTE C:note.hi 7
emu_knob NOTE C:note.lo -5
emu_knob NOTE D:note.src:one 1
emu_yes
emu_shot note
N="note=-5:7 note.src=list"
check_take "NOTE -5..+7, LIST UP of 4" src=random seed=2 $N "$K"
emu_knob NOTE E:note.order:one 2
emu_knob NOTE F:note.len 5
emu_yes
N="$N note.order=ping note.len=5"
check_take "LIST PING of 5" src=random seed=2 $N "$K"
emu_knob NOTE B:lmod:one 1
emu_yes
emu_shot legato
N="$N lmod=legato"
check_take "LMOD = LEGATO: each trig reaches the next" src=random seed=2 $N "$K"

# ---- 2. LOCK, both lanes ----
emu_page LOCK
emu_knob LOCK B:lane1.pct 100
emu_knob LOCK D:lane1.src:one 1
emu_knob LOCK E:lane1.order:one 4
emu_knob LOCK F:lane1.len 3
L="lane1.pct=100 lane1.src=list lane1.order=odd lane1.len=3"
emu_yes
check_take "LOCK lane 1 SMP 100 %, LIST ODD of 3" src=random seed=2 $N $L "$K"
emu_keys DOWN
emu_knob LOCK B:lane2.pct 60
emu_knob LOCK D:lane2.src:one 1
emu_knob LOCK E:lane2.order:one 5
emu_knob LOCK F:lane2.len 6
L="$L lane2.pct=60 lane2.src=list lane2.order=rand lane2.len=6"
emu_yes
emu_shot lock
check_take "lane 2 at 60 %: lane 1 and the fields unchanged" src=random seed=2 $N $L "$K"
check_lane2 "lane 2 FRQ 60 %, LIST RAND of 6, in the lock store" src=random seed=2 $N $L "$K"

# ---- 3. NO, keep, save ----
emu_no
check_take "NO: the original" src=random seed=2 "$K"
check_lane2 "NO: no FRQ locks" src=random seed=2 "$K"
emu_yes
check_take "YES after NO: the same take (seed 2) back, lists and all" src=random seed=2 $N $L "$K"
emu_keys YES
check_take "YES again, no knob turned: the next seed (V2), lists and all" src=random seed=3 $N $L "$K"
emu_keep
emu_no
emu_check "NO: closed" 0 "$(emu_get open)"
emu_keys YES+SCNE w:300 NO+SCNE
check_take "kept, SAVE + RELOAD PATTERN: the take" src=random seed=3 $N $L "$K"
check_lane2 "SAVE + RELOAD PATTERN: lane 2" src=random seed=3 $N $L "$K"
emu_done
