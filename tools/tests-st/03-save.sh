#!/bin/sh
# Saving and reloading takes on the Syntakt (plan-syntakt.md S4; the Rytm's 03 and I9). The Syntakt saves a
# pattern only into a project saved on the card ("Prj must be re-saved!" for the preset project), so:
#   1. FUNC + GLOBAL (Save Proj): the project as slot 002; FUNC + YES (Save Ptn) with seed 2 kept
#   2. seed 3 kept over it; FUNC + NO (Reload Ptn): seed 2 back
#   3. seed 3 kept again, the project saved; power off and a cold boot on that card: the working state
#      the OS saved at power off has the kept take (I9). (Until analog-emu-st 5c970d9 the take came back only
#      through LOAD PROJECT; that check went, since its menu presses depend on where the cursor starts.)
#   4. A7: power off with a take pending: the card has the take as it stands; after the cold boot it is
#      the track, nothing is pending, and NO only closes
# The power-off save needs analog-emu-st's model of the Syntakt's supercap signal (PPDSDR_G bit 3,
# analog-emu-st 5c970d9; status/U2.md); before it, the OS never armed the save.
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
emu_check "power off: the OS saved its working state" "8000 sectors written" "$(grep -o '[0-9]* sectors written' "$D/power-off.log")"
emu_boot "$EMU_CARD"
emu_check "power cycle: the cold boot has the kept take (seed 3)" "$(cat "$D/kept3.txt")" "$(emu_dump $EMU_TRACK)"

# 4. A7: power off with a take pending. Recipes are in RAM only (design A3), so after the cold boot track 4 has
#    a new recipe, on SRC = TRACK (it has trigs, V6), whose take would be the track itself: RAND makes one that
#    differs. The card is copied first: emu_power_off copies EMU_CARD to power-off.img, which it already is.
cp "$EMU_CARD" "$D/card-cold.img"; EMU_CARD=$D/card-cold.img
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 1
emu_yes
emu_check "power off pending: a take is pending" 1 "$(emu_get auditioning)"
emu_check "power off pending: the take differs from the kept track" 1 "$([ "$(emu_dump $EMU_TRACK)" != "$(cat "$D/kept3.txt")" ] && echo 1)"
emu_dump $EMU_TRACK > "$D/pending4.txt"
emu_power_off
emu_boot "$EMU_CARD"
emu_check "power off pending: the card has the take as it stood (seed 4)" "$(cat "$D/pending4.txt")" "$(emu_dump $EMU_TRACK)"
emu_open
emu_check "power off pending: after the boot nothing is pending" 0 "$(emu_get auditioning)"
emu_no
emu_check "power off pending: NO closes" 0 "$(emu_get open)"
emu_check "power off pending: NO leaves the take" "$(cat "$D/pending4.txt")" "$(emu_dump $EMU_TRACK)"
emu_done
