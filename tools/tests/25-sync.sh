#!/bin/sh
# Task F5: SYNC on the device.
#   1. a RANDOM take (kept2's seed 2) with SYNC 100, SYNC with SKIP 3, each against tools/seedref; NO
#   2. SOURCE = TRACK through origins: a step with three parameter locks made by hand (GRID REC, T5
#      held, three FLTR knobs) and a plain trig on step 10; SYNC 100 anticipates step 5 onto step 4,
#      all 42 lock values with it; NO restores record and lock store; kept, SAVE and RELOAD PATTERN
#   3. lock rows (plan section 10, I8): a test build where no new row fits (RT_ROWS_MAX 0). SYNC
#      moving the locked step is refused with NO LOCK ROWS LEFT, nothing written, no audition; so is
#      SYNC with LOCK lane 1. (The device layer counts each moved lock as a new row: its sound index
#      to row type table does not find the rows stock made for these locks; docs/status/F5.md.)
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 25-sync

LK=$((0x407dd8e3 + 0x2e91))          # A01 BD's lock store (tools/emu.py LOCKS): 88 bytes a step
# locks STEP: the step's 42 parameter-lock words (sound indexes 0..41; -1 none), 0-based step
locks() { emu_py peek "$EMU_SNAP" "$(printf '0x%x' $((LK + 88 * $1)))" 2 42 -s; }
nlocks() { locks "$1" | tr ' ' '\n' | grep -c -v '^-'; }
# store: BD's step record and lock store, as a checksum
store() { { emu_py peek "$EMU_SNAP" 0x407dd8e3 1 917; emu_py peek "$EMU_SNAP" $LK 1 5674; } | cksum; }
# msg: the mod's message line (seedr.c msg), or nothing
msg() {
    _msg_p=$(emu_get msg)
    [ "$_msg_p" = 0 ] && return 0
    emu_py peek "$EMU_SNAP" "$(printf '0x%x' $((_msg_p & 0xffffffff)))" 1 24 | python3 -c \
        "import sys; b=bytes(int(x) for x in sys.stdin.read().split()); print(b.split(b'\0')[0].decode())"
}
# by_hand: a fresh project; GRID REC, trigs on steps 5 and 10, T5 held: FLTR DEC, FRQ, RES turned
by_hand() {
    emu_boot
    emu_run --press RECORD@+200M --press T5@+400M --press T10@+550M --press FLTR@+700M --press T5@+1000M:1400M \
        --nudge 1:1@+1200Mx6/40M --nudge 4:1@+1600Mx6/40M --nudge 5:1@+2000Mx6/40M --press RECORD@+2700M
}

# ---- 1. a RANDOM take ----
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_page PLACE
emu_knob PLACE B:sync 100
emu_yes
emu_shot place
check_take "RANDOM seed 2, SYNC 100" src=random seed=2 sync=100 "$K"
emu_knob PLACE E:skip 3
emu_yes
check_take "SYNC 100, SKIP 3" src=random seed=2 sync=100 skip=3 "$K"
emu_no
check_take "NO: seed 2" src=random seed=2 "$K"

# ---- 2. SOURCE = TRACK: whole steps with their locks ----
by_hand
save_orig hand
H="orig=$D/hand.txt"
L5=$(locks 4)
S0=$(store)
emu_check "by hand: trigs on 5 and 10, three locks on 5, none on 4" "x 3 0" \
    "$(sed -n 's/^trig //p' "$D/hand.txt" | cut -d' ' -f5) $(nlocks 4) $(nlocks 3)"
emu_open
emu_check "open: a track with trigs gets SOURCE = TRACK (V6)" 0 "$(emu_get 'recipe[0].src')"
emu_page PLACE
emu_knob PLACE B:sync 100
emu_yes
emu_shot track
check_take "SOURCE = TRACK, SYNC 100: step 5 onto step 4" src=track sync=100 "$H"
emu_check "SYNC 100: step 4 has step 5's 42 lock values, step 5 none" "$L5 0" "$(locks 3) $(nlocks 4)"
emu_no
emu_check "NO: record and lock store as before (I7)" "$S0" "$(store)"
emu_yes
emu_keep
emu_no
emu_keys YES+SCNE w:300 NO+SCNE
check_take "kept, SAVE + RELOAD PATTERN: the move" src=track sync=100 "$H"
emu_check "SAVE + RELOAD PATTERN: the locks on step 4" "$L5 0" "$(locks 3) $(nlocks 4)"

# ---- 3. no lock rows left ----
EMU_VERSION=GR0T emu_build norows -DRT_ROWS_MAX=0
by_hand
emu_check "norows build, by hand: the same three locks" "$L5" "$(locks 4)"
S0=$(store)
emu_open
emu_page PLACE
emu_knob PLACE B:sync 100
emu_yes
emu_shot norows
emu_check "no rows: SYNC moving the locks is refused" "NO LOCK ROWS LEFT" "$(msg)"
emu_check "refused: record and lock store unchanged (I8)" "$S0" "$(store)"
emu_check "refused: no audition started" 0 "$(emu_get auditioning)"
emu_page LOCK
emu_knob LOCK B:lane1.pct 100
emu_yes
emu_check "no rows: SYNC with LOCK lane 1 refused" "NO LOCK ROWS LEFT" "$(msg)"
emu_check "refused: record and lock store unchanged (I8)" "$S0" "$(store)"
emu_done
