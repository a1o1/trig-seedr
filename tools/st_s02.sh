#!/bin/sh
# Task S0.2 (docs/plan-syntakt.md): analog-emu runs the stock Syntakt OS 1.42 from this checkout.
#   1. a cold boot on a new card; RECORD lights the default pattern's track 1 trigs (steps 1 and 4)
#   2. keys by name: T2 in grid recording adds a trig on step 2
#   3. snapshots: the next run resumes from the last one
#   4. TRIG, TRIG: stock's TRIG sub-pages (screenshots trig1.png, trig2.png)
#   5. power off to the card, cold boot on it: reports what the card kept (OPEN while analog-emu
#      models only the Rytm's power-off save)
# Writes only under build/emu/st-s02/.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu_st.sh"
emu_init st-s02

# 1. boot, RECORD
emu_boot
emu_shot boot
emu_keys RECORD
emu_check "boot, RECORD: track 1's trigs lit" "1 4" "$(st_lit)"

# 2. T2 in grid recording
emu_keys T2
emu_check "T2: a trig on step 2" "1 2 4" "$(st_lit)"
T2=$EMU_SNAP

# 3. a run from the snapshot resumes it: RECORD off and on again shows the same trigs
emu_keys RECORD RECORD
emu_check "resumed from the snapshot: the same trigs" "1 2 4" "$(st_lit)"

# 4. stock's TRIG sub-pages
emu_keys RECORD TRIG
emu_shot trig1
emu_keys TRIG
emu_shot trig2

# 5. power off with step 2's trig, cold boot on that card
emu_at "$T2"
emu_power_off
emu_boot "$EMU_CARD"
emu_keys RECORD
KEPT=$(st_lit)
if [ "$KEPT" = "1 2 4" ]; then
    emu_check "power off, cold boot: the trig on step 2 kept" "1 2 4" "$KEPT"
else
    echo "OPEN $EMU_NAME: power off, cold boot: trigs lit $KEPT (the edit was not kept; $(grep -o '[0-9]* sectors written' "$D/power-off.log"))"
fi
emu_done
