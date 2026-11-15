#!/bin/sh
# Release 0.3's part 8: METER: SKIP 2, BIAS -40 %, NOT 1 at 100 % against genref; NO.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 08-meter
emu_build
emu_fixture kept2 fix_kept2
emu_open
emu_page METER
emu_knob METER SKIP 2
emu_knob METER VSHP 4
emu_knob METER BIAS -40
emu_knob METER VAMT 100
emu_yes
emu_shot meter
check_gen "METER SKIP 2, BIAS -40, NOT 1 100 %" "$(GENREF_METER='4 2 -40 4 100' genref 2)"
emu_no
check_gen "NO: seed 2" "$(genref 2)"
emu_done
