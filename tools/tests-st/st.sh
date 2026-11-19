# tools/tests-st/st.sh: the Syntakt's test files' helpers (docs/plan-syntakt.md S4; sourced after R is set):
#     R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; emu_init <name>; st_build
# tools/emu.sh and tools/emu_st.sh's helpers, on seedr for the Syntakt OS 1.42 (mods/seedr-st on the hook-bus
# core, tools/st_build.sh) in ../analog-emu-st (status/S0.3.md; ST_B overrides). What differs from the Rytm:
#   emu_dump      tools/st_peek.py dump --seed (lib/'s condition codes), lane 1 by ST_SLOT (a lock slot)
#   seedref       tools/seedref --device st, with the track's machine as seedr read it (st_machine)
#   emu_rec       grid recording as the Syntakt keeps it (st_peek.py rec)
#   st_mem        a track's record or lock store as hex, for byte-for-byte checks
#   st_slot       the lock slot of a lane's parameter on a track
case $R in
*/.claude/worktrees/*) _st_ws=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _st_ws=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_st_ws/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
export T
ST_SLOT=

# st_build: seedr for the Syntakt, built again only when a source changed; FW, MAP, DTEMU_FW, EMU_PROFILE
st_build() {
    FW=$R/build/emu-st/seedr-emu.syx MAP=$R/build/emu-st/seedr-emu.syx.map.json
    mkdir -p "$R/build/emu-st"; emu_lock "$R/build/emu-st/seedr"
    if emu_stale "$FW" "$R/mods/seedr-st" "$R/frame" "$R/lib" "$R/device" "$R/mods/seedr-ar2/grooves-swing.h"; then
        sh "$T/st_build.sh" seedr "$R/mods/seedr-st" > "$R/build/emu-st/seedr.build.log" 2>&1 ||
            { emu_unlock "$R/build/emu-st/seedr"; cat "$R/build/emu-st/seedr.build.log" >&2; emu_die "st_build.sh"; }
    fi
    emu_unlock "$R/build/emu-st/seedr"
    [ -f "$MAP" ] || emu_die "no map: $MAP"
    DTEMU_FW=$FW EMU_PROFILE=v1
    export DTEMU_FW
}

st_peek() { python3 "$T/st_peek.py" "$@"; }

# emu_dump [TRACK [LENGTH]]: the track in seedref's format (lane 1: the lock slot ST_SLOT, if set)
emu_dump() {
    if [ -n "$ST_SLOT" ]; then st_peek dump "$EMU_SNAP" "${1:-0}" "${2:-0}" --seed --lane "$ST_SLOT"
    else st_peek dump "$EMU_SNAP" "${1:-0}" "${2:-0}" --seed; fi
}

# st_machine TRACK: the track's machine type as seedr last read it (st_machine_seen), -1 none or not read yet
st_machine() {
    _m_a=$(emu_py sym "$MAP" st_machine_seen 2>/dev/null) || { echo -1; return; }
    emu_py peek "$EMU_SNAP" "$(printf '0x%08x' $((_m_a + $1)))" 1 1 -s
}

# seedref ARG...: --device st; with machine= the track's machine as seedr read it (track= in the arguments,
# else track 0), unless an argument names one
seedref() {
    # shellcheck disable=SC2046
    emu_cc seedref "$T/seedref.c" $(ls "$R"/lib/*.c | grep -v '/test_')
    _sr_t=0 _sr_m=
    for _sr_a in "$@"; do
        case $_sr_a in track=*) _sr_t=${_sr_a#track=} ;; machine=*) _sr_m=given ;; esac
    done
    if [ -z "$_sr_m" ] && [ -n "$EMU_SNAP" ] && [ -n "$MAP" ]; then
        "$W/seedref" --device st "$@" machine="$(st_machine "$_sr_t")"     # last: seedref reads its mode from the first
    else
        "$W/seedref" --device st "$@"
    fi
}

emu_rec() {
    [ "$(st_peek rec "$EMU_SNAP")" = "$1" ] || emu_keys RECORD
    [ "$(st_peek rec "$EMU_SNAP")" = "$1" ] || emu_die "grid recording did not turn $1"
}

# st_mem rec|locks [TRACK]: a track's record or lock store as hex (pattern A01)
st_mem() {
    python3 - "$EMU_SNAP" "$1" "${2:-0}" <<'PY'
import os, sys
sys.path.insert(0, os.environ['T'])
import st_peek as s, emu
snap, what, t = sys.argv[1], sys.argv[2], int(sys.argv[3])
if what == 'rec':
    a, n = s.record(snap, t, 0), 0x499
else:
    a, n = s.u32(snap, s.pattern(snap, 0) + s.LOCK_MODEL + 0x10) + s.LOCK_TRACK * t, s.LOCK_TRACK
print(emu.mem(snap, a, n).hex())
PY
}

# st_slot TRACK PARM: the lock slot of the track's parameter PARM (an index in its list, seedref --ids)
st_slot() {
    _slot_id=$(seedref --ids track="$1" | awk -v p="$2" '$1 == p { print $3 }')
    [ -n "$_slot_id" ] || emu_die "no parameter $2 on track $1"
    st_peek slot "$EMU_SNAP" "$_slot_id"
}

# st_open: seedr opened with TRIG held + PAGE (emu.sh's emu_open), checked
st_open() { emu_open; }

# emu_msg: the mod's message on the strip (40 bytes: EUCLID IS ON: TURN IT OFF is 25)
emu_msg() {
    _msg_p=$(emu_get msg)
    [ "$_msg_p" = 0 ] && return 0
    emu_py peek "$EMU_SNAP" "$(printf '0x%x' $((_msg_p & 0xffffffff)))" 1 40 | python3 -c \
        "import sys; b=bytes(int(x) for x in sys.stdin.read().split()); print(b.split(b'\\0')[0].decode())"
}

# st_rec_addr TRACK: the address of a track's record (pattern A01), for stock knobs (emu_sknob)
st_rec_addr() {
    python3 -c "import os, sys; sys.path.insert(0, os.environ['T']); import st_peek as s
print('0x%08x' % s.record(sys.argv[1], int(sys.argv[2]), 0))" "$EMU_SNAP" "$1"
}

# st_tracks: every track's record and lock store, as one digest
st_tracks() {
    for _tr_t in 0 1 2 3 4 5 6 7 8 9 10 11 12; do st_mem rec $_tr_t; st_mem locks $_tr_t; done | shasum | cut -d' ' -f1
}

# emu_recipe TRACK KEY=VALUE...: emu.sh's, with the track (and so its machine's list and lane ranges) given to
# seedref. seedr reads a track's machine when it first asks for the track (st_machine_sync): open and close it
# on the track first (st_seen), or the lanes' indexes are moved to the machine's list.
emu_recipe() {
    _rec_t=$1; shift
    _rec_f=$(seedref --recipe track="$_rec_t" "$@") || emu_die "recipe: $*"
    _rec_ifs=$IFS; IFS='
'
    for _rec_l in $_rec_f; do
        IFS=$_rec_ifs
        emu_poke "recipe[$_rec_t].${_rec_l% *}" "${_rec_l##* }"
    done
    IFS=$_rec_ifs
    emu_poke recipe_set $(($(emu_get recipe_set) | (1 << _rec_t)))
}

# st_seen: seedr opened and closed on the current track, so it has read the track's machine
st_seen() {
    emu_open
    [ "$(emu_get auditioning)" = 0 ] || emu_die "st_seen: a take pending"
    emu_no
}

# st_parm TRACK NAME: the index of a parameter in the track's list (its machine's as seedr read it)
st_parm() { seedref --ids track="$1" | awk -v n="$2" '$2 == n { print $1; exit }'; }
