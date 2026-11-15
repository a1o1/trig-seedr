#!/bin/sh
# Release 0.3's part 6: a test build where no new lock row fits (RT_ROWS_MAX 0): GEN 3 with SMP %
# above 0 refuses and writes nothing (I8).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 06-norows
EMU_VERSION=GR0T emu_build norows -DRT_ROWS_MAX=0
emu_boot
emu_open
emu_page GEN3
emu_knob GEN3 SMP 8
emu_yes
emu_shot norows
emu_check "no lock rows: nothing written" "$ORIG
slock -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1" "$(emu_cmp 1)
$(emu_cmp 4)"
emu_done
