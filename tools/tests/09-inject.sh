#!/bin/sh
# Settings written into the mod's memory (dtboot --poke) instead of turned on the knobs: a test
# sets a whole recipe in one run. On release 0.3 the settings are seedr.c's prm; on version 1
# emu_recipe writes the recipe store (profile v1, docs/status/0.5.md).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 09-inject
emu_build
emu_fixture kept2 fix_kept2
if [ "$EMU_PROFILE" = r03 ]; then
    emu_poke prm.dens 58
    emu_poke prm.bias -40
    emu_poke prm.vel_lo 20
    emu_poke prm.vel_hi 30
    emu_poke gen_dirty 1                 # as after a knob turn: YES keeps the seed (2)
    emu_open
    emu_check "the poked settings are the mod's" "58 -40 20 30" \
        "$(emu_get prm.dens) $(emu_get prm.bias) $(emu_get prm.vel_lo) $(emu_get prm.vel_hi)"
    emu_yes
    p=$(GENREF_METER='4 0 -40 0 0' genref 2 16 0 0 0 0 0 0 1 58)
    # genref takes velocities 60..120; the vel_lo / vel_hi pokes are checked on the trigs' count
    emu_check "YES with poked DENS 58, BIAS -40: the trigs" \
        "$(echo "$p" | head -1 | tr ' ' '\n' | cut -c1 | tr -d '\n')" "$(emu_cmp 1 | tr ' ' '\n' | cut -c1 | tr -d '\n')"
    emu_check "velocities in the poked range 20..30" ok \
        "$(emu_dump | awk '$1 == "vel" { for (i = 2; i <= NF; i++) if ($i != "-" && ($i < 20 || $i > 30)) bad = 1; print bad ? "bad: " $0 : "ok" }')"
else
    emu_recipe 0 src=random dens=58 bias=-40 vel=20:30
    emu_open
    emu_yes
    emu_check "YES with an injected recipe = seedref" \
        "$(seedref src=random dens=58 bias=-40 vel=20:30)" "$(emu_dump)"
fi
emu_done
