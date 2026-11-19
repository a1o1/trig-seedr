#!/bin/sh
# A9 (requirements.md; design-v1.md section 10): YES writes nothing on the FX track (NOT ON THE FX
# TRACK) or on a track in stock's Euclidean mode (EUCLID IS ON: TURN IT OFF). The same for TOOLS' YOLO,
# which also leaves the recipe as it was (in Euclidean mode: on the FX track seedr skips TOOLS, so
# yolo_run's own FX check cannot be reached from the keys). Nothing is pending after a refusal, and no
# track changes.
#
# 1. The FX track (TRK + FX, grid recording off), from a new project.
# 2. A01 BD with stock's EUC on (TRIG, TRIG: stock's EUCLIDEAN page; firmware-notes-os174.md section 2,
#    +0x2CA), seedr opened from there with TRIG; then EUC off: YES generates again.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 44-refuse
emu_build
pending() { emu_get auditioning; }
# every track (BD .. FX) in the dump format, as one digest
tracks() {
    _tr_t=0
    while [ $_tr_t -le 12 ]; do emu_dump $_tr_t; _tr_t=$((_tr_t + 1)); done | shasum | cut -d' ' -f1
}
# the device's recipe for TRACK, every field
recipe_dev() {
    seedref --recipe | while read -r _rd_p _rd_v; do
        echo "$_rd_p $(emu_get "recipe[$1].$_rd_p")"
    done
}
BD=0x407dd8e3                                           # A01 BD's record (tools/emu.py REC0)
EUC=$((BD + 0x2ca)) PL1=$((BD + 0x2cb))
K_PL1=0 K_EUC=3                                         # stock's EUCLIDEAN page: A PL1, D EUC

# ---- 1. the FX track ----
emu_boot
emu_rec 0                                               # TRK + a key selects a track only with RECORD off
emu_keys TRK+FX
emu_open
ALL=$(tracks)
emu_yes
emu_check "FX track, YES: NOT ON THE FX TRACK" "NOT ON THE FX TRACK" "$(emu_msg)"
emu_check "FX track, YES: nothing pending" 0 "$(pending)"
emu_check "FX track, YES: no track changed" "$ALL" "$(tracks)"
emu_no
emu_check "NO: closed" 0 "$(emu_get open)"

# ---- 2. stock's Euclidean mode on BD ----
emu_rec 0
emu_keys TRK+T1
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's EUCLIDEAN page" 1 "$(emu_py trigpage "$MAP" "$EMU_SNAP")"
emu_sknob $K_EUC "$EUC" 1
emu_sknob $K_PL1 "$PL1" 5
emu_keys TRIG
emu_check "TRIG on stock's EUCLIDEAN page: seedr opens" 1 "$(emu_get open)"
ALL=$(tracks)
R0=$(recipe_dev 0)
emu_yes
emu_shot euclid-on
emu_check "EUC on, YES: EUCLID IS ON: TURN IT OFF" "EUCLID IS ON: TURN IT OFF" "$(emu_msg)"
emu_check "EUC on, YES: nothing pending" 0 "$(pending)"
emu_check "EUC on, YES: no track changed" "$ALL" "$(tracks)"
emu_check "EUC on, YES: EUC still on" 1 "$(emu_py peek "$EMU_SNAP" "$EUC" 1)"
emu_page TOOLS
emu_knob TOOLS TOOL 1
emu_keys YES
emu_check "EUC on, YOLO: EUCLID IS ON: TURN IT OFF" "EUCLID IS ON: TURN IT OFF" "$(emu_msg)"
emu_check "EUC on, YOLO: nothing pending" 0 "$(pending)"
emu_check "EUC on, YOLO: no track changed" "$ALL" "$(tracks)"
emu_check "EUC on, YOLO: the recipe as it was" "$R0" "$(recipe_dev 0)"

# EUC off again (stock's page: seedr closes), then YES generates: the refusal came from EUC alone
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's EUCLIDEAN page again" 1 "$(emu_py trigpage "$MAP" "$EMU_SNAP")"
emu_sknob $K_EUC "$EUC" 0
save_orig euc-off
emu_open
emu_page TAKE
emu_yes
emu_check "EUC off, YES: a take pending" 1 "$(pending)"
check_take "EUC off, YES: BD's recipe, rendered" src=random seed=1 orig="$D/euc-off.txt"
emu_done
