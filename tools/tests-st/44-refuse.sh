#!/bin/sh
# A9 on the Syntakt (plan-syntakt.md S4; the Rytm's 44): YES writes nothing on the FX track (NOT ON THE FX
# TRACK) or on a track in stock's Euclidean mode (EUCLID IS ON: TURN IT OFF), and TOOLS' YOLO neither, leaving
# the recipe as it was. Nothing is pending after a refusal, and no track's record or lock store changes.
# 1. The FX track (the FX key selects it). 2. Track 1 with stock's EUC on (TRIG, TRIG: the Euclidean page;
# EUC is knob D, record +0x3d1), seedr opened from there with TRIG; then EUC off: YES generates again.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st44-refuse
st_build
pending() { emu_get auditioning; }
recipe_dev() {
    seedref --recipe | while read -r _rd_p _rd_v; do echo "$_rd_p $(emu_get "recipe[$1].$_rd_p")"; done
}

# ---- 1. the FX track ----
emu_boot
emu_rec 0
emu_keys FX
emu_open
ALL=$(st_tracks)
emu_yes
emu_check "FX track, YES: NOT ON THE FX TRACK" "NOT ON THE FX TRACK" "$(emu_msg)"
emu_check "FX track, YES: nothing pending" 0 "$(pending)"
emu_check "FX track, YES: no track changed" "$ALL" "$(st_tracks)"
emu_no
emu_check "NO: closed" 0 "$(emu_get open)"

# ---- 2. stock's Euclidean mode on track 1 ----
emu_rec 0
emu_keys TRK+T1
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's Euclidean page" 1 "$(st_peek trigpage "$EMU_SNAP")"
EUC=$(($(st_rec_addr 0) + 0x3d1))
emu_sknob 3 "$EUC" 1
emu_keys TRIG
emu_check "TRIG on stock's Euclidean page: seedr opens" 1 "$(emu_get open)"
ALL=$(st_tracks)
R0=$(recipe_dev 0)
emu_yes
emu_shot euclid-on
emu_check "EUC on, YES: EUCLID IS ON: TURN IT OFF" "EUCLID IS ON: TURN IT OFF" "$(emu_msg)"
emu_check "EUC on, YES: nothing pending" 0 "$(pending)"
emu_check "EUC on, YES: no track changed" "$ALL" "$(st_tracks)"
emu_check "EUC on, YES: EUC still on" 1 "$(emu_py peek "$EMU_SNAP" "$EUC" 1)"
emu_page TOOLS
EMU_TRACK=0 emu_knob TOOLS TOOL 1
emu_keys YES
emu_check "EUC on, YOLO: EUCLID IS ON: TURN IT OFF" "EUCLID IS ON: TURN IT OFF" "$(emu_msg)"
emu_check "EUC on, YOLO: nothing pending" 0 "$(pending)"
emu_check "EUC on, YOLO: no track changed" "$ALL" "$(st_tracks)"
emu_check "EUC on, YOLO: the recipe as it was" "$R0" "$(recipe_dev 0)"

# EUC off again (stock's page: seedr closes), then YES generates: the refusal came from EUC alone
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's Euclidean page again" 1 "$(st_peek trigpage "$EMU_SNAP")"
emu_sknob 3 "$EUC" 0
EMU_TRACK=0
save_orig euc-off
emu_open
emu_page TAKE
emu_yes
emu_check "EUC off, YES: a take pending" 1 "$(pending)"
check_take "EUC off, YES: track 1's recipe (SRC TRACK, every layer 0), rendered" orig="$D/euc-off.txt" init=1
emu_done
