#!/bin/sh
# Release 0.3's part 5 on version 1: SOURCE = TRACK with PLACE ROT (0.3's ROTATE, whole steps), the
# field rotation (FLD VEL by trig), TOOLS' ROTATE ALL over every track (SD moves too) and NO; whole
# steps carry their sample locks.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 05-rotate
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
SD0=$(emu_dump 1)
emu_dump 1 > "$D/sd0.txt"
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 0
emu_page PLACE
emu_knob PLACE A:rot 3
emu_yes
emu_shot rotate
check_take "SOURCE = TRACK, ROT +3: whole steps" src=track rot=3 "$K"
emu_no
check_take "NO: seed 2" "$K"
emu_knob PLACE A:rot 0
emu_knob PLACE F:fld:one 1
emu_knob PLACE G:frot 3
emu_yes
check_take "FLD VEL, FROT +3, by trig" src=track fld=vel frot=3 "$K"
emu_no
emu_page TOOLS
emu_knob TOOLS STEPS 3
emu_yes
check_take "ROTATE ALL +3: BD" src=track rot=3 "$K"
emu_check "ROTATE ALL +3: SD" "$(seedref src=track rot=3 track=1 orig="$D/sd0.txt")" "$(emu_dump 1)"
emu_no
emu_check "NO: SD back" "$SD0" "$(emu_dump 1)"
check_take "NO: BD back" "$K"
# whole steps carry their locks: 04's kept take with sample locks, +1
emu_fixture smp3 fix_smp3
save_orig smp3
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 0
emu_page LOCK
emu_knob LOCK B:lane1.pct 0
emu_page PLACE
emu_knob PLACE A:rot 1
emu_yes
check_take "ROT +1 carries the sample locks" src=track rot=1 orig="$D/smp3.txt"
emu_done
