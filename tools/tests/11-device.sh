#!/bin/sh
# Task 0.4: the device layer's version 1 interface (device/rytm_ar2.h) in analog-emu, through a test mod
# (tools/tests/11-device.c: TEMPO moves the track one step, CHAIN auditions a take, SONG restores, FILL tries
# a lock no row can hold). On A01 BD:
#   1. a step with three parameter locks made by hand (GRID REC, T1 held, three FLTR knobs)
#   2. TEMPO: moved one step later, every field and all 44 lock values with it
#   3. SAVE PATTERN, TEMPO again, RELOAD PATTERN: the saved move, locks intact
#   4. power off and cold boot on that card: the same (I9)
#   5. CHAIN then SONG: back byte for byte, record and lock store (I7)
#   6. FILL: refused, nothing written (I8)
# Snapshots, card images and the build stay in this checkout's build/emu/11-device/; core-ar2 is the
# harness's build (tools/emu.sh emu_build).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"
emu_init 11-device
EMU_QUIET=1 emu_build
W=$D; M=$W/devtest; FW=$W/devtest.syx
mkdir -p $W
rm -f $W/*.rec $W/*.lks
rm -rf $M && mkdir -p $M && cp $R/tools/tests/11-device.c $M/devtest.c
cat > $M/mod.json <<EOF
{
 "id": "devtest", "version": "0.4", "title": "device layer test", "category": "Test",
 "license": "GPL-2.0-or-later", "author": "trig-seedr", "description": "Test build only (tools/tests/11-device.sh).",
 "device": "analog-rytm-mk2", "os": "1.74", "sources": ["devtest.c"], "cflags": ["-I$R/device"],
 "subscribe": [{"event": "ev_key", "fn": "dt_key", "order": 40}],
 "resources": {"core": "0.13"}, "requires": ["core"]
}
EOF
cd $E/elekloader
emu_elebuild $M $M/out > $W/devtest-build.log 2>&1 || { cat $W/devtest-build.log; emu_die "build devtest"; }
python3 -m elekloader.patch --stock "$STOCK" --mod "$CORE" --mod $M/out/devtest-0.4.elemod \
    --out $FW --lockfile $W/devtest.lock.json --version SDEV | tail -1
cd $W
run() { snap_in=$1; snap_out=$2; shift 2; $B/dtboot $FW --load $snap_in "$@" --save $snap_out > /dev/null; }
# BD's record and lock store as two hex files: SNAP -> SNAP.rec, SNAP.lks
dump() { python3 - $B/dtpeek $FW $1 <<'PY'
import os, subprocess, sys
dtpeek, fw, snap = sys.argv[1:4]
P = 0x407dd8e3                                   # live pattern object 1 = A01, track 0 (BD)
def mem(addr, n):
    out = subprocess.run([dtpeek, snap, 'mem', '%x' % addr, str(n)], env=dict(os.environ, DTEMU_FW=fw),
                         capture_output=True, text=True, check=True).stdout
    b = []
    for line in out.splitlines():
        if ':' in line:
            b += [int(x, 16) for x in line.split(':', 1)[1].split()]
    return bytes(b)
open(snap + '.rec', 'wb').write(mem(P, 0x395))
open(snap + '.lks', 'wb').write(mem(P + 0x2e91, 0x162a))
# the test mod's results (04-device.c dt_res): after the marker, TEMPO, CHAIN, SONG, FILL (1: not run)
at = subprocess.run([dtpeek, snap, 'find', '5eed04047e570404'], env=dict(os.environ, DTEMU_FW=fw),
                    capture_output=True, text=True).stdout.split()
res = [int.from_bytes(mem(int(at[0], 16) + 8 + 4 * i, 4), 'big', signed=True) for i in range(4)] if at and 'match' not in at[1:2] else []
open(snap + '.res', 'w').write(' '.join(map(str, res)))
PY
}
# check NAME SNAP 'python expression on a (this snapshot) and the named others'
check() { name=$1; snap=$2; expr=$3; shift 3; for s in $snap "$@"; do [ -f $s.rec ] || dump $s; done
_chk=$(check_py "$name" "$expr" $snap "$@"); echo "$_chk" | sed "s/^PASS /PASS $EMU_NAME: /; s/^FAIL /FAIL $EMU_NAME: /"
case $_chk in PASS*) EMU_PASS=$((EMU_PASS + 1)) ;; *) EMU_FAIL=$((EMU_FAIL + 1)) ;; esac; }
check_py() { python3 - "$@" <<'PY'
import sys
name, expr, snaps = sys.argv[1], sys.argv[2], sys.argv[3:]
def load(s):
    r, l = open(s + '.rec', 'rb').read(), open(s + '.lks', 'rb').read()
    class T: pass
    t = T(); t.rec, t.lks = r, l
    t.res = [int(x) for x in open(s + '.res').read().split()]   # TEMPO, CHAIN, SONG, FILL; 0 OK, -3 ROOM
    t.flags = lambda i: r[2 * i] << 8 | r[2 * i + 1]
    t.fields = lambda i: tuple(r[o + i] for o in (0x80, 0xc0, 0x100, 0x140, 0x180, 0x1c0, 0x200, 0x240, 0x280))
    t.lock = lambda i, k: int.from_bytes(l[88 * i + 2 * k: 88 * i + 2 * k + 2], 'big', signed=True)
    # sound indexes 0..41; words 42 and 43 of a step's 88 bytes are not locks (stock keeps them itself)
    t.locks = lambda i: {k: t.lock(i, k) for k in range(42) if t.lock(i, k) >= 0}
    t.step = lambda i: (t.flags(i) & ~0x10, t.fields(i), l[88 * i: 88 * i + 88])
    return t
a = load(snaps[0]); o = [load(s) for s in snaps[1:]]
ok = eval(expr)
print(('PASS ' if ok else 'FAIL ') + name)
if not ok or '-v' in sys.argv:
    print('  results (TEMPO CHAIN SONG FILL)', a.res)
    for i in range(4):
        print('  step %d flags %04x fields %s locks %s' % (i, a.flags(i), a.fields(i), a.locks(i)))
PY
}

# 0. boot on a fresh card, close the FILE SYSTEM OK dialogs (as tools/test_seedr.sh part 1)
$B/dtcard $W/dev.img format > /dev/null
$B/dtboot $FW --instr 5.6G --card $W/dev.img --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G --save d0.snap > /dev/null
# 1. GRID REC, a trig on step 1, the FLTR page; T1 held: DEC, FRQ, RES turned (locks); GRID REC off
run d0.snap d1.snap --instr 3.0G --press RECORD@5.8G --press T1@6.0G --press FLTR@6.3G --press T1@6.6G:1.4G \
    --nudge 1:1@6.8Gx6/40M --nudge 4:1@7.2Gx6/40M --nudge 5:1@7.6Gx6/40M --press RECORD@8.3G
check "by hand: step 1 has a trig and three locks" d1.snap 'a.flags(0) & 1 and len(a.locks(0)) == 3 and not a.locks(1)'
# Each run starts at the clock its snapshot ends at (d1: 8.6 G); every press and hold ends inside its run.
# 2. TEMPO: one step later (d2 ends 9.0 G)
run d1.snap d2.snap --instr 0.4G --press TEMPO@8.7G
check "move: step 2 is step 1 with its locks, step 1 is empty" d2.snap \
    'a.res[0] == 0 and a.step(1) == o[0].step(0) and a.step(0) == o[0].step(1) and all(a.step(i) == o[0].step(i) for i in range(2, 64)) and (a.flags(1) & 0x10) == (o[0].flags(1) & 0x10)' d1.snap
# 3. SAVE PATTERN (d3 ends 9.8 G), move again (10.2 G), RELOAD PATTERN (11.0 G)
run d2.snap d3.snap --instr 0.8G --press YES@9.1G:0.4G --press SCNE@9.25G
run d3.snap d4.snap --instr 0.4G --press TEMPO@9.9G
check "move again: step 3 has the locks" d4.snap 'a.step(2) == o[0].step(1) and not (a.flags(1) & 1)' d3.snap
run d4.snap d5.snap --instr 0.8G --press NO@10.3G:0.4G --press SCNE@10.45G
check "RELOAD PATTERN: the saved move, three locks on step 2" d5.snap \
    'all(a.step(i) == o[0].step(i) for i in range(64)) and len(a.locks(1)) == 3' d2.snap
# 4. power cycle on a copy of the card
cp $W/dev.img $W/dev-pc.img
$B/dtboot $FW --load d5.snap --card $W/dev-pc.img --instr 0.8G --power-off 11.2G > /dev/null
$B/dtboot $FW --instr 5.6G --card $W/dev-pc.img --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G --save p1.snap > /dev/null
check "power cycle: the same steps and locks" p1.snap 'all(a.step(i) == o[0].step(i) for i in range(64))' d2.snap
# 5. audition (d6 ends 11.4 G) and restore (11.8 G)
run d5.snap d6.snap --instr 0.4G --press CHAIN@11.1G
check "CHAIN: a take (moved 3, sample lock on its trig, accent on step 5)" d6.snap \
    'a.res[1] == 0 and a.flags(4) & 1 and a.lock(4, 19) == 5 << 8 and len(a.locks(4)) == 4 and (a.flags(4) ^ o[0].flags(1)) & 0x08' d5.snap
run d6.snap d7.snap --instr 0.4G --press SONG@11.5G
check "SONG: record and lock store byte for byte as before (I7)" d7.snap \
    'a.res[2] == 0 and a.rec == o[0].rec and a.lks == o[0].lks and a.rec != o[1].rec' d5.snap d6.snap
# 6. a lock no row can hold (12.2 G)
run d7.snap d8.snap --instr 0.4G --press FILL@11.9G
check "FILL: refused with SEED_E_ROOM, nothing written (I8)" d8.snap 'a.res[3] == -3 and a.rec == o[0].rec and a.lks == o[0].lks' d7.snap
emu_done
