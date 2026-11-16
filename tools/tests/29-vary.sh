#!/bin/sh
# Task F9: PHRS and CVAR on the device, each take against tools/seedref.
#   1. A01 BD at 64 steps (stock's EUCLIDEAN LEN, EUC off again), empty, so its recipe is RANDOM.
#      PHRS ABAB: pages 1 and 3 have the same trigs, and so have 2 and 4; PHRS ABCD on seed 5.
#   2. CVAR 1:2 (PHRS OFF): takes 1 and 2 as 1:2 and 2:2 conditions; CVAR AAAB with PHRS AAAB and
#      VARY 50. Kept, SAVE and RELOAD PATTERN: the conditions stay.
#   3. A track shorter than four pages: LEN 24, the kept take as the original, PHRS ABAB on
#      SOURCE = TRACK with VEL and COND (page by page as far as the track goes); NO restores it.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 29-vary
emu_build

BD=0x407dd8e3                                           # A01 BD's record (tools/emu.py REC0)
LEN=$((BD + 0x2c5)) EUC=$((BD + 0x2ca))
K_LEN=2 K_EUC=3                                         # stock's EUCLIDEAN page: C LEN, D EUC
byte() { emu_py peek "$EMU_SNAP" "$1" 1; }
# set_len N: BD's length through stock's EUCLIDEAN page, Euclidean mode off again (22-euclid.sh)
set_len() {
    emu_keys TRIG TRIG
    emu_sknob $K_EUC "$EUC" 1
    emu_sknob $K_LEN "$LEN" "$1"
    emu_sknob $K_EUC "$EUC" 0
    emu_check "stock LEN $1, EUC off" "$1 0" "$(byte "$LEN") $(byte "$EUC")"
}
# page P: BD's trig row, page P (1..4) of 16 steps
page() { emu_dump | sed -n 's/^trig //p' | cut -d' ' -f$((16 * $1 - 15))-$((16 * $1)); }
# conds: the distinct trig conditions on BD
conds() { emu_dump | sed -n 's/^cond //p' | tr ' ' '\n' | sort -u | tr '\n' ' '; }

# ---- 1. PHRS ----
emu_boot
set_len 64
emu_open
emu_check "open: the empty track's new recipe is RANDOM (V6)" 1 "$(emu_get 'recipe[0].src')"
emu_knob TAKE C:phrs:one 3
emu_yes
emu_shot phrs
check_take "PHRS ABAB, 64 steps" src=random phrs=abab length=64
emu_check "ABAB: page 3 is page 1, page 4 is page 2" "$(page 1) $(page 2)" "$(page 3) $(page 4)"
emu_knob TAKE A:seed 5
emu_knob TAKE C:phrs:one 5
emu_yes
check_take "PHRS ABCD, seed 5" src=random seed=5 phrs=abcd length=64

# ---- 2. CVAR ----
emu_knob TAKE A:seed 1
emu_knob TAKE C:phrs:one 0
emu_knob TAKE D:cvar:one 1
emu_yes
emu_shot cvar
check_take "CVAR 1:2" src=random cvar=1:2 length=64
emu_check "CVAR 1:2: plain trigs, 1:2 and 2:2" "- 30 31 " "$(conds)"
emu_knob TAKE B:vary 50
emu_knob TAKE C:phrs:one 2
emu_knob TAKE D:cvar:one 2
emu_yes
check_take "CVAR AAAB, PHRS AAAB, VARY 50" src=random vary=50 phrs=aaab cvar=aaab length=64
emu_keep
emu_no
emu_keys YES+SCNE w:300 NO+SCNE
check_take "kept, SAVE + RELOAD PATTERN: the conditions" src=random vary=50 phrs=aaab cvar=aaab length=64

# ---- 3. a track of a page and a half ----
set_len 24
save_orig len24
O="orig=$D/len24.txt"
emu_open
emu_page TAKE
emu_knob TAKE D:cvar:one 0              # before SRC TRACK, which hides CVAR (M2.1: it would change nothing)
emu_page RHYTHM
emu_knob RHYTHM A:src:one 0
emu_page TAKE
emu_knob TAKE C:phrs:one 3
emu_page TRIG
emu_knob TRIG A:cond 50
emu_yes
emu_shot len24
check_take "24 steps, SOURCE = TRACK, PHRS ABAB, COND 50" src=track vary=50 phrs=abab cond=50 "$O"
emu_no
check_take "NO: the 24 steps as they were" "$O"
emu_done
