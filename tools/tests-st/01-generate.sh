#!/bin/sh
# The Rytm's 01 on the Syntakt (plan-syntakt.md S4): an empty track's recipe is RAND; YES gives seed 1 as
# tools/seedref --device st renders it; seed 2 from the original; NO; FUNC + YES applies; closing keeps the
# take. On track 4 of the preset A01, emptied with stock's keys (fixtures.sh).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st01-generate
st_build
fix_kept2
emu_fixture_save kept2
emu_done
