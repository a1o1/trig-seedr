#!/bin/sh
# Release 0.3's part 1 on version 1: build; boot empty; YES seed 1; seed 2 from the original; NO;
# FUNC + YES; closing keeps; each take as tools/seedref renders it. Leaves fixture "kept2".
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 01-generate
emu_build
fix_kept2
emu_shot closed
emu_fixture_save kept2
emu_done
