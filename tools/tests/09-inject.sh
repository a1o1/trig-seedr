#!/bin/sh
# Settings written into the mod's recipe store (dtboot --poke, tools/emu.sh emu_recipe) instead of
# turned on the knobs: a test sets a whole recipe in one run.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 09-inject
emu_build
emu_boot
emu_recipe 0 src=random dens=58 bias=-40 vel=20:30
emu_open
emu_check "the injected settings are the mod's" "1 58 -40 20 30" \
    "$(emu_get 'recipe[0].src') $(emu_get 'recipe[0].dens') $(emu_get 'recipe[0].bias') $(emu_get 'recipe[0].vel.lo') $(emu_get 'recipe[0].vel.hi')"
emu_yes
check_take "YES with an injected recipe = seedref" src=random dens=58 bias=-40 vel=20:30
emu_done
