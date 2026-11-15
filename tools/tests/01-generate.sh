#!/bin/sh
# Release 0.3's part 1: build; boot empty; YES seed 1; seed 2 from the original; NO; FUNC + YES;
# closing keeps; a take equals a fresh generate. Leaves fixture "kept2" for the other files.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 01-generate
emu_build
fix_kept2
emu_shot closed
emu_fixture_save kept2
emu_done
