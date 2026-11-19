#!/bin/sh
# The Rytm's 13 on the Syntakt (plan-syntakt.md S4): YES generates only with grid recording on (RECORD TO
# GENERATE otherwise, nothing written); FUNC + YES is the only way a take stays (APPLIED); YES after NO brings
# the same take back; turning grid recording off, closing seedr or another pattern with a take pending puts the
# original back (NOT APPLIED). On track 4 from the kept2 fixture (seed 2 kept).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st13-apply
st_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
pending() { emu_get auditioning; }

# 1. grid recording off: YES writes nothing
emu_rec 0
emu_open
emu_keys YES
emu_check "RECORD off: YES says RECORD TO GENERATE" "RECORD TO GENERATE" "$(emu_msg)"
check_take "RECORD off: nothing written" "$K"
emu_check "RECORD off: no take pending" 0 "$(pending)"

# 2. grid recording on: YES after applying takes the next seed (3)
emu_rec 1
emu_keys YES
check_take "RECORD on: YES writes seed 3" src=random seed=3 "$K"
emu_check "RECORD on: a take is pending" 1 "$(pending)"
emu_no
check_take "NO: the original is back" "$K"

# 3. YES after NO: the same take; FUNC + YES applies; it stays after closing
emu_yes
check_take "YES after NO: the same take (seed 3)" src=random seed=3 "$K"
emu_keep
emu_check "FUNC + YES: APPLIED" "APPLIED" "$(emu_msg)"
emu_keys TRIG
emu_check "applied, then closed with TRIG: no NOT APPLIED" "APPLIED 0" "$(emu_msg) $(emu_get open)"
emu_open
check_take "applied: the take stays after closing" src=random seed=3 "$K"
save_orig kept3
K3="orig=$D/kept3.txt"

# 4. YES after applying: the next seed; grid recording off with a take pending: not applied
emu_yes
check_take "YES after applying: the next seed (4)" src=random seed=4 "$K3"
emu_rec 0
emu_check "RECORD off: NOT APPLIED" "NOT APPLIED" "$(emu_msg)"
check_take "RECORD off: the applied take (seed 3) is back" "$K3"

# 5. YES after NOT APPLIED: the take that was heard; YES while it plays: the next; closing: not applied
emu_yes
check_take "YES after NOT APPLIED: the same take (seed 4)" src=random seed=4 "$K3"
emu_keys YES
check_take "YES while the take plays: the next seed (5)" src=random seed=5 "$K3"
emu_keys TRIG
emu_check "closed with TRIG: NOT APPLIED, seedr closed" "NOT APPLIED 0" "$(emu_msg) $(emu_get open)"
check_take "closed: the applied take is back" "$K3"

# 6. another pattern with a take pending: not applied (A01 gets its original back, A02 is untouched)
A02=$(st_peek dump "$EMU_SNAP" $EMU_TRACK 0 1 --seed)
emu_open
emu_yes
check_take "YES after closing: the same take (seed 5)" src=random seed=5 "$K3"
emu_keys PTN+T2 w:300
emu_check "pattern A02: NOT APPLIED" "NOT APPLIED" "$(emu_msg)"
emu_check "pattern A02: A01's track 4 is the applied take again" "$(seedref track=$EMU_TRACK "$K3")" \
    "$(st_peek dump "$EMU_SNAP" $EMU_TRACK 0 0 --seed)"
emu_check "pattern A02: A02's track 4 untouched" "$A02" "$(st_peek dump "$EMU_SNAP" $EMU_TRACK 0 1 --seed)"
emu_done
