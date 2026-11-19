#!/bin/sh
# Saving and reloading takes on the Syntakt (plan-syntakt.md S4; the Rytm's 03 and I9). The Syntakt saves a
# pattern only into a project saved on the card ("Prj must be re-saved!" for the preset project), so:
#   1. FUNC + GLOBAL (Save Proj): the project as slot 002; FUNC + YES (Save Ptn) with seed 2 kept
#   2. seed 3 kept over it; FUNC + NO (Reload Ptn): seed 2 back
#   3. seed 3 kept again, the project saved; power off and a cold boot on that card; GLOBAL, PROJECT, LOAD
#      PROJECT 002: the kept take is back
# analog-emu's power off does not save the Syntakt's working state (status/S0.2.md: its handler writes 0
# sectors), so a cold boot starts on the preset project and the take comes back through the project load.
# On track 4 from the kept2 fixture.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st03-save
st_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"

# save_project: FUNC + GLOBAL, the slot after the one shown, YES (SAVE PROJECT AS), YES (the name)
save_project() { emu_rec 0; emu_keys F+GLOBAL w:300 DOWN w:200 YES w:500 YES w:2500; }

# 1. the project, then the pattern
save_project
emu_keys F+YES w:500
emu_check "Save Ptn: the pattern as it is (seed 2 kept)" "$(cat "$D/kept2.txt")" "$(emu_dump $EMU_TRACK)"

# 2. seed 3 kept over it; Reload Ptn
emu_open
emu_yes
emu_keep
emu_no
check_take "seed 3 kept over the saved pattern" src=random seed=3 "$K"
emu_rec 0
emu_keys F+NO w:500
emu_check "Reload Ptn: seed 2 back" "$(cat "$D/kept2.txt")" "$(emu_dump $EMU_TRACK)"

# 3. seed 3 kept again (SEED turned away and back: YES renders seed 3 from the reloaded original), the
#    project saved, power off, cold boot, LOAD PROJECT 002
emu_open
emu_knob TAKE A:seed 4
emu_knob TAKE A:seed 3
emu_yes
emu_keep
emu_no
check_take "seed 3 kept again" src=random seed=3 "$K"
emu_dump $EMU_TRACK > "$D/kept3.txt"
save_project
emu_power_off
emu_boot "$EMU_CARD"
COLD=$(emu_dump $EMU_TRACK)
emu_keys GLOBAL w:300 YES w:300 YES w:300 DOWN w:200 YES w:1500 NO NO NO w:500
emu_shot loaded
emu_check "LOAD PROJECT 002 after a power cycle: the kept take (seed 3)" "$(cat "$D/kept3.txt")" "$(emu_dump $EMU_TRACK)"
emu_check "the cold boot itself showed the preset's track 4, not the take" 1 "$([ "$COLD" != "$(cat "$D/kept3.txt")" ] && echo 1)"
emu_done
