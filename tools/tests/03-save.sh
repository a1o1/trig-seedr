#!/bin/sh
# Release 0.3's part 3 on version 1: SAVE / RELOAD PATTERN; power cycle (I9); the project's
# modified mark; power off with a take pending (A7).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 03-save
emu_build
# 1. SAVE PATTERN (YES + SCENE), seed 3 over it, RELOAD PATTERN (NO + SCENE): back to seed 2
emu_fixture kept2 fix_kept2
emu_keys YES+SCNE
emu_open
emu_knob TAKE A:seed 3
emu_yes
emu_keep
emu_no
check_take "seed 3 over a saved pattern" src=random seed=3
emu_keys NO+SCNE
check_take "RELOAD PATTERN: seed 2" src=random seed=2
# 2. power off (the working state goes to the card), cold boot on that card
emu_fixture kept2 fix_kept2
emu_power_off
emu_boot "$EMU_CARD"
check_take "power cycle: seed 2" src=random seed=2
# 3. modified mark: SAVE PROJECT AS slot 2 clears it; a take kept sets it
emu_boot
# shellcheck disable=SC2046
emu_run $(python3 "$T/presses.py" 100 "NO*6" GLOBAL "UP*9" YES "UP*2" DOWN YES "F+UP*16" DOWN YES w:100 YES w:500 \
    2>/dev/null | sed 's/@/@+/g')
emu_keys NO NO NO NO
emu_check "modified mark: 0 after SAVE PROJECT AS" "modified 0" "$(emu_cmp 5)"
emu_open
emu_yes
emu_keep
emu_no
emu_check "modified mark: 1 after a take is kept" "modified 1" "$(emu_cmp 5)"
check_take "the take after the save: seed 1" src=random seed=1
# 4. A7: power off with a take pending. The OS saves its working state with the take as it stands,
# and after the cold boot the take is the track (the snapshot was in RAM): nothing pending, NO only
# closes. (SAVE PATTERN, YES + SCENE, cannot run during a take: SCENE closes seedr, NOT APPLIED.)
save_orig seed1
emu_open
emu_yes
check_take "power off pending: YES takes seed 2" src=random seed=2 orig="$D/seed1.txt"
emu_power_off
emu_boot "$EMU_CARD"
check_take "power off pending: the card has the take (seed 2)" src=random seed=2 orig="$D/seed1.txt"
emu_open
emu_check "power off pending: after the boot nothing is pending" 0 "$(emu_get auditioning)"
emu_no
emu_check "power off pending: NO closes" 0 "$(emu_get open)"
check_take "power off pending: NO leaves the take" src=random seed=2 orig="$D/seed1.txt"
emu_done
