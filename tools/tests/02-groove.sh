#!/bin/sh
# Release 0.3's part 2: GROOVE µtime; NO; a GEN take grooved, NO undoes only the groove; velocity
# from a groove (with grooves-live.h); the regenerate rule.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 02-groove
emu_build
# grooves[1] = MPC 16 57 (Live's Swing MPC 3000 16ths 57): odd steps late
emu_fixture kept2 fix_kept2
emu_open
emu_page GROOVE
emu_knob GROOVE GRV 1
emu_yes
emu_shot groove
check_groove "GROOVE MPC 16 57 on seed 2" "$(genref 2 16 1 100 0 0 0 0 1)"
emu_no
check_groove "NO: seed 2, µtime 0" "$(genref 2 16 0 0 0 0 0 0 1)"
emu_page GEN1
emu_knob GEN1 SEED 1
emu_yes
emu_page GROOVE
emu_yes
check_groove "GEN seed 1, then GROOVE" "$(genref 1 16 1 100 0 0 0 0 1)"
emu_no
check_groove "NO: the GEN take kept" "$(genref 1 16 0 0 0 0 0 0 1)"
if [ -f "$R/mods/seedr-ar2/grooves-live.h" ]; then
    # FUNK MODERN (grooves[44], velocities 40..127) with VEL +48 % on the kept seed-1 take
    emu_knob GROOVE GRV 44
    emu_knob GROOVE GVEL 48
    emu_yes
    emu_shot groove-funk
    check_groove "GROOVE FUNK MODERN, VEL +48" "$(genref 1 16 44 100 48 0 0 0 1)"
else
    echo "SKIP 02-groove: FUNK MODERN (no grooves-live.h)"
fi
# Regenerate: YES with no knob turned since the last YES takes the next seed
emu_fixture kept2 fix_kept2
emu_open
emu_yes
check_gen "reopen, YES: seed 3" "$(genref 3)"
emu_yes
check_gen "YES again: seed 4" "$(genref 4)"
emu_knob GEN1 DENS 58
emu_yes
check_gen "DENS 58 %, YES: seed 4" "$(genref 4 16 0 0 0 0 0 0 1 58)"
emu_done
