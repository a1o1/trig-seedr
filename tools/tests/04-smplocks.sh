#!/bin/sh
# Release 0.3's part 4 on version 1: LOCK lane 1 (sample slot) locks; NO removes them; they
# survive SAVE + RELOAD PATTERN. Leaves fixture "smp3" (05-rotate starts from it).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 04-smplocks
emu_build
fix_smp3
emu_fixture_save smp3
emu_done
