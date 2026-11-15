#!/bin/sh
# Release 0.3's part 6 on version 1: a test build where no new lock row fits (RT_ROWS_MAX 0): a
# take with LOCK lane 1 above 0 % is refused and nothing is written (I8).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 06-norows
EMU_VERSION=GR0T emu_build norows -DRT_ROWS_MAX=0
emu_boot
emu_open
emu_page LOCK
emu_knob LOCK B:lane1.pct 8
emu_yes
emu_shot norows
check_take "no lock rows: nothing written"
emu_check "no lock rows: no audition started" 0 "$(emu_get auditioning)"
emu_done
