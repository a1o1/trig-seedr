#!/bin/sh
# Generate and apply (Alan, 2026-11-16; docs/flashing.md, the manual's Concepts and Keys pages): YES generates
# only with grid recording on (RECORD TO GENERATE otherwise, nothing written); FUNC + YES is the only
# way a take stays (APPLIED); leaving the track, the pattern or seedr, or turning grid recording off,
# with a take pending puts the original back (NOT APPLIED). On A01 BD from the kept2 fixture.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 13-apply
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
pending() { emu_get auditioning; }

# 1. grid recording off: seedr browses, YES writes nothing
emu_rec 0
emu_open
emu_keys YES
emu_check "RECORD off: YES says RECORD TO GENERATE" "RECORD TO GENERATE" "$(emu_msg)"
check_take "RECORD off: nothing written" "$K"
emu_check "RECORD off: no take pending" 0 "$(pending)"

# 2. grid recording on: YES generates (no knob turned since the fixture's last YES: seed 3)
emu_rec 1
emu_keys YES
check_take "RECORD on: YES writes seed 3" src=random seed=3 "$K"
emu_check "RECORD on: a take is pending" 1 "$(pending)"

# 3. (Another track with a take pending is not applied either: the same condition as another pattern,
# section 8. It is not driven here: with grid recording on, TRK + a TRIG key edits that step in
# analog-emu, and dtboot cannot press the pads. Checked on the hardware, docs/flashing.md.)
emu_no
check_take "NO: BD's original is back" "$K"

# 4. YES after NO brings the same take back (M2.5); FUNC + YES applies it; it stays after seedr
# closes and opens again
emu_yes
check_take "YES after NO: the same take (seed 3)" src=random seed=3 "$K"
emu_keep
emu_check "FUNC + YES: APPLIED" "APPLIED" "$(emu_msg)"
emu_keys TRIG
emu_check "applied, then closed: no NOT APPLIED" "APPLIED 0" "$(emu_msg) $(emu_get open)"
emu_open
check_take "applied: the take stays after closing" src=random seed=3 "$K"
save_orig kept4
K4="orig=$D/kept4.txt"

# 5. YES after applying: the next seed; grid recording off with a take pending: not applied
emu_yes
check_take "YES after applying: the next seed (4)" src=random seed=4 "$K4"
emu_rec 0
emu_check "RECORD off: NOT APPLIED" "NOT APPLIED" "$(emu_msg)"
check_take "RECORD off: the applied take (seed 3) is back" "$K4"

# 6. YES after NOT APPLIED brings the take that was heard back; YES while it plays moves on;
# closing seedr with a take pending: not applied
emu_yes
check_take "YES after NOT APPLIED: the same take (seed 4)" src=random seed=4 "$K4"
emu_keys YES
check_take "YES while the take plays: the next seed (5)" src=random seed=5 "$K4"
emu_keys TRIG
emu_check "closed with TRIG: NOT APPLIED, seedr closed" "NOT APPLIED 0" "$(emu_msg) $(emu_get open)"
check_take "closed: the applied take is back" "$K4"

# 7. TOOLS' ROTATE ALL needs grid recording too
emu_open
emu_rec 0
emu_page TOOLS
emu_keys YES
emu_check "TOOLS, RECORD off: RECORD TO GENERATE" "RECORD TO GENERATE" "$(emu_msg)"
check_take "TOOLS, RECORD off: nothing rotated" "$K4"

# 8. another pattern with a take pending: not applied (A01 gets its original back, A02 is untouched)
A02=$(emu_py dump "$EMU_SNAP" 0 0 1)
emu_page TAKE
emu_yes
check_take "YES after closing: the same take (seed 5)" src=random seed=5 "$K4"
emu_keys BANK_A+T2
emu_check "pattern A02: NOT APPLIED" "NOT APPLIED" "$(emu_msg)"
emu_check "pattern A02: A01's BD is the applied take again" "$(seedref "$K4")" "$(emu_py dump "$EMU_SNAP" 0 0 0)"
emu_check "pattern A02: A02's BD untouched" "$A02" "$(emu_py dump "$EMU_SNAP" 0 0 1)"
emu_done
