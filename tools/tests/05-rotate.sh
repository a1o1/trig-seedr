#!/bin/sh
# Release 0.3's part 5: ROTATE whole steps, one field, every track (SD moves too), NO; whole steps
# carry sample locks.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 05-rotate
emu_build
emu_fixture kept2 fix_kept2
SD0=$(emu_cmp 1 16 1)
emu_open
emu_page ROTATE
emu_knob ROTATE STEPS 3
emu_yes
emu_shot rotate
check_gen "ROTATE +3, whole steps" "$(genref 2 16 0 0 0 0 0 0 1 50 0 1 8 3)"
emu_no
check_gen "NO: seed 2" "$(genref 2)"
emu_knob ROTATE WHAT 1
emu_yes
check_gen "ROTATE +3 VEL (by trig)" "$(GENREF_ROT='3 1 0' genref 2)"
emu_no
emu_knob ROTATE ALL 1
emu_knob ROTATE WHAT 0
emu_yes
check_gen "ROTATE ALL +3: BD" "$(genref 2 16 0 0 0 0 0 0 1 50 0 1 8 3)"
emu_check "ROTATE ALL +3: SD moved" \
    "$(python3 -c "import sys; v=sys.argv[1].split(); print(' '.join(v[(i-3)%16] for i in range(16))+' ')" "$SD0")" \
    "$(emu_cmp 1 16 1)"
emu_no
emu_check "NO: SD back" "$SD0" "$(emu_cmp 1 16 1)"
check_gen "NO: BD back" "$(genref 2)"
# whole steps carry their locks: 04's kept take with sample locks, +1
emu_fixture smp3 fix_smp3
emu_open
emu_page ROTATE
emu_yes
check_smp "ROTATE +1 carries the sample locks" "$(genref 3 16 0 0 0 0 0 0 1 50 100 1 8 1)"
emu_done
