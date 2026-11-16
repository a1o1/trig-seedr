#!/bin/sh
# tools/screens.sh: the screenshots of the ten pages the README and the site show (docs/screens/<page>.png),
# and TOOLS with a YOLO roll pending (yolo.png).
#     sh tools/screens.sh             writes them to build/emu/screens/ to look at
#     sh tools/screens.sh --install   also copies them into docs/screens/
# One rich example, so every page shows real values and the strip's bars (velocity) vary: A01 BD made
# 64 steps long, with a recipe that uses many features at once (MAP as the source with velocity from the
# map's levels, SYNC, accents on the strongest steps, a NOTE list, a noise contour on lane 1, conditions,
# retrigs, a groove with drift, a phrase and candidate scoring), generated and applied with grid
# recording on; VIEW's cursor on a step that carries several of them. Needs what tools/run_tests.sh needs.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu.sh"
emu_init screens
emu_build
emu_boot
emu_run --poke 407ddba8=40000200                # A01 BD 64 steps long (record +0x2c5)
emu_recipe 0 src=map map_part=0 map_dens=190 sync=40 vel=30:127 shape=map amt=100 acc=30 amod=top hum=10 \
    note=-12:12 note.src=list note.len=8 note.order=ping lmod=legato \
    lane1.pct=60 lane1=1:8 lane1.src=noise lane1.zoom=8 cond=25 ctyp=mix rtrg=15 rate=9:12 \
    grv=1 timing=80 drft=30 vary=40 phrs=aaab qual=4 seed=3
emu_poke shared.map_x 60
emu_poke shared.map_y 190
emu_poke shared.map_chaos 40
emu_rec 1
emu_open
emu_yes
emu_keep
emu_run -s 2400M                                # past the APPLIED message, so the strip shows
emu_poke cursor 20                              # VIEW's cursor: step 21
for p in TAKE RHYTHM PLACE DYNAMICS NOTE LOCK TRIG TIME VIEW TOOLS; do
    emu_page "$p"
    emu_run -s 600M
    emu_shot "$(echo "$p" | tr 'A-Z' 'a-z')"
done
# TOOLS with YOLO: a roll pending (the dot in the title bar), WILD 60
emu_page TOOLS
emu_knob TOOLS TOOL 1
emu_knob TOOLS WILD 60
emu_knob TOOLS ROLL 3
emu_keys YES
emu_run -s 2400M                                # past the ROLL message, so the strip shows
emu_shot yolo
emu_no
echo "screens: $D"
if [ "$1" = --install ]; then
    for p in take rhythm place dynamics note lock trig time view tools yolo; do cp "$D/$p.png" "$R/docs/screens/$p.png"; done
    echo "copied into docs/screens/"
fi
