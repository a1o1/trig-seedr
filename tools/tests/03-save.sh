#!/bin/sh
# Release 0.3's part 3: SAVE / RELOAD PATTERN; power cycle; the project's modified mark.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 03-save
emu_build
# 1. SAVE PATTERN (YES + SCENE), seed 3 over it, RELOAD PATTERN (NO + SCENE): back to seed 2
emu_fixture kept2 fix_kept2
emu_keys YES+SCNE
emu_open
emu_knob GEN1 SEED 3
emu_yes
emu_keep
emu_no
check_gen "seed 3 over a saved pattern" "$(genref 3)"
emu_keys NO+SCNE
check_gen "RELOAD PATTERN: seed 2" "$(genref 2)"
# 2. power off (the working state goes to the card), cold boot on that card
emu_fixture kept2 fix_kept2
emu_power_off
emu_boot "$EMU_CARD"
check_gen "power cycle: seed 2" "$(genref 2)"
# 3. modified mark: SAVE PROJECT AS slot 2 clears it; generate + keep sets it
emu_boot
# shellcheck disable=SC2046
emu_run $(python3 "$T/presses.py" 100 "NO*6" GLOBAL "UP*9" YES "UP*2" DOWN YES "F+UP*16" DOWN YES w:100 YES w:500 \
    2>/dev/null | sed 's/@/@+/g')
emu_keys NO NO NO NO
emu_check "modified mark: 0 after SAVE PROJECT AS" "modified 0" "$(emu_cmp 5)"
emu_open
emu_page GEN1
emu_yes
emu_keep
emu_no
emu_check "modified mark: 1 after generate + keep" "modified 1" "$(emu_cmp 5)"
check_gen "generate after the save" "$(genref 1)"
emu_done
