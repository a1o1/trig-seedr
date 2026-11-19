#!/bin/sh
# Task S3b (docs/plan-syntakt.md): LOCK follows the track's machine (SD4) on the Syntakt OS 1.42 in analog-emu.
#   1. track 1's machine (BD MODERN, type 0) is read from its Sound; LOCK's lanes start on knob A (TUNE) and the
#      filter's FRQ
#   2. lane 1 on DEC (BD MODERN's, stock id 0x0a5) and lane 2 on FRQ at 100 %: YES writes their locks in the
#      lock slots stock's parameter table gives those ids
#   3. the machine changed to SY BITS (type 37, 8 controls) with stock's menu (FUNC + SYN): lane 2 stays on FRQ
#      (now one place later), lane 1's DEC is not SY BITS's: YES says SETTING OUT OF RANGE and writes nothing
#   4. lane 1 set again (PARM turned): YES writes again
# Needs ../analog-emu-st (status/S0.3.md; ST_B overrides). Writes only under build/emu/st-s3b/ and build/emu-st/.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init st-s3b
FW=$(sh "$T/st_build.sh" seedr "$R/mods/seedr-st" | tail -1) || emu_die "st_build.sh"
MAP=$FW.map.json DTEMU_FW=$FW EMU_PROFILE=v1
export DTEMU_FW T
peek() { python3 "$T/st_peek.py" "$@"; }
emu_rec() {
    [ "$(peek rec "$EMU_SNAP")" = "$1" ] || emu_keys RECORD
    [ "$(peek rec "$EMU_SNAP")" = "$1" ] || emu_die "grid recording did not turn $1"
}
# machine TRACK: the type the mod last read (st_machine_seen)
machine() { _m_a=$(emu_py sym "$MAP" st_machine_seen); emu_py peek "$EMU_SNAP" $((_m_a + $1)) 1 1 -s; }
# slot ID: the lock slot of stock parameter ID (its parameter table's entry, byte +7)
slot() { emu_py peek "$EMU_SNAP" $((0x4022d843 + 52 * $1)) 1; }
# locked SLOT: the steps of track 1 with a lock in SLOT, as "1 4 ..."
locked() { peek dump "$EMU_SNAP" 0 0 0 --lane "$1" | awk '$1 == "lane1" { for (i = 2; i <= NF; i++) if ($i != "-") s = s " " i - 1 } END { print substr(s, 2) }'; }
trigs() { peek dump "$EMU_SNAP" 0 | awk '$1 == "trig" { for (i = 2; i <= NF; i++) if ($i == "x") s = s " " i - 1 } END { print substr(s, 2) }'; }
DEC=0x0a5

# 1. BD MODERN on track 1
emu_boot
emu_keys TRIG TRIG TRIG
emu_check "seedr opens" 1 "$(emu_get open)"
emu_check "track 1's machine: BD MODERN (type 0)" 0 "$(machine 0)"
emu_page LOCK
emu_shot lock-bd-modern
emu_check "lane 1 starts on knob A (TUNE, place 0); lane 2 on FRQ (place 7 + 4)" "0 11" \
    "$(emu_get 'recipe[0].lane[0].parm') $(emu_get 'recipe[0].lane[1].parm')"

# 2. lane 1 on DEC (place 2), lane 2 on FRQ, both 100 %
emu_knob LOCK A:lane1.parm 2
emu_knob LOCK B:lane1.pct 100
emu_keys DOWN
emu_knob LOCK B:lane2.pct 100
emu_shot lock-lanes
emu_rec 1
emu_keys YES
emu_check "YES: a take pending" 1 "$(emu_get auditioning)"
T1=$(trigs)
emu_check "YES: the take has trigs" 1 "$([ -n "$T1" ] && echo 1)"
emu_check "YES: lane 1's DEC locks in DEC's slot, on every trig" "$T1" "$(locked "$(slot $DEC)")"
FRQ=$(sed -n 's/^#define ST_PAGE_FLTR {.*{ "FRQ", \(0x[0-9a-f]*\),.*/\1/p' "$R/device/st_params.h")
emu_check "YES: lane 2's FRQ locks in FRQ's slot, on every trig" "$T1" "$(locked "$(slot "$FRQ")")"
emu_keys NO
emu_check "NO: back to the original" 0 "$(emu_get auditioning)"
emu_keys NO
emu_check "NO again: seedr closed" 0 "$(emu_get open)"

# 3. SY BITS on track 1 (stock's machine menu: six places below BD MODERN)
emu_rec 0
emu_keys F+SYN DOWN DOWN DOWN DOWN DOWN DOWN YES
emu_keys NO NO SYN
emu_shot syn-sy-bits
emu_keys TRIG TRIG TRIG
emu_check "seedr opens again" 1 "$(emu_get open)"
emu_check "track 1's machine: SY BITS (type 37)" 37 "$(machine 0)"
emu_page LOCK
emu_check "lane 2 stays on FRQ (place 8 + 4); lane 1 stays where it was (2)" "2 12" \
    "$(emu_get 'recipe[0].lane[0].parm') $(emu_get 'recipe[0].lane[1].parm')"
emu_shot lock-sy-bits
REC0=$(peek dump "$EMU_SNAP" 0)
emu_rec 1
emu_keys YES
emu_check "YES, lane 1 on BD MODERN's DEC: SETTING OUT OF RANGE" "SETTING OUT OF RANGE" "$(emu_msg)"
emu_check "... nothing pending" 0 "$(emu_get auditioning)"
emu_check "... the track unchanged" "$REC0" "$(peek dump "$EMU_SNAP" 0)"

# 4. lane 1 set again: SY BITS's knob B (place 1)
emu_keys UP
emu_knob LOCK A:lane1.parm 1
emu_keys YES
emu_check "lane 1 set again: YES writes a take" 1 "$(emu_get auditioning)"
SB=$(python3 - "$R/device/st_params.h" <<'PY'
import re, sys
s = open(sys.argv[1]).read()
line = [l for l in s.splitlines() if '"SY BITS"' in l][0]
print(int(re.findall(r'\{ *"[^"]*", *(0x[0-9a-f]+)', line)[1], 16))
PY
)
emu_check "... lane 1's locks in SY BITS's knob B's slot" "$(trigs)" "$(locked "$(slot "$SB")")"
emu_done
