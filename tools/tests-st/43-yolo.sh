#!/bin/sh
# TOOLS YOLO on the Syntakt (plan-syntakt.md S4; the Rytm's 43): YES rolls a recipe for the track (lib/yolo.c,
# the track's role by its place: track 4 is percussion) and generates it; the rolled recipe and the take are
# what seedref --device st yolo=ROLL:WILD makes; YES again takes the next roll; NO puts the track and the recipe
# back as they were before the first roll; YES after NO brings the roll heard back; FUNC + YES keeps both;
# RECORD off with a roll pending puts both back. On track 4 from the kept2 fixture.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st43-yolo
st_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
pending() { emu_get auditioning; }
recipe_dev() {
    seedref --recipe track=$EMU_TRACK | while read -r _rd_p _rd_v; do
        [ "$_rd_p" = seed ] && echo "seed $(emu_get "recipe[$EMU_TRACK].seed")" && continue
        echo "$_rd_p $(emu_get "recipe[$EMU_TRACK].$_rd_p")"
    done
}
FROM="metr=$(emu_get "recipe[$EMU_TRACK].metr") beat=$(emu_get "recipe[$EMU_TRACK].beat") lane1.parm=$(emu_get "recipe[$EMU_TRACK].lane[0].parm") lane2.parm=$(emu_get "recipe[$EMU_TRACK].lane[1].parm")"
FROM="$FROM map_x=$(emu_get shared.map_x) map_y=$(emu_get shared.map_y) map_chaos=$(emu_get shared.map_chaos)"
REC0=$(recipe_dev)

emu_open
emu_page TOOLS
emu_knob TOOLS TOOL 1
emu_check "TOOL = YOLO: WILD 50, ROLL 0" "50 0" "$(emu_get tool.wild) $(emu_get tool.roll)"

# 1. YES: roll 0 at WILD 50
# shellcheck disable=SC2086
emu_yes
check_take "YES: the take of roll 0, WILD 50" $FROM yolo=0:50 "$K"
emu_check "YES: the recipe is roll 0's" "$(seedref --recipe track=$EMU_TRACK $FROM yolo=0:50 "$K")" "$(recipe_dev)"
emu_check "YES: a take pending, ROLL 0 on the strip" "1 ROLL 0" "$(pending) $(emu_msg)"

# 2. YES again: the next roll
emu_keys YES
check_take "YES again: roll 1" $FROM yolo=1:50 "$K"

# 3. NO: the track and the recipe as before the first roll; YES after NO: roll 1 again
emu_no
check_take "NO: track 4's original" "$K"
emu_check "NO: the recipe as before the first roll" "$REC0" "$(recipe_dev)"
emu_yes
check_take "YES after NO: roll 1 again" $FROM yolo=1:50 "$K"

# 4. WILD 90, ROLL 7, FUNC + YES keeps the roll and its take
emu_knob TOOLS WILD 90
emu_knob TOOLS ROLL 7
emu_yes
check_take "ROLL 7, WILD 90" $FROM yolo=7:90 "$K"
emu_keep
emu_check "FUNC + YES: APPLIED" "APPLIED" "$(emu_msg)"
emu_check "applied: the recipe stays roll 7's" "$(seedref --recipe track=$EMU_TRACK $FROM yolo=7:90 "$K")" "$(recipe_dev)"
save_orig rolled7
REC7=$(recipe_dev)

# 5. a roll pending, RECORD off: NOT APPLIED, the applied take and roll 7's recipe back
emu_yes
check_take "YES: roll 8 from the applied take" $FROM yolo=8:90 "orig=$D/rolled7.txt"
emu_rec 0
emu_check "RECORD off: NOT APPLIED" "NOT APPLIED" "$(emu_msg)"
check_take "RECORD off: the applied take is back" "orig=$D/rolled7.txt"
emu_check "RECORD off: roll 7's recipe is back" "$REC7" "$(recipe_dev)"
emu_done
