#!/bin/sh
# Release 0.3's part 7: the keys. TRIG on EUCLIDEAN opens the mod, TRIG again closes it (stock: TRIG
# page), TRIG: EUCLIDEAN, TRIG: the mod again; a page key (SRC) closes it. Checked on the mod's open
# flag, stock's TRIG sub-page (tools/emu.py trigpage) and the screen's title bar.
#
# The fixture ends with stock on its TRIG sub-page (0), not on EUCLIDEAN, so the first TRIG goes to
# EUCLIDEAN. (tools/test_seedr.sh part 7 starts from the same state and compares two EUCLIDEAN
# screens where it means the mod's: docs/status/0.5.md.)
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 07-keys
emu_build
emu_fixture kept2 fix_kept2
state() { echo "open $(emu_get open), stock $(emu_py trigpage "$MAP" "$EMU_SNAP")"; }
emu_check "start: closed, stock's TRIG page" "open 0, stock 0" "$(state)"
emu_keys TRIG
emu_check "TRIG: stock's EUCLIDEAN page" "open 0, stock 1" "$(state)"
t0=$(emu_py title "$EMU_SNAP")
emu_keys TRIG
emu_check "TRIG on EUCLIDEAN opens the mod" "open 1, stock 1" "$(state)"
t1=$(emu_py title "$EMU_SNAP")
emu_check "the mod's screen is not stock's" 1 "$([ "$t1" != "$t0" ] && echo 1)"
emu_keys TRIG
emu_check "TRIG: closed, stock's TRIG page" "open 0, stock 0" "$(state)"
emu_keys TRIG
emu_check "TRIG: EUCLIDEAN" "open 0, stock 1" "$(state)"
emu_keys TRIG
emu_check "TRIG: the mod again, same screen" "open 1, stock 1, $t1" "$(state), $(emu_py title "$EMU_SNAP")"
emu_keys SRC
emu_check "SRC closes it" "open 0, stock -1" "$(state)"
emu_done
