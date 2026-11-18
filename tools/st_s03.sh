#!/bin/sh
# Task S0.3 (docs/plan-syntakt.md): elekloader builds for the Syntakt OS 1.42 in analog-emu.
#   1. the hook-bus core alone (tools/st_build.sh --core mini) boots to the same screen as stock
#   2. with mods/hello-st: tick and draw run; keys and an encoder turn reach ev_key / ev_enc; on its first tick
#      the native heap gives seedr's buffers (107,336 bytes), and the largest block it gives is reported
#   3. Synfact's core-st: reported OPEN while it does not fit its 64 KiB area (gcc 16)
# Needs an analog-emu that recognises a modified Syntakt image: the local branch syntakt-modified, built in
# ../analog-emu-st (status/S0.3.md). ST_B overrides that folder. Writes only under build/emu/st-s03/ and build/emu-st/.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init st-s03

# use FW: boot this build; screen: the screen as text
use() { FW=$1 DTEMU_FW=$1; export DTEMU_FW; MAP=$1.map.json; }
screen() { "$DTPEEK" "$EMU_SNAP" fb; }
# hello FIELD: a word of mods/hello-st's struct hello
hello() {
    _h_i=0
    for _h_f in magic ticks draws keys encs last_key last_flags buf buf_ok largest; do
        [ "$_h_f" = "$1" ] && break
        _h_i=$((_h_i + 1))
    done
    emu_py peek "$EMU_SNAP" $(($(emu_py sym "$MAP" hello-st:hello) + 4 * _h_i)) 4 1 -s
}

# 0. stock, for the screen to compare with
emu_boot
STOCK_SCREEN=$(screen)
emu_keys TRIG
STOCK_TRIG=$(screen)

# 1. the hook-bus core alone
use "$(sh "$T/st_build.sh" core-mini | tail -1)"
emu_boot
emu_shot core-mini
emu_check "core (hook bus) alone: the stock main screen" "$STOCK_SCREEN" "$(screen)"

# 2. core + hello
use "$(sh "$T/st_build.sh" hello "$R/mods/hello-st" | tail -1)"
emu_boot
emu_check "hello: its first tick ran" 1212501071 "$(hello magic)"
emu_check "hello: tick and draw run" "1 1" "$([ "$(hello ticks)" -gt 0 ] && echo 1) $([ "$(hello draws)" -gt 0 ] && echo 1)"
emu_check "hello: the screen is still stock's" "$STOCK_SCREEN" "$(screen)"
emu_check "hello: the heap gives seedr's 107,336 bytes, both ends written" 1 "$(hello buf_ok)"
echo "  heap: seedr's buffer at $(printf 0x%08x "$(hello buf)"); the largest block after it: $(hello largest) bytes (to 4 KiB)"
K0=$(hello keys)
emu_keys TRIG
emu_check "hello: TRIG reaches ev_key (press and release)" 2 "$(($(hello keys) - K0))"
emu_check "hello: stock still gets TRIG (its Trig Parameters page)" "$STOCK_TRIG" "$(screen)"
emu_shot hello-trig
emu_run --turn 0:8@+100Mx4/50M
emu_check "hello: a knob turn reaches ev_enc" 1 "$([ "$(hello encs)" -gt 0 ] && echo 1)"

# 3. Synfact's core-st
if sh "$T/st_build.sh" core-st --core st > "$D/core-st.log" 2>&1; then
    use "$(tail -1 "$D/core-st.log")"
    emu_boot
    emu_check "core-st alone: the stock main screen" "$STOCK_SCREEN" "$(screen)"
else
    echo "OPEN $EMU_NAME: core-st does not link: $(grep -o 'REFUSED.*' "$W/../emu-st/core-st.patch.log" "$R/build/emu-st/core-st.patch.log" 2>/dev/null | head -1 | sed 's/^[^:]*://')"
fi
emu_done
