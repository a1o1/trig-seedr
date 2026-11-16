#!/bin/sh
# TOOLS YOLO (0.7, Alan): YES rolls a recipe for this track (lib/yolo.c) and generates it; the rolled
# recipe and the take are what seedref yolo=ROLL:WILD makes; YES again takes the next roll; NO puts the
# track and the recipe back as they were before the first roll; FUNC + YES keeps both; leaving with a
# roll pending (RECORD off) puts both back. On A01 BD from the kept2 fixture.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 43-yolo
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
pending() { emu_get auditioning; }
# the device's recipe for BD, every field, as seedref --recipe prints one
recipe_dev() {
    seedref --recipe | while read -r _rd_p _rd_v; do
        [ "$_rd_p" = seed ] && echo "seed $(emu_get 'recipe[0].seed')" && continue
        echo "$_rd_p $(emu_get "recipe[0].$_rd_p")"
    done
}
# the user's meter and lanes, which a roll keeps
FROM="metr=$(emu_get 'recipe[0].metr') beat=$(emu_get 'recipe[0].beat') lane1.parm=$(emu_get 'recipe[0].lane[0].parm') lane2.parm=$(emu_get 'recipe[0].lane[1].parm')"
REC0=$(recipe_dev)

emu_open
emu_page TOOLS
emu_knob TOOLS TOOL 1
emu_check "TOOL = YOLO: WILD 50, ROLL 0" "50 0" "$(emu_get tool.wild) $(emu_get tool.roll)"

# 1. YES: roll 0 at WILD 50, generated
# shellcheck disable=SC2086
emu_yes
check_take "YES: the take of roll 0, WILD 50" $FROM yolo=0:50 "$K"
emu_check "YES: the recipe is roll 0's" "$(seedref --recipe $FROM yolo=0:50 "$K")" "$(recipe_dev)"
emu_check "YES: a take pending, ROLL 0 on the strip" "1 ROLL 0" "$(pending) $(emu_msg)"
emu_check "the roll changed the recipe" 1 "$([ "$(recipe_dev)" != "$REC0" ] && echo 1)"

# 2. YES again: the next roll
emu_keys YES
check_take "YES again: roll 1" $FROM yolo=1:50 "$K"
emu_check "YES again: ROLL 1" 1 "$(emu_get tool.roll)"

# 3. NO: the track and the recipe as before the first roll
emu_no
check_take "NO: BD's original" "$K"
emu_check "NO: the recipe as before the first roll" "$REC0" "$(recipe_dev)"
emu_check "NO: nothing pending" 0 "$(pending)"

# 4. WILD 90, a chosen ROLL, FUNC + YES keeps the roll and its take
emu_knob TOOLS WILD 90
emu_knob TOOLS ROLL 7
emu_yes
check_take "ROLL 7, WILD 90" $FROM yolo=7:90 "$K"
emu_keep
emu_check "FUNC + YES: APPLIED" "APPLIED" "$(emu_msg)"
emu_check "applied: the recipe stays roll 7's" "$(seedref --recipe $FROM yolo=7:90 "$K")" "$(recipe_dev)"
check_take "applied: the take stays" $FROM yolo=7:90 "$K"
save_orig rolled7
REC7=$(recipe_dev)

# 5. a roll pending, RECORD off: NOT APPLIED, the applied take and roll 7's recipe back
emu_yes
check_take "YES: roll 8 from the applied take" $FROM yolo=8:90 "orig=$D/rolled7.txt"
emu_rec 0
emu_check "RECORD off: NOT APPLIED" "NOT APPLIED" "$(emu_msg)"
check_take "RECORD off: the applied take is back" "orig=$D/rolled7.txt"
emu_check "RECORD off: roll 7's recipe is back" "$REC7" "$(recipe_dev)"

# 6. RECORD off: YES on YOLO writes nothing and leaves the recipe
emu_keys YES
emu_check "RECORD off: RECORD TO GENERATE" "RECORD TO GENERATE" "$(emu_msg)"
emu_check "RECORD off: the recipe untouched" "$REC7" "$(recipe_dev)"
emu_done
