#!/bin/sh
# Task F10, the preview strip and VIEW, in analog-emu. The preview (mods/seedr-ar2/preview.c) is the
# take as YES would write it: its buffer pv_out is compared with seedref and with what YES then
# writes, and the strip on the screen with pv_out. It renders once per knob change, before YES,
# from the original during an audition, and again when stock's keys change the track; an idle
# screen renders nothing. Then the cost of a render, from the instructions the emulator executes,
# on a 64-step track with most layers on and on a 16-step RANDOM track, and of seedr's idle screen
# (build/emu/30-preview/cost.txt).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 30-preview
emu_build

# ---- helpers ----

# the layout of struct seed_track and seed_step, from the cross compiler, as NAME=OFFSET
pv_layout() {
    for _l_f in length default_vel s; do echo "t.$_l_f=$(emu_field lib/seed.h:seed_track $_l_f | cut -d' ' -f1)"; done
    for _l_f in flags note vel len utime cond rlen rrate rvel lane; do
        echo "s.$_l_f=$(emu_field lib/seed.h:seed_step $_l_f | cut -d' ' -f1)"
    done
    echo "s.sizeof=$(emu_field lib/seed.h:seed_step sizeof | cut -d' ' -f1)"
}
PV_LAYOUT=$(pv_layout | tr '\n' ' ')

