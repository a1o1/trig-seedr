# tools/emu.sh: emulator helpers for tools/tests/*.sh (POSIX sh, sourced). A test file starts
#     R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; emu_init <name>
# and then builds, boots, drives the mod through the Rytm's keys and knobs, and compares tracks.
#
# Everything a run writes lives under this checkout's build/emu/: the core-ar2 and mod builds, the
# patched OS, and per test file (<name>) its card, snapshots, screens and logs. Nothing is written
# into analog-emu/ or elekloader/, so suites in two worktrees run side by side (plan task 0.5).
#
# State: EMU_SNAP is the current snapshot and EMU_CLK its clock. Every action runs dtboot from it
# and makes the next snapshot; clocks in actions are relative to EMU_CLK (tools/emu.py plan).
# The mod's state (page, knobs, open) is read from the snapshot by symbol (the patched OS's
# .map.json), so navigation and knob settings are checked, not timed.
#
# sh has no local variables: each function's own variables start with _<function>_.
#
# Variables (docs/dev-environment.md section 4): WORKSPACE, B, STOCK, ELEKLOADER_CROSS. Defaults
# work in the main checkout and in a worktree, on the Mac and in the Linux shell.

: "${R:?set R to the repository before sourcing tools/emu.sh}"
case $R in
*/.claude/worktrees/*) E=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) E=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
T=$R/tools
STOCK=${STOCK:-"$E/Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx"}
if [ -z "$B" ]; then
    if [ "$(uname -s)" = Darwin ]; then B=$E/analog-emu/target/release; else B=${CARGO_TARGET_DIR:-$E/.toolchains/target-linux}/release; fi
fi
ELEKLOADER_CROSS=${ELEKLOADER_CROSS:-m68k-elf-}
export ELEKLOADER_CROSS PYTHONDONTWRITEBYTECODE=1 DTPEEK=$B/dtpeek
W=$R/build/emu
EMU_PASS=0 EMU_FAIL=0 EMU_POKES= EMU_QUIET= EMU_NAME=emu

emu_py() { python3 "$T/emu.py" "$@"; }

# emu_die MESSAGE: the test file stops (a failure inside $(...) only ends that subshell, and the
# caller's next check fails)
emu_die() { echo "FAIL $EMU_NAME: $*" >&2; echo "$EMU_NAME: stopped after $EMU_PASS passed, $((EMU_FAIL + 1)) failed" >&2; exit 1; }

# ---- locks: a folder made atomically; a lock whose owner has gone is taken over ----
emu_lock() {
    while ! mkdir "$1.lock" 2>/dev/null; do
        _lock_pid=$(cat "$1.lock/pid" 2>/dev/null)
        if [ -n "$_lock_pid" ] && ! kill -0 "$_lock_pid" 2>/dev/null; then rm -rf "$1.lock"; continue; fi
        sleep 1
    done
    echo $$ > "$1.lock/pid"
}
emu_unlock() { rm -rf "$1.lock"; }

# emu_init NAME: this test file's folder, emptied
emu_init() {
    EMU_NAME=$1 D=$W/$1 EMU_N=0
    rm -rf "$D"; mkdir -p "$D" "$W/fix"
    for _init_f in "$B/dtboot" "$B/dtpeek" "$B/dtcard" "$STOCK"; do
        [ -f "$_init_f" ] || emu_die "$_init_f is missing (sh tools/doctor.sh)"
    done
}

# ---- building: core-ar2 and the mod into build/emu, then the patched OS ----

# extra compiler flags for core-ar2: gcc 15 and later default to C23, which core-ar2 0.13's
# method calls through "()" pointers do not compile under (docs/status/0.5.md)
emu_core_cflags() {
    _cf_v=$(${ELEKLOADER_CROSS}gcc -dumpversion 2>/dev/null | cut -d. -f1)
    if [ "${_cf_v:-0}" -ge 15 ]; then echo '-std=gnu17'; fi
    echo '-fno-store-merging'               # gcc 16.2's ColdFire miscompile (lib/Makefile's note)
}

# emu_elebuild MODDIR OUT [CFLAG...]: elekloader's build with an output folder and extra cflags
emu_elebuild() {
    PYTHONPATH=$E/elekloader python3 - "$STOCK" "$@" <<'PY'
import sys
from elekloader.sdk import build
stock, m, out, cflags = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
try:
    path, mod = build.build(m, stock, out, extra={'cflags': cflags} if cflags else None)
except (build.BuildError, OSError, KeyError, ValueError) as e:
    print('BUILD FAILED: %s' % e); sys.exit(1)
print('BUILT %s: .run %d, .fast %d, .bss %d bytes' % (path, mod.size('.run'), mod.size('.fast'), mod.size('.bss')))
PY
}

# emu_stale FILE DIR...: true when FILE is missing or a file in the folders is newer
emu_stale() {
    [ -f "$1" ] || return 0
    _stale_f=$1; shift
    [ -n "$(find "$@" -newer "$_stale_f" -type f ! -path '*/out/*' ! -name '*.pyc' 2>/dev/null | head -1)" ]
}

# emu_build [VARIANT [CFLAG...]]: FW and MAP for mods/seedr-ar2 built with these cflags (a test
# build: emu_build norows -DRT_ROWS_MAX=0). Each variant is built once per checkout and change.
emu_build() {
    _build_v=${1:-seedr}; [ $# -gt 0 ] && shift
    _build_core=$W/core
    mkdir -p "$W"; emu_lock "$_build_core"
    if emu_stale "$_build_core/stamp" "$E/elekloader/mods/core-ar2" "$E/elekloader/mods/core" "$E/elekloader/elekloader" ||
       [ "$(emu_core_cflags | tr '\n' ' ')" != "$(cat "$_build_core/stamp" 2>/dev/null)" ]; then
        rm -rf "$_build_core"; mkdir -p "$_build_core"
        # shellcheck disable=SC2046
        if emu_elebuild "$E/elekloader/mods/core-ar2" "$_build_core" $(emu_core_cflags) > "$_build_core/build.log" 2>&1; then
            emu_core_cflags | tr '\n' ' ' > "$_build_core/stamp"
        else
            emu_unlock "$_build_core"; cat "$_build_core/build.log" >&2; emu_die "core-ar2 build"
        fi
    fi
    emu_unlock "$_build_core"
    CORE=$(ls "$_build_core"/core-*.elemod)
    FW=$W/$_build_v.syx MAP=$W/$_build_v.syx.map.json
    _build_m=$W/mod-$_build_v
    emu_lock "$_build_m"
    if emu_stale "$FW" "$R/mods/seedr-ar2" "$R/lib" "$R/device" || [ "$_build_core/stamp" -nt "$FW" ] ||
       [ "$*" != "$(cat "$_build_m/cflags" 2>/dev/null)" ]; then
        rm -rf "$_build_m"; mkdir -p "$_build_m"; echo "$*" > "$_build_m/cflags"
        if emu_elebuild "$R/mods/seedr-ar2" "$_build_m" "$@" > "$_build_m/build.log" 2>&1 &&
           (cd "$E/elekloader" && PYTHONPATH=$E/elekloader python3 -m elekloader.patch --stock "$STOCK" \
               --mod "$CORE" --mod "$(ls "$_build_m"/*.elemod)" --out "$FW" --lockfile "$W/$_build_v.lock.json" \
               --version "${EMU_VERSION:-GR01}") >> "$_build_m/build.log" 2>&1; then :
        else
            rm -f "$FW"; emu_unlock "$_build_m"; cat "$_build_m/build.log" >&2; emu_die "build $_build_v"
        fi
    fi
    emu_unlock "$_build_m"
    export DTEMU_FW=$FW
    [ -n "$EMU_QUIET" ] || grep '^BUILT' "$_build_m/build.log" | tail -1 | sed "s|^BUILT $R/|  built |"
    EMU_PROFILE=v1
}

# emu_has SYMBOL: the build has it
emu_has() { python3 -c "import json,sys; m=json.load(open(sys.argv[1])); sys.exit(not (sys.argv[2] in m or 'seedr:'+sys.argv[2] in m))" "$MAP" "$1"; }

# ---- running ----

# emu_next: EMU_S is the next snapshot's name (not in $(...): the counter must advance here)
emu_next() { EMU_N=$((EMU_N + 1)); EMU_S=$(printf '%s/s%03d' "$D" $EMU_N); }

emu_at() { EMU_SNAP=$1; EMU_CLK=$(cat "$1.clk"); }

# emu_boot [CARD]: a cold boot on a fresh card (or on CARD, as emu_power_off left it); the FILE
# SYSTEM OK dialogs closed with NO
emu_boot() {
    if [ -n "$1" ]; then EMU_CARD=$1
    else
        EMU_CARD=$D/card.img; rm -f "$EMU_CARD"
        "$B/dtcard" "$EMU_CARD" format > /dev/null || emu_die "dtcard format"
    fi
    emu_next; _boot_s=$EMU_S
    "$B/dtboot" "$FW" --instr 5.6G --card "$EMU_CARD" --press NO@4.2G --press NO@4.6G --press NO@5.0G \
        --press NO@5.4G --save "$_boot_s.snap" > "$_boot_s.log" 2>&1 || { tail -5 "$_boot_s.log" >&2; emu_die "boot"; }
    echo 5600000000 > "$_boot_s.snap.clk"
    emu_at "$_boot_s.snap"
}

# emu_run [-s SETTLE] DTBOOT-ARG...: run from EMU_SNAP, with @+REL clocks, until SETTLE (default
# 400M) after the last one. Pokes queued with emu_poke go with it.
emu_run() {
    _run_settle=400M
    if [ "$1" = -s ]; then _run_settle=$2; shift 2; fi
    _run_p=$(emu_py plan "$EMU_CLK" "$_run_settle" "$@") || emu_die "plan $*"
    _run_instr=$(echo "$_run_p" | head -1)
    _run_args=$(echo "$_run_p" | tail -n +2)
    emu_next; _run_s=$EMU_S _run_k=
    # shellcheck disable=SC2086
    [ -n "$EMU_POKES" ] && _run_k=$(emu_py pokes "$EMU_SNAP" $EMU_POKES)
    # shellcheck disable=SC2086
    "$B/dtboot" "$FW" --load "$EMU_SNAP" --instr "$_run_instr" $_run_k $_run_args --save "$_run_s.snap" \
        > "$_run_s.log" 2>&1 || { tail -5 "$_run_s.log" >&2; emu_die "dtboot $*"; }
    echo $((EMU_CLK + _run_instr)) > "$_run_s.snap.clk"
    echo "$(basename "$_run_s") $EMU_CLK +$_run_instr $_run_k $*" >> "$D/actions.log"
    EMU_POKES=
    emu_at "$_run_s.snap"
}

# emu_power_off: the power switched off 600M from now (the OS saves its working state to the
# card); EMU_CARD is then a copy of the card as the OS left it, for emu_boot
emu_power_off() {
    cp "$EMU_CARD" "$D/power-off.img"; EMU_CARD=$D/power-off.img
    "$B/dtboot" "$FW" --load "$EMU_SNAP" --card "$EMU_CARD" --instr 1.2G --power-off $((EMU_CLK + 600000000)) \
        > "$D/power-off.log" 2>&1 || { tail -5 "$D/power-off.log" >&2; emu_die "power off"; }
}

# emu_keys TOKEN...: key presses 150M apart. KEY; KEY:HOLD; F+KEY (FUNC held 400M, KEY 150M into
# it); H+KEY (H held, KEY 150M into it); w:N (wait N M more)
emu_keys() {
    _keys_t=100 _keys_a=
    for _keys_k in "$@"; do
        case $_keys_k in
        w:*) _keys_t=$((_keys_t + ${_keys_k#w:})); continue ;;
        F+*) _keys_a="$_keys_a --press FUNC@+${_keys_t}M:400M --press ${_keys_k#F+}@+$((_keys_t + 150))M"
             _keys_t=$((_keys_t + 400)) ;;
        *+*) _keys_a="$_keys_a --press ${_keys_k%%+*}@+${_keys_t}M:400M --press ${_keys_k#*+}@+$((_keys_t + 150))M"
             _keys_t=$((_keys_t + 400)) ;;
        *) _keys_a="$_keys_a --press $_keys_k@+${_keys_t}M" ;;
        esac
        _keys_t=$((_keys_t + 150))
    done
    # shellcheck disable=SC2086
    emu_run $_keys_a
}

# ---- the mod's state, by symbol ----

# emu_addr NAME[[INDEX]][.FIELD]: a symbol's address, or a field's in the struct the profile says
# it is; INDEX: an element of an array of those structs (recipe[3].vel.lo)
emu_addr() {
    _addr_s=${1%%.*} _addr_i=0
    case $_addr_s in *\[*\]) _addr_i=${_addr_s#*[}; _addr_i=${_addr_i%]}; _addr_s=${_addr_s%%\[*} ;; esac
    _addr_a=$(emu_py sym "$MAP" "$_addr_s") || emu_die "no symbol $_addr_s"
    if [ "$_addr_i" != 0 ]; then
        _addr_z=$(emu_field "$(emu_struct "$_addr_s")" sizeof) || emu_die "no struct for $_addr_s"
        _addr_a=$(printf '0x%08x' $((_addr_a + _addr_i * ${_addr_z% *})))
    fi
    case $1 in *.*)
        _addr_o=$(emu_field "$(emu_struct "$_addr_s")" "${1#*.}") || emu_die "no field $1"
        _addr_a=$(printf '0x%08x' $((_addr_a + ${_addr_o% *}))) ;;
    esac
    echo "$_addr_a"
}

# emu_field HEADER:STRUCT FIELD -> "offset size", from the cross compiler; cached per checkout.
# FIELD sizeof: the struct's size.
emu_field() {
    _field_c=$W/layout-$(echo "$1" | tr '/:' '__')
    _field_l() { awk -v k="$2" '$1 == k { print $2, $3; exit }' "$_field_c" 2>/dev/null; }
    if [ -z "$(_field_l "$@")" ]; then
        if [ "$2" = sizeof ]; then
            emu_py layout "${ELEKLOADER_CROSS}gcc" "$R/${1%%:*}" "${1#*:}" | sed 's/$/ 0/' >> "$_field_c" || return 1
        else
            emu_py layout "${ELEKLOADER_CROSS}gcc" "$R/${1%%:*}" "${1#*:}" "$2" | head -1 >> "$_field_c" || return 1
        fi
    fi
    [ -n "$(_field_l "$@")" ] || return 1
    _field_l "$@"
}

# emu_size NAME[[INDEX]][.FIELD]: the field's size; 4 for a plain symbol (an int)
emu_size() {
    case $1 in
    *.*) _size_s=${1%%.*}; emu_field "$(emu_struct "${_size_s%%\[*}")" "${1#*.}" | cut -d' ' -f2 ;;
    *) echo 4 ;;
    esac
}

# emu_get NAME[.FIELD]: its value in EMU_SNAP (signed)
emu_get() { emu_py peek "$EMU_SNAP" "$(emu_addr "$1")" "$(emu_size "$1")" 1 -s; }

# emu_poke NAME[.FIELD] VALUE: written before the next run. All the run's writes are merged into
# 32-bit words (dtboot --poke) together when it starts, so fields that share a word keep each
# other's bytes.
emu_poke() { EMU_POKES="$EMU_POKES $(emu_addr "$1"):$(emu_size "$1")=$2"; }

# seedref ARG...: tools/seedref's output (its usage: tools/seedref.c)
seedref() {
    # shellcheck disable=SC2046
    emu_cc seedref "$T/seedref.c" $(ls "$R"/lib/*.c | grep -v '/test_')
    "$W/seedref" "$@"
}

# emu_recipe TRACK KEY=VALUE...: writes a whole recipe (the defaults, SOURCE = TRACK, with those
# settings: seedref's keys) into the mod's recipe store for TRACK before the next run, and marks
# the track's recipe as set
emu_recipe() {
    _rec_t=$1; shift
    _rec_f=$(seedref --recipe "$@") || emu_die "recipe: $*"
    _rec_ifs=$IFS; IFS='
'
    for _rec_l in $_rec_f; do
        IFS=$_rec_ifs
        emu_poke "recipe[$_rec_t].${_rec_l% *}" "${_rec_l##* }"
    done
    IFS=$_rec_ifs
    if emu_has recipe_set; then
        emu_poke recipe_set $(($(emu_get recipe_set) | (1 << _rec_t)))
    fi
}

# ---- profiles: what the harness knows of a mod build. v1 is the version 1 framework (tasks 0.3,
# 0.6): LEFT / RIGHT, the recipe store recipe[] and recipe_set, open, page, tool. (Release 0.3's
# profile, r03, was removed with lib/gen.c at gate G0.) ----

emu_struct() {
    case $EMU_PROFILE:$1 in
    v1:recipe) echo lib/seed.h:seed_recipe ;;
    v1:tool) echo lib/seed.h:seed_tool ;;
    v1:shared) echo lib/seed.h:seed_shared ;;
    *) echo "?" ;;
    esac
}

emu_pages() {
    case $EMU_PROFILE in
    v1) echo TAKE RHYTHM PLACE DYNAMICS NOTE LOCK TRIG TIME VIEW TOOLS ;;
    esac
}

# emu_knob_of PAGE KNOB -> "encoder target [F|-] [one]": the knob's encoder (0..7 = A..H), the
# value it sets, F when FUNC is held for it (a range's low end), one: turned a step at a time
# (its value does not move by 1 a step)
emu_knob_of() {
    case $EMU_PROFILE:$1:$2 in
    v1:TOOLS:STEPS) echo 1 tool.steps ;;
    v1:TOOLS:TOOL) echo 0 tool.tool - one ;;
    v1:TOOLS:WILD) echo 1 tool.wild ;;
    v1:TOOLS:ROLL) echo 2 tool.roll ;;
    # v1: KNOB is LETTER:KEY, the knob and the seedref key it sets (emu_knob RHYTHM B:dens 58);
    # a .lo key is the low end, FUNC + knob. Enum and grid knobs: LETTER:KEY:one. LETTER:shared.FIELD:
    # a knob that sets the pattern's shared settings (struct seed_shared: emu_knob RHYTHM D:shared.map_x 100).
    v1:*:[A-H]:shared.*) echo "$(($(printf '%d' "'${2%%:*}") - 65)) ${2#*:} -" ;;
    v1:*:[A-H]:*)
        _ko_l=${2%%:*} _ko_k=${2#*:} _ko_one=
        case $_ko_k in *:one) _ko_k=${_ko_k%:one} _ko_one=one ;; esac
        _ko_p=$(seedref --fields "$_ko_k=0" | head -1 | cut -d' ' -f1)
        [ -n "$_ko_p" ] || return 1
        _ko_f=-; case $_ko_k in *.lo) _ko_f=F ;; esac
        echo "$(($(printf '%d' "'$_ko_l") - 65)) recipe[${EMU_TRACK:-0}].$_ko_p $_ko_f $_ko_one" ;;
    *) return 1 ;;
    esac
}

# emu_open: TRIG held + PAGE (the shortcut, from any page)
emu_open() {
    emu_run --press TRIG@+100M:400M --press PAGE@+250M
    [ "$(emu_get open)" = 1 ] || emu_die "open: the mod did not open"
}

# emu_page NAME: RIGHT until the mod's page is NAME
emu_page() {
    _page_i=0 _page_want=
    for _page_p in $(emu_pages); do
        [ "$_page_p" = "$1" ] && _page_want=$_page_i
        _page_i=$((_page_i + 1))
    done
    [ -n "$_page_want" ] || emu_die "page $1: not a page of profile $EMU_PROFILE"
    _page_k=$(((_page_want - $(emu_get page) + _page_i) % _page_i))
    [ $_page_k -eq 0 ] && return 0
    _page_key=RIGHT
    _page_a= _page_t=100
    while [ $_page_k -gt 0 ]; do
        _page_a="$_page_a --press $_page_key@+${_page_t}M"
        _page_t=$((_page_t + 150)) _page_k=$((_page_k - 1))
    done
    # shellcheck disable=SC2086
    emu_run -s 200M $_page_a
    [ "$(emu_get page)" = "$_page_want" ] || emu_die "page $1: on page $(emu_get page), not $_page_want"
}

# emu_knob PAGE KNOB VALUE: turns the knob until its value is VALUE (PAGE must be showing)
emu_knob() {
    _knob_spec=$(emu_knob_of "$1" "$2") || emu_die "knob $1 $2: unknown to profile $EMU_PROFILE"
    # shellcheck disable=SC2086
    set -- "$1" "$2" "$3" $_knob_spec
    _knob_enc=$4 _knob_tgt=$5 _knob_f= _knob_one=${7:-} _knob_stuck=0 _knob_tries=0
    [ "${6:--}" = F ] && _knob_f=:F
    while :; do
        _knob_cur=$(emu_get "$_knob_tgt")
        [ "$_knob_cur" = "$3" ] && return 0
        _knob_tries=$((_knob_tries + 1))
        [ $_knob_tries -gt 24 ] && emu_die "knob $1 $2: $_knob_cur, wanted $3"
        _knob_d=$(($3 - _knob_cur)) _knob_dir=1
        [ $_knob_d -lt 0 ] && _knob_dir=-1 _knob_d=$((-_knob_d))
        # A new dtboot takes every knob as asleep and sends two wake-up turns first; the mod (its
        # state is in the snapshot) keeps a knob awake 2 s (400M) after its last turn, and would take
        # those as steps. So the turns start 600M in, when the mod has let the knob sleep too.
        if [ -n "$_knob_one" ]; then _knob_a="--nudge $_knob_enc:$_knob_dir$_knob_f@+600M"
        elif [ $_knob_d -ge 8 ] && [ -z "$_knob_f" ]; then _knob_a="--nudge $_knob_enc:$_knob_dir:8@+600Mx$((_knob_d / 8))/50M"
        else _knob_a="--nudge $_knob_enc:$_knob_dir$_knob_f@+600Mx$_knob_d/80M"
        fi
        # shellcheck disable=SC2086
        emu_run -s 300M $_knob_a
        if [ "$(emu_get "$_knob_tgt")" = "$_knob_cur" ]; then
            _knob_stuck=$((_knob_stuck + 1))
            [ $_knob_stuck -ge 2 ] && emu_die "knob $1 $2: stays at $_knob_cur, wanted $3"
        fi
    done
}

# emu_sknob ENCODER ADDRESS VALUE: turns a stock knob (0..7 = A..H, on the page stock shows) until
# the byte at ADDRESS is VALUE. --nudge wakes a sleeping knob with two 8-count turns, which on 1.74
# moves a stock value by 2 (a step of 1 then cannot be made), so the turns are raw encoder counts,
# a UI frame (10M) apart: one 8-count turn wakes the knob and its next 4 counts are taken with it,
# then each 4 counts is one step (measured in the emulator, task F2: N + 1 turns of 4 after the 8
# move exactly N). The byte is read back and turned again if needed, as emu_knob does.
emu_sknob() {
    _sknob_tries=0
    while :; do
        _sknob_cur=$(emu_py peek "$EMU_SNAP" "$2" 1)
        [ "$_sknob_cur" = "$3" ] && return 0
        _sknob_tries=$((_sknob_tries + 1))
        [ $_sknob_tries -gt 4 ] && emu_die "stock knob $1: $_sknob_cur, wanted $3"
        _sknob_d=$(($3 - _sknob_cur)) _sknob_s=1
        [ $_sknob_d -lt 0 ] && _sknob_s=-1 _sknob_d=$((-_sknob_d))
        emu_run -s 300M --turn "$1:$((8 * _sknob_s))@+600M" --turn "$1:$((4 * _sknob_s))@+610Mx$((_sknob_d + 1))/10M"
    done
}

# emu_rec 0|1: grid recording off / on (RECORD pressed only when it is not so already). Generating
# needs it (seedr.c: RECORD TO GENERATE); turning it off with a take pending does not apply the take.
emu_rec() {
    [ "$(emu_py peek "$EMU_SNAP" 0x417e320c 1)" = "$1" ] || emu_keys RECORD
    [ "$(emu_py peek "$EMU_SNAP" 0x417e320c 1)" = "$1" ] || emu_die "grid recording did not turn $1"
}
# emu_yes: YES with grid recording on (generate); emu_keep: FUNC + YES (apply)
emu_yes() { emu_rec 1; emu_keys YES; }
emu_no() { emu_keys NO; }
emu_keep() { emu_keys F+YES; }
# emu_msg: the mod's message on the strip (2 s), or nothing
emu_msg() {
    _msg_p=$(emu_get msg)
    [ "$_msg_p" = 0 ] && return 0
    emu_py peek "$EMU_SNAP" "$(printf '0x%x' $((_msg_p & 0xffffffff)))" 1 24 | python3 -c \
        "import sys; b=bytes(int(x) for x in sys.stdin.read().split()); print(b.split(b'\\0')[0].decode())"
}

# ---- results ----

# emu_dump [TRACK [LENGTH]]: the track in seedref's dump format (LENGTH: default the track's)
emu_dump() { emu_py dump "$EMU_SNAP" "${1:-0}" "${2:-0}"; }
# emu_cmp LINE [LENGTH [TRACK]]: a line of release 0.3's comparison format (tools/emucmp.py, as
# 0.3's genref printed it; 03-save and 10-pages use it): 1 trigs and velocities, 2 flag words, 3 µtimes, 4 sample locks, 5 the mark
emu_cmp() { python3 "$T/emucmp.py" "$DTPEEK" "$FW" "$EMU_SNAP" "${2:-16}" "${3:-0}" | sed -n "${1}p"; }

# emu_shot NAME: the screen as build/emu/<test>/NAME.png
emu_shot() { "$DTPEEK" "$EMU_SNAP" fb | python3 "$T/fb2png.py" "$D/$1.png" 4 > /dev/null; }

# emu_check LABEL WANT GOT: PASS, or FAIL with both
emu_check() {
    if [ "$2" = "$3" ]; then
        EMU_PASS=$((EMU_PASS + 1))
        [ -n "$EMU_QUIET" ] || echo "PASS $EMU_NAME: $1"
    else
        EMU_FAIL=$((EMU_FAIL + 1))
        echo "FAIL $EMU_NAME: $1"
        echo "$2" | sed 's/^/  want /'
        echo "$3" | sed 's/^/  got  /'
    fi
}

# emu_done: the summary; the test file's exit status
emu_done() {
    echo "$EMU_NAME: $EMU_PASS passed, $EMU_FAIL failed"
    [ "$EMU_FAIL" -eq 0 ]
    exit $?
}

# ---- fixtures: a state several test files start from, made once per mod build ----

# emu_fixture NAME FUNCTION: EMU_SNAP becomes the fixture's snapshot and EMU_CARD its card, made
# by FUNCTION (which starts with emu_boot) unless this build has it already. FUNCTION's checks
# count, without PASS lines.
emu_fixture() {
    _fix_f=$W/fix/$1-$(cksum < "$FW" | cut -d' ' -f1)
    emu_lock "$_fix_f"
    if [ ! -f "$_fix_f.snap" ]; then
        _fix_q=$EMU_QUIET _fix_n=$EMU_NAME _fix_x=$EMU_FAIL
        EMU_QUIET=1 EMU_NAME="$_fix_n (fixture $1)"
        $2
        EMU_QUIET=$_fix_q EMU_NAME=$_fix_n
        if [ "$EMU_FAIL" -ne "$_fix_x" ]; then emu_unlock "$_fix_f"; emu_die "fixture $1 failed"; fi
        cp "$EMU_SNAP" "$_fix_f.snap"; cp "$EMU_SNAP.clk" "$_fix_f.snap.clk"; cp "$EMU_CARD" "$_fix_f.img"
    fi
    emu_unlock "$_fix_f"
    cp "$_fix_f.img" "$D/card.img"; EMU_CARD=$D/card.img
    emu_at "$_fix_f.snap"
}

# emu_fixture_save NAME: the current state is fixture NAME for this build (a test file that makes
# it with its own checks, as 01 makes "kept2")
emu_fixture_save() {
    [ "$EMU_FAIL" -eq 0 ] || return 0
    _fixs_f=$W/fix/$1-$(cksum < "$FW" | cut -d' ' -f1)
    emu_lock "$_fixs_f"
    cp "$EMU_SNAP" "$_fixs_f.snap"; cp "$EMU_SNAP.clk" "$_fixs_f.snap.clk"; cp "$EMU_CARD" "$_fixs_f.img"
    emu_unlock "$_fixs_f"
}

# emu_cc NAME SOURCE...: a host program in build/emu (tools/seedref), rebuilt when a
# source changes
emu_cc() {
    _cc_n=$1; shift
    emu_lock "$W/$_cc_n"
    if [ ! -x "$W/$_cc_n" ] || [ -n "$(find "$@" -newer "$W/$_cc_n" 2>/dev/null | head -1)" ] ||
       [ -n "$(find "$R/lib" "$R/device" "$R/mods/seedr-ar2" -name '*.h' -newer "$W/$_cc_n" | head -1)" ]; then
        cc -std=c99 -O2 -Wall -o "$W/$_cc_n" "$@" || { emu_unlock "$W/$_cc_n"; emu_die "cc $_cc_n"; }
    fi
    emu_unlock "$W/$_cc_n"
}
