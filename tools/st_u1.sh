#!/bin/sh
# Task U1 (docs/plan-syntakt.md, unit readiness): a canary over the candidate mods' area on the stock Syntakt OS
# 1.42 in analog-emu. dtboot's --fill writes PAT ^ address over a range before the cold boot (before the OS's own
# reset code runs); a session of stock's keys follows; --check-fill counts the words that changed. Two controls
# show the probe sees writes: the DSP boot buffer and the DMA ring, which the OS fills during the boot.
#   1. a cold boot on a new card, filled; pages, knobs, grid-recording edits, PLAY, pattern changes, a project
#      save and LOAD PROJECT: the candidate unchanged
#   2. power off, a cold boot on that card (filled again), LOAD PROJECT, PLAY: the candidate unchanged
# Needs analog-emu-st's --fill / --check-fill (local branch syntakt-modified). ST_B overrides that folder.
# Writes only under build/emu/st-u1/.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init st-u1

PAT=a5c3e10f
AREA=${ST_AREA:-478b0000-47c00000}     # the candidate: above the DSP boot buffer, below the DMA ring
DSPBUF=4784f2c4-478af2c8               # control: the second CPU's program, copied there at boot
RING=47c00000-47c10000                 # control: the DMA ring's first 64 KiB, cleared at boot

# boot_filled [CARD]: emu_boot with the canary and the controls filled before the OS starts
boot_filled() {
    if [ -n "$1" ]; then EMU_CARD=$1
    else
        EMU_CARD=$D/card.img; rm -f "$EMU_CARD"
        "$B/dtcard" "$EMU_CARD" format > /dev/null || emu_die "dtcard format"
    fi
    emu_next; _bf_s=$EMU_S
    "$B/dtboot" "$FW" --instr 4G --card "$EMU_CARD" --fill $AREA=$PAT --fill $DSPBUF=$PAT --fill $RING=$PAT \
        --check-fill $DSPBUF=$PAT --check-fill $RING=$PAT --save "$_bf_s.snap" > "$_bf_s.log" 2>&1 \
        || { tail -5 "$_bf_s.log" >&2; emu_die "boot"; }
    echo 4000000000 > "$_bf_s.snap.clk"
    emu_at "$_bf_s.snap"
}
# changed RANGE: the words of RANGE no longer PAT ^ address, in the current snapshot
changed() {
    emu_run -s 1M --check-fill "$1=$PAT"
    sed -n "s/^check-fill: 0x[0-9a-f]*-0x[0-9a-f]*: \([0-9]*\) words changed.*/\1/p" "$EMU_S.log"
}
save_project() { emu_keys F+GLOBAL w:300 DOWN w:200 YES w:500 YES w:2500; }
load_project() { emu_keys GLOBAL w:300 YES w:300 YES w:300 DOWN w:200 YES w:1500 NO NO NO w:500; }

# ---- 1. a session ----
boot_filled
emu_check "boot: the controls were written by the OS (DSP buffer, DMA ring)" "ok ok" \
    "$(grep -c 'check-fill: 0x4784f2c4.*: [1-9][0-9]* words changed' "$EMU_S.log" | sed 's/1/ok/') $(grep -c 'check-fill: 0x47c00000.*: 16384 words changed' "$EMU_S.log" | sed 's/1/ok/')"
emu_check "boot: the candidate unchanged" 0 "$(changed $AREA)"
emu_keys TRIG TRIG TRIG SYN SYN FLTR AMP LFO FX TRK+T2 TRIG SYN TRK+T1 MENU w:300 NO RIGHT LEFT UP DOWN
emu_run --turn "0:8@+100M" --turn "1:-8@+300M" --turn "2:8@+500M" --turn "7:8@+700M"
emu_keys RECORD T1 T3 T5 T7 T9 T11 T13 T15 T16 RECORD
emu_keys PLAY w:3000 PTN+T2 w:3000 PTN+T3 w:2000 PTN+T1 w:2000 STOP
emu_check "session: the candidate unchanged" 0 "$(changed $AREA)"
save_project
load_project
emu_keys PLAY w:3000 STOP
emu_shot session
emu_check "project save and load, PLAY: the candidate unchanged" 0 "$(changed $AREA)"

# ---- 2. power off, cold boot on that card, LOAD PROJECT, PLAY ----
emu_power_off
boot_filled "$EMU_CARD"
emu_check "cold boot on the saved card: the candidate unchanged" 0 "$(changed $AREA)"
emu_keys GLOBAL w:300 YES w:300 YES w:300; emu_shot p2a
# after a cold boot stock asks SAVE 'NEW PROJECT' BEFORE LOADING? (NO), then ANY CHANGES WILL BE LOST. PROCEED
# WITH LOADING? (YES)
emu_keys DOWN w:200 YES w:1500 NO w:1500 YES w:3000
emu_shot loaded                         # LOAD OK
emu_keys NO NO w:300 PLAY w:4000 STOP
emu_check "LOAD PROJECT, PLAY: the candidate unchanged" 0 "$(changed $AREA)"
emu_done
