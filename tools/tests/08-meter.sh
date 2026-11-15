#!/bin/sh
# Release 0.3's part 8 on version 1: SKIP 2 (PLACE), BIAS -40 (RHYTHM), SHAPE NOT 1 at 100 %
# (DYNAMICS) against tools/seedref; NO.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 08-meter
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_page RHYTHM
emu_knob RHYTHM D:bias -40
emu_page PLACE
emu_knob PLACE E:skip 2
emu_page DYNAMICS
emu_knob DYNAMICS B:shape:one 4
emu_knob DYNAMICS C:amt 100
emu_yes
emu_shot meter
check_take "SKIP 2, BIAS -40, NOT 1 100 %" src=random seed=2 bias=-40 skip=2 shape=not1 amt=100 "$K"
emu_no
check_take "NO: seed 2" src=random seed=2 "$K"
emu_done
