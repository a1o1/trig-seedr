# tools/emu_st.sh: tools/emu.sh on the Syntakt (plan-syntakt.md, task S0.2). Sourced as emu.sh is:
#     R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu_st.sh"; emu_init <name>
# It runs the stock Syntakt OS 1.42 (no mod is built yet): emu_run, emu_keys, emu_shot, emu_check,
# emu_done and emu_power_off are emu.sh's; emu_boot and the trig LEDs are the Syntakt's.

: "${R:?set R to the repository before sourcing tools/emu_st.sh}"
case $R in
*/.claude/worktrees/*) _st_e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _st_e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
STOCK=${STOCK_ST:-"$_st_e/Official Firmwares/Syntakt OS 1.42/Syntakt OS/Syntakt_OS1.42.syx"}
. "$R/tools/emu.sh"
FW=$STOCK
export DTEMU_FW=$FW                     # dtpeek reads the firmware from it (emu_shot, st_lit)

# emu_boot [CARD]: a cold boot to the main screen (4G; stock 1.42 shows no start-up dialog), on
# CARD, or on a new formatted card
emu_boot() {
    if [ -n "$1" ]; then EMU_CARD=$1
    else
        EMU_CARD=$D/card.img; rm -f "$EMU_CARD"
        "$B/dtcard" "$EMU_CARD" format > /dev/null || emu_die "dtcard format"
    fi
    emu_next; _boot_s=$EMU_S
    "$B/dtboot" "$FW" --instr 4G --card "$EMU_CARD" --save "$_boot_s.snap" > "$_boot_s.log" 2>&1 \
        || { tail -5 "$_boot_s.log" >&2; emu_die "boot"; }
    echo 4000000000 > "$_boot_s.snap.clk"
    emu_at "$_boot_s.snap"
}

# st_lit: the trig keys (1..16) lit red, as stock lights the track's trigs in grid recording
st_lit() {
    "$DTPEEK" "$EMU_SNAP" leds | sed -n 's/^led \([0-9]*\):.*Some((\([0-9]*\), \([0-9]*\), \([0-9]*\)))$/\1 \2 \3 \4/p' |
        awk '$1 < 16 && $2 >= 100 && $3 < 50 && $4 < 50 { s = s " " $1 + 1 } END { print substr(s, 2) }'
}
