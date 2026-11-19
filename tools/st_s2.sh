#!/bin/sh
# Task S2 (docs/plan-syntakt.md): the Syntakt device layer (device/syntakt_st.h) on the stock OS 1.42 in analog-emu,
# through a test mod (mods/devtest-st, built with tools/st_build.sh --core mini) that runs a poked command each tick.
#   1. attach: the layer's memory from the native heap
#   2. snapshot track 1 of A01, then write a known take through stock's routines: every step one later, step 3's
#      NOTE 64, VEL 90, micro timing +5, condition 1:2, a lock of FRQ (slot 28) 100 on step 2, none on step 5.
#      tools/st_peek.py reads the track as the take says, the other tracks untouched
#   3. restore: the record and the lock store byte for byte as before the snapshot
# Whether the sequencer plays what the layer writes is not checked here (status/S2.md, open points).
# Needs ../analog-emu-st (status/S0.3.md; ST_B overrides). Writes only under build/emu/st-s2/ and build/emu-st/.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init st-s2
FW=$(sh "$T/st_build.sh" devtest "$R/mods/devtest-st" | tail -1) || emu_die "st_build.sh"
DTEMU_FW=$FW MAP=$FW.map.json
export DTEMU_FW
DT=$(emu_py sym "$MAP" devtest-st:dt) || emu_die "no symbol dt"
peek() { python3 "$T/st_peek.py" "$@"; }
# dt FIELD: a word of the test mod's struct devtest
dt() {
    _dt_i=0
    for _dt_f in magic cmd arg res mem ticks track lane_slot; do
        [ "$_dt_f" = "$1" ] && break
        _dt_i=$((_dt_i + 1))
    done
    emu_py peek "$EMU_SNAP" $((DT + 4 * _dt_i)) 4 1 -s
}
# cmd N: poke dt.cmd, run until the mod's tick has taken it
cmd() {
    EMU_POKES="$(printf '0x%08x' $((DT + 4))):4=$1"
    emu_run -s 300M
    [ "$(dt cmd)" = 0 ] || emu_die "command $1 not taken"
}
# mem WHAT [TRACK]: a track's bytes as hex: rec (the record), locks (its lock store)
mem() {
    python3 - "$EMU_SNAP" "$1" "${2:-0}" <<'PY'
import sys; sys.path.insert(0, sys.path[0] or '.')
sys.path.insert(0, __import__('os').environ['T'])
import st_peek as s, emu
snap, what, t = sys.argv[1], sys.argv[2], int(sys.argv[3])
if what == 'rec':
    a, n = s.record(snap, t, 0), 0x499
else:
    a, n = s.u32(snap, s.pattern(snap, 0) + s.LOCK_MODEL + 0x10) + s.LOCK_TRACK * t, s.LOCK_TRACK
print(emu.mem(snap, a, n).hex())
PY
}
export T

# 1. boot, attach
emu_boot
emu_check "the test mod runs" 1146377044 "$(dt magic)"
cmd 1
emu_check "attach: memory from the native heap" "0 1" "$(dt res) $([ "$(dt mem)" -ge 1073741824 ] && echo 1)"
REC0=$(mem rec) LKS0=$(mem locks) OTHER0=$(mem rec 1)$(mem locks 1)
DUMP0=$(peek dump "$EMU_SNAP" 0 0 0 --lane 28)

# 2. snapshot, the known take
cmd 2
emu_check "snapshot" 0 "$(dt res)"
cmd 3
emu_check "write: SEED_OK, on track 1, FRQ in slot 28" "0 0 28" "$(dt res) $(dt track) $(dt lane_slot)"
emu_check "write: steps changed" 1 "$([ "$(dt arg)" -gt 0 ] && echo 1)"
emu_shot take
WANT=$(echo "$DUMP0" | python3 -c "
import sys
rows = dict((l.split()[0], l.split()[1:]) for l in sys.stdin.read().splitlines())
n = int(rows['length'][0]); out = []
for k in ('length', 'trig', 'flags', 'note', 'vel', 'len', 'utime', 'cond', 'rlen', 'rrate', 'rvel', 'lane1'):
    v = rows[k]
    if k not in ('length', 'flags'):
        v = [v[(i - 1) % n] for i in range(n)]
    if k == 'flags':                     # swing (0x10) stays with the position, the rest moves
        f = [int(x, 16) for x in v]
        v = ['%02x' % ((f[(i - 1) % n] & ~0x10) | (f[i] & 0x10)) for i in range(n)]
    for i, x in {'note': {2: '64'}, 'vel': {2: '90'}, 'utime': {2: '5'}, 'cond': {2: '32'},
                 'lane1': {1: '100', 4: '-'}}.get(k, {}).items():
        v[i] = x
    out.append(k + ' ' + ' '.join(v))
print('\n'.join(out))")
emu_check "the track reads as the take" "$WANT" "$(peek dump "$EMU_SNAP" 0 0 0 --lane 28)"
emu_check "the other tracks untouched (track 2's record and locks)" "$OTHER0" "$(mem rec 1)$(mem locks 1)"

# 3. restore
cmd 4
emu_check "restore: the record byte for byte" "$REC0" "$(mem rec)"
emu_check "restore: the lock store byte for byte" "$LKS0" "$(mem locks)"
cmd 5
emu_check "drop" 0 "$(dt res)"
emu_done
