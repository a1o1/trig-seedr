#!/bin/sh
# Task S6 (docs/plan-syntakt.md, docs/flashing-st.md): the memory canary for a Syntakt unit (mods/canary-st), built
# and checked in analog-emu-st before Alan flashes it.
#   1. builds it in ST_AREA=47bf0000-47c00000 with the minimal core: build/emu-st/canary-emu.syx, and with --unit
#      build/emu-st/canary-unit.syx (elektron-firmware-tool, tools/st_build.sh's checks)
#   2. a session: boot, filling, then whole passes with every word as written through keys, grid recording, a
#      trig, PLAY and STOP, the TRIG pages, a pattern change; the box says OK and the number of passes
#   3. a word poked into the range: the next pass stops at it and the box says BAD and its address
# Writes only under build/emu/st-canary/ and build/emu-st/. The unit file is never sent anywhere from here.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init st-canary
ST_AREA=47bf0000-47c00000 sh "$T/st_build.sh" canary --unit "$R/mods/canary-st" > "$D/build.log" 2>&1 \
    || { tail -5 "$D/build.log"; emu_die "st_build.sh"; }
FW=$(grep 'canary-emu.syx$' "$D/build.log" | tail -1)
U=$(grep 'canary-unit.syx$' "$D/build.log" | tail -1)
grep '^verified canary-unit' "$D/build.log"
emu_check "the unit file verifies (container within the official size, margin at least 4096)" yes "$([ -f "$U" ] && echo yes)"
DTEMU_FW=$FW MAP=$FW.map.json
export DTEMU_FW
C=$(emu_py sym "$MAP" canary-st:canary) || emu_die "no symbol canary"
# cw FIELD: a word of struct canary
cw() {
    _cw_i=0
    for _cw_f in magic state at passes bad bad_val ticks; do
        [ "$_cw_f" = "$1" ] && break
        _cw_i=$((_cw_i + 1))
    done
    emu_py peek "$EMU_SNAP" $((C + 4 * _cw_i)) 4 1
}

# 2. a session
emu_boot
emu_check "it runs" 1128353362 "$(cw magic)"
emu_run -s 3G
emu_check "filled, then checking with every word as written" "1 yes" "$(cw state) $([ "$(cw passes)" -ge 1 ] && echo yes)"
P0=$(cw passes)
emu_keys RECORD T2 RECORD PLAY w:2000 STOP TRIG TRIG TRIG
emu_keys PTN T2 w:500
emu_run -s 3G
emu_check "after keys, grid recording, PLAY, the TRIG pages and a pattern change: more passes, still OK" "1 yes" \
    "$(cw state) $([ "$(cw passes)" -gt "$P0" ] && echo yes)"
emu_shot ok

# 3. a word changed in the range
POKE=0x47900000
EMU_POKES="$POKE:4=0x12345678"
emu_run -s 3G
emu_check "a word poked into the range: BAD at its address" "2 $((POKE)) $((0x12345678))" "$(cw state) $(cw bad) $(cw bad_val)"
emu_shot bad
emu_done
