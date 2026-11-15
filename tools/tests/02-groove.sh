#!/bin/sh
# Release 0.3's part 2 on version 1: TIME GRV on a RANDOM take; NO; with SOURCE = TRACK the groove
# on the kept take's own trigs (0.3's GROOVE page); NO; GVEL with grooves-live.h; the regenerate
# rule (YES with no knob turned takes the next seed).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 02-groove
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_page TIME
emu_knob TIME A:grv 1
emu_yes
emu_shot groove
check_take "GRV 1 on seed 2" src=random seed=2 grv=1 "$K"
emu_no
check_take "NO: seed 2, micro timing 0" src=random seed=2 "$K"
emu_page RHYTHM
emu_knob RHYTHM A:src:one 0
emu_page TIME
emu_yes
check_take "SOURCE = TRACK, GRV 1: the kept take grooved" src=track grv=1 "$K"
emu_no
check_take "NO: the kept take as it was" "$K"
if [ -f "$R/mods/seedr-ar2/grooves-live.h" ]; then
    # FUNK MODERN (grooves[44] in grooves-live.h, velocities 40..127) with GVEL +48 %
    emu_knob TIME A:grv 44
    emu_knob TIME C:gvel 48
    emu_yes
    emu_shot groove-funk
    check_take "SOURCE = TRACK, GRV 44, GVEL +48" src=track grv=44 gvel=48 "$K"
else
    echo "SKIP 02-groove: GVEL (no grooves-live.h)"
fi
# Regenerate: YES with no knob turned since the last YES takes the next seed
emu_fixture kept2 fix_kept2
emu_open
emu_yes
check_take "reopen, YES: seed 3" src=random seed=3 "$K"
emu_yes
check_take "YES again: seed 4" src=random seed=4 "$K"
emu_page RHYTHM
emu_knob RHYTHM C:dens 58
emu_yes
check_take "DENS 58 %, YES: seed 4" src=random seed=4 dens=58 "$K"
emu_done
