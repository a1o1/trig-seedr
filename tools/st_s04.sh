#!/bin/sh
# Task S0.4 (docs/plan-syntakt.md, docs/firmware-notes-st142.md): edits made with the stock Syntakt OS 1.42's
# keys show in tools/st_peek.py's reading of the snapshot.
#   1. boot: track 1 of the default pattern A01, length 64, trigs on steps 1 and 4 of each 16
#   2. RECORD: grid recording reads 1
#   3. T5 in grid recording: a new trig on step 5, its fields reset (NOTE, VEL, LEN default; micro timing 0;
#      no condition; retrig 14 / 12 / 0)
#   4. T1 held on the TRIG page, knob A (NOTE) and B (VEL) turned: step 1's NOTE and VEL change, others not
#   5. T1 held on the SYN page, knob A turned: a lock of sound id 18 on step 1
#   6. a new boot; TRIG, TRIG: the TRIG view's sub-page 0, then 1; knob D on Euclidean: EUC on
# Writes only under build/emu/st-s04/.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu_st.sh"
emu_init st-s04
peek() { python3 "$T/st_peek.py" "$@"; }
row() { peek dump "$EMU_SNAP" 0 "${2:-16}" ${3:+--lane $3} | sed -n "s/^$1 //p"; }

# 1. boot
emu_boot
emu_check "boot: track 1's length" 64 "$(row length 0)"
emu_check "boot: track 1's trigs (steps 1..16)" "x . . x . . . . . . . . . . . ." "$(row trig)"
emu_check "boot: grid recording off" 0 "$(peek rec "$EMU_SNAP")"
emu_check "boot: the TRIG page is not shown" -1 "$(peek trigpage "$EMU_SNAP")"

# 2. RECORD
emu_keys RECORD
emu_check "RECORD: grid recording on" 1 "$(peek rec "$EMU_SNAP")"

# 3. a new trig on step 5
emu_keys T5
emu_check "T5: a trig on step 5" "x . . x x . . . . . . . . . . ." "$(row trig)"
emu_check "T5: step 5's NOTE VEL LEN utime cond rlen rrate rvel" "- - - 0 - 14 12 0" \
    "$(for r in note vel len utime cond rlen rrate rvel; do row $r | cut -d' ' -f5; done | tr '\n' ' ' | sed 's/ $//')"

# 4. NOTE and VEL on a held trig (TRIG page: A NOTE, B VEL)
emu_keys TRIG
emu_run -s 400M --press T1@+100M:1500M --turn 0:4@+400Mx6/40M --turn 1:-4@+900Mx6/40M
N1=$(row note | cut -d' ' -f1) V1=$(row vel | cut -d' ' -f1)
emu_check "T1 held, A and B turned: step 1 has its own NOTE and VEL" 1 \
    "$([ "$N1" != - ] && [ "$V1" != - ] && echo 1)"
emu_check "T1 held: step 5's NOTE and VEL still the default" "- -" "$(row note | cut -d' ' -f5) $(row vel | cut -d' ' -f5)"
echo "  step 1: NOTE $N1, VEL $V1"

# 5. a lock: SYN page, T1 held, knob A
emu_keys SYN
emu_run -s 400M --press T1@+100M:1500M --turn 0:4@+400Mx6/40M --turn 0:4@+900Mx6/40M
emu_check "T1 held on SYN, A turned: a lock of id 18 on step 1 only" 1 \
    "$(row lane1 4 18 | awk '$1 != "-" && $2 == "-" && $3 == "-" && $4 == "-" { print 1 }')"

# 6. the TRIG sub-pages and EUC, from a new boot (after the held-trig edits above, knob D does not turn EUC on;
# not looked into)
emu_boot
emu_keys TRIG
emu_check "TRIG: sub-page 0" 0 "$(peek trigpage "$EMU_SNAP")"
emu_keys TRIG
emu_check "TRIG again: sub-page 1 (Euclidean)" 1 "$(peek trigpage "$EMU_SNAP")"
emu_run -s 400M --turn 3:8@+600M --turn 3:4@+610Mx3/10M
emu_check "knob D on Euclidean: EUC on" "1 0" "$(peek euc "$EMU_SNAP")"
emu_done