pv_py() {
    python3 - "$@" <<'PY'
import os, subprocess, sys

def mem(snap, addr, n):
    out = subprocess.run([os.environ['DTPEEK'], snap, 'mem', '%x' % addr, str(n)], capture_output=True, text=True,
                         check=True).stdout
    b = []
    for line in out.splitlines():
        if ':' in line:
            b += [int(x, 16) for x in line.split(':', 1)[1].split()]
    return bytes(b[:n])

def track(snap, addr, lay):
    """a struct seed_track in the mod's memory: (length, default_vel, steps as dicts)"""
    z = lay['s.sizeof']
    raw = mem(snap, addr, lay['t.s'] + 64 * z)
    be = lambda b, s: int.from_bytes(b, 'big', signed=s)
    n = be(raw[lay['t.length']:lay['t.length'] + 2], True)
    steps = []
    for i in range(64):
        b = raw[lay['t.s'] + z * i:lay['t.s'] + z * (i + 1)]
        f = lambda k, w=1, s=False: be(b[lay['s.' + k]:lay['s.' + k] + w], s)
        steps.append(dict(flags=f('flags', 2), note=f('note'), vel=f('vel'), len=f('len'), utime=f('utime', 1, True),
                          cond=f('cond', 1, True), rlen=f('rlen'), rrate=f('rrate'), rvel=f('rvel', 1, True),
                          lane1=f('lane', 2, True)))
    return n, raw[lay['t.default_vel']], steps

def dump(n, steps):
    """seedref's format (tools/seedref.c)"""
    d = lambda v: '-' if v == 0xff else str(v)
    s = steps[:n]
    rows = [('trig', ['x' if x['flags'] & 1 else '.' for x in s]), ('flags', ['%02x' % (x['flags'] & 0x3f) for x in s]),
            ('note', [d(x['note']) for x in s]), ('vel', [d(x['vel']) for x in s]), ('len', [d(x['len']) for x in s]),
            ('utime', [str(x['utime']) for x in s]), ('cond', ['-' if x['cond'] < 0 else str(x['cond']) for x in s]),
            ('rlen', [str(x['rlen']) for x in s]), ('rrate', [str(x['rrate']) for x in s]),
            ('rvel', [str(x['rvel']) for x in s]), ('lane1', ['-' if x['lane1'] == -32768 else str(x['lane1']) for x in s])]
    return 'length %d\n' % n + '\n'.join(k + ' ' + ' '.join(v) for k, v in rows)

def screen(snap):
    out = subprocess.run([os.environ['DTPEEK'], snap, 'fb'], capture_output=True, text=True).stdout
    rows = []
    for line in out.splitlines():
        if line.strip('\n'):
            rows += [[c in '▀█' for c in line], [c in '▄█' for c in line]]
    return rows[:64]

def strip(snap, n, dv, steps, top=51, bottom=63):
    """two lines: the bar heights pv_out gives (preview.c's formula), and those on the screen"""
    want, got, px = [], [], screen(snap)
    for i in range(n):
        s = steps[i]
        if s['flags'] & 1:
            v = dv if s['vel'] == 0xff else s['vel']
            want.append(1 + max(1, min(127, v)) * (bottom - top - 1) // 127)
        else:
            want.append(0 if i & 3 else 1)
        h, x = 0, 2 * i
        while bottom - h >= top and x < len(px[bottom - h]) and px[bottom - h][x]:
            h += 1
        got.append(h)
    return ' '.join(map(str, want)) + '\n' + ' '.join(map(str, got))

a = sys.argv[1:]
if a[0] in ('dump', 'strip'):
    lay = {k: int(v) for k, v in (x.split('=') for x in a[3].split())}
    n, dv, steps = track(a[1], int(a[2], 0), lay)
    print(dump(n, steps) if a[0] == 'dump' else strip(a[1], n, dv, steps))
PY
}

pv_dump() { pv_py dump "$EMU_SNAP" "$(emu_addr ${1:-pv_out})" "$PV_LAYOUT"; }
pv_strip() { pv_py strip "$EMU_SNAP" "$(emu_addr pv_out)" "$PV_LAYOUT"; }
renders() { emu_get pv_renders; }
# check_preview LABEL SEEDREF-ARG...: pv_out is seedref's take, and the strip on the screen is pv_out
check_preview() {
    _cp_l=$1; shift
    emu_check "$_cp_l: the preview is seedref's take" "$(seedref "$@")" "$(pv_dump)"
    _cp_s=$(pv_strip)
    emu_check "$_cp_l: the strip draws it" "$(echo "$_cp_s" | sed -n 1p)" "$(echo "$_cp_s" | sed -n 2p)"
}

# ---- 1. on BD with seed 2 kept (RANDOM): the preview follows the knobs before YES ----
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
check_preview "open, TAKE" src=random seed=2 "$K"
emu_page RHYTHM
r0=$(renders)
emu_knob RHYTHM C:dens 57
emu_shot rhythm-dens57
check_preview "DENS 57, before YES" src=random seed=2 dens=57 "$K"
emu_check "DENS 57: a render for each value (seven turns, 50 -> 57)" 7 $(($(renders) - r0))
emu_check "DENS 57: the track is not written before YES" "$(cat "$D/kept2.txt")" "$(emu_dump)"

# ---- 2. YES writes what the preview showed; during the audition the preview renders from the original ----
emu_yes
check_take "YES: DENS 57" src=random seed=2 dens=57 "$K"
emu_check "YES: the track is the preview" "$(pv_dump)" "$(emu_dump)"
emu_knob RHYTHM C:dens 40
check_preview "auditioning, DENS 40: from the original, not the take" src=random seed=2 dens=40 "$K"
check_take "auditioning, DENS 40: the track keeps the take until YES" src=random seed=2 dens=57 "$K"
emu_shot rhythm-audition
r0=$(renders)
emu_no
emu_check "NO: back to the original" "$(cat "$D/kept2.txt")" "$(emu_dump)"
emu_check "NO: no render (the original and the recipe are the same)" 0 $(($(renders) - r0))
emu_keys w:600
emu_check "idle: no render" 0 $(($(renders) - r0))
check_preview "NO: the preview" src=random seed=2 dens=40 "$K"

# ---- 3. SOURCE = TRACK, and a trig recorded with stock's keys while seedr is open (V10) ----
emu_knob RHYTHM A:src:one 0
check_preview "SRC TRACK: the track itself" "$K"
r0=$(renders)
emu_rec 1
emu_keys T3
save_orig rec3
emu_check "RECORD, T3: step 3 changed" 1 "$([ "$(cat "$D/rec3.txt")" != "$(cat "$D/kept2.txt")" ] && echo 1)"
check_preview "RECORD, T3: the preview follows the track" "orig=$D/rec3.txt"
emu_check "RECORD, T3: rendered again" 1 "$([ $(($(renders) - r0)) -ge 1 ] && echo 1)"

# ---- 4. VIEW: the track's steps, the footer, no strip ----
emu_page VIEW
emu_shot view
emu_run --nudge 0:1@+600Mx2/80M
emu_shot view-cursor
emu_page TAKE

# ---- 5. a 64-step track with most layers on: the preview and the take ----
HEAVY="src=random seed=7 bias=-40 rot=3 skip=1 vel=40:120 shape=not1 amt=50 acc=30 hum=20 note=48:72
    lane1.pct=50 lane2.pct=50 cond=30 rtrg=20 rand=10"
# BD 64 steps long (record +0x2c5; +0x2c7, the speed, stays 2)
emu_run --poke 407ddba8=40000200
save_orig len64
# shellcheck disable=SC2086
emu_recipe 0 $HEAVY dens=60
emu_page RHYTHM
# shellcheck disable=SC2086
check_preview "64 steps, most layers" $HEAVY dens=60 "orig=$D/len64.txt"
emu_shot rhythm-64
emu_yes
# shellcheck disable=SC2086
check_take "64 steps: YES" $HEAVY dens=60 "orig=$D/len64.txt"
emu_check "64 steps: the track is the preview" "$(pv_dump)" "$(emu_dump)"
emu_page VIEW
emu_run --nudge 0:1@+600Mx2/80M
emu_shot view-64
emu_page RHYTHM
emu_no

# ---- 6. the cost. dtboot counts the instructions the CPU executes (the rest of a run's budget is
# idle, skipped). A run of 16 DENS turns, each a new value and a render, against the same turns
# with DENS held at 0 (no change, no render): the difference is the renders'. ----
executed() { sed -n 's/.*instructions (\([0-9]*\) executed.*/\1/p' "${EMU_SNAP%.snap}.log"; }
# cost LABEL RECIPE...: on BD as it is, with RECIPE and DENS
cost() {
    _c_l=$1; shift
    _c_s=$EMU_SNAP
    emu_recipe 0 "$@" dens=0
    emu_run -s 600M                                 # the recipe's own render, before the count
    _c_r=$(renders)
    emu_run -s 1700M --nudge 2:-1@+600Mx16/80M
    _c_b=$(executed) _c_rb=$(($(renders) - _c_r))
    emu_at "$_c_s"
    emu_recipe 0 "$@" dens=40
    emu_run -s 600M
    _c_r=$(renders)
    emu_run -s 1700M --nudge 2:1@+600Mx16/80M
    _c_t=$(executed) _c_rt=$(($(renders) - _c_r)) _c_d=$(($(emu_get 'recipe[0].dens') - 40))
    emu_check "cost, $_c_l: DENS held at 0, no render" 0 "$_c_rb"
    emu_check "cost, $_c_l: a render for each DENS value" "$_c_d" "$_c_rt"
    [ "$_c_rt" -gt 0 ] || return 0
    _c_e=$(((_c_t - _c_b) / _c_rt))
    echo "$_c_l: $_c_rt renders, $_c_e instructions each ($((_c_e / 200)) us at the emulator's 200 M a second);" \
        "the turns' run executed $_c_t, the run without renders $_c_b" >> "$D/cost.txt"
}
: > "$D/cost.txt"
# shellcheck disable=SC2086
cost "64 steps, most layers on" $HEAVY
# the worst case for value lists (F6's open point): NOTE and both lanes 16-item lists in RAND order at
# VARY 90, where every draw may walk back up to 64 takes
# shellcheck disable=SC2086
cost "64 steps, most layers, three 16-item lists, VARY 90" $HEAVY vary=90 note.src=list note.len=16 note.order=rand \
    lane1.pct=100 lane1=1:100 lane1.src=list lane1.len=16 lane1.order=rand \
    lane2.pct=100 lane2=0:127 lane2.src=list lane2.len=16 lane2.order=rand
emu_run --poke 407ddba8=10000200            # 16 steps
cost "16 steps, RANDOM only" src=random seed=7
# seedr's whole screen, idle on RHYTHM (no render: a compare per draw), against stock's TRIG page
emu_run -s 1000M
_i_o=$(executed)
emu_keys NO
emu_run -s 1000M
_i_c=$(executed)
echo "idle on RHYTHM, 1 s: $_i_o instructions executed; with seedr closed (stock's TRIG page): $_i_c;" \
    "seedr's screen costs $((_i_o - _i_c)) a second, $(((_i_o - _i_c) / 2000000)) % of 200 M" >> "$D/cost.txt"
sed 's/^/  /' "$D/cost.txt"
emu_done
