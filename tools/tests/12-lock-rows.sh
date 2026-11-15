#!/bin/sh
# Task D1: the pattern's stored lock rows (device/rytm_ar2.h: RT_LOCK_TYPE, rt_rows, the room check) in
# analog-emu, through a test mod (tools/tests/12-lock-rows.c; TEMPO runs the command poked into dt.cmd).
#   1. the map, from the default pattern A01: a lock on each sound index 0..41 through stock's lock set
#      makes the row (RT_LOCK_TYPE(track, k), track) on a voice track and on the FX track; with the fine
#      flag a second row (0x80, 0x80); the row goes with the lock. The table is in build/emu/12-lock-rows/map.txt.
#   2. near the limit (RT_ROWS_MAX 71), the device layer's write of a lane on BD step 0:
#      at 70 rows a new coarse lock is taken and survives SAVE + RELOAD PATTERN; at 71 a new one is
#      refused with nothing written (I8); a fine lock (two rows) is refused at 70 and taken at 69
#   3. stock past its limit: at 71 rows a lock set without the room check makes no row; SAVE + RELOAD
#      PATTERN keep it (the live lock store, not the rows, is what they save and restore)
# Snapshots, card images and the build stay in this checkout's build/emu/12-lock-rows/.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"
emu_init 12-lock-rows
EMU_QUIET=1 emu_build
M=$D/lrtest
rm -rf $M && mkdir -p $M && cp $R/tools/tests/12-lock-rows.c $M/lrtest.c
cat > $M/mod.json <<EOF
{
 "id": "lrtest", "version": "0.1", "title": "lock row test", "category": "Test",
 "license": "GPL-2.0-or-later", "author": "trig-seedr", "description": "Test build only (tools/tests/12-lock-rows.sh).",
 "device": "analog-rytm-mk2", "os": "1.74", "sources": ["lrtest.c"], "cflags": ["-I$R/device", "-fno-store-merging"],
 "subscribe": [{"event": "ev_key", "fn": "dt_key", "order": 40}, {"event": "ev_tick", "fn": "dt_tick", "order": 40}],
 "resources": {"core": "0.13"}, "requires": ["core"]
}
EOF
FW=$D/lrtest.syx MAP=$D/lrtest.syx.map.json
(cd $E/elekloader && emu_elebuild $M $M/out && PYTHONPATH=$E/elekloader python3 -m elekloader.patch --stock "$STOCK" \
    --mod "$CORE" --mod $M/out/lrtest-0.1.elemod --out $FW --lockfile $D/lrtest.lock.json --version SLRT) \
    > $D/lrtest-build.log 2>&1 || { cat $D/lrtest-build.log; emu_die "build lrtest"; }
export DTEMU_FW=$FW
DT=$(emu_py sym "$MAP" lrtest:dt) || emu_die "no symbol dt"
BD=0x407dd8e3 BDL=$((0x407dd8e3 + 0x2e91))         # A01 BD's record and lock store (firmware notes section 1)

# cmd CMD [ARG]: TEMPO with dt.cmd = CMD, dt.arg = ARG; a command that runs over ticks is run until done
cmd() {
    EMU_POKES="$(printf '0x%08x' $((DT + 8))):4=$1 $(printf '0x%08x' $((DT + 12))):4=${2:-0}"
    emu_run --press TEMPO@+100M
    _cmd_n=0
    while [ "$(res)" = 1 ] && [ $_cmd_n -lt 8 ]; do emu_run -s 2G; _cmd_n=$((_cmd_n + 1)); done
}
res() { emu_py peek "$EMU_SNAP" $((DT + 16)) 4 1 -s; }
used() { emu_py peek "$EMU_SNAP" $((DT + 20)) 4 1 -s; }
# lock STEP K: BD's stored lock (value x 256, -1 none); trig STEP: BD's trig bit
lock() { emu_py peek "$EMU_SNAP" $((BDL + 88 * $1 + 2 * $2)) 2 1 -s; }
trig() { echo $(($(emu_py peek "$EMU_SNAP" $((BD + 2 * $1)) 2 1) & 1)); }
# bd: a digest of BD's record and lock store
bd() { python3 -c "import sys; sys.path.insert(0, '$T'); import emu, hashlib
print(hashlib.sha1(emu.mem('$EMU_SNAP', $BD, 0x395) + emu.mem('$EMU_SNAP', $BDL, 0x162a)).hexdigest())"; }
save_reload() { emu_keys YES+SCNE w:300; emu_keys NO+SCNE w:300; }

emu_boot
S0=$EMU_SNAP

# ---- 1. the map ----
cmd 1
emu_check "measure on BD, MT, FX" 0 "$(res)"
cmd 2
emu_check "measure on FX, fine" 0 "$(res)"
emu_py peek "$EMU_SNAP" $((DT + 28)) 1 $((4 * 42 * 12)) > $D/map.raw
mapcheck() { python3 - $D/map.raw "$@" <<'PY'
import sys
v = [int(x) for x in open(sys.argv[1]).read().split()]
e = lambda p, k: v[(p * 42 + k) * 12:(p * 42 + k + 1) * 12]
track = (0, 6, 12, 12)
what = sys.argv[2]
if what == 'table':
    print('k   ' + '  '.join('%-14s' % n for n in ('BD fine 0', 'MT fine 1', 'FX fine 0', 'FX fine 1')))
    for k in range(42):
        print('%2d  ' % k + '  '.join('%-14s' % ' '.join('(%d,%d)' % (x[1 + 2 * i], x[2 + 2 * i]) for i in range(min(x[0], 2)))
                                     for x in (e(p, k) for p in range(4))))
    sys.exit()
p, rows = int(what), int(sys.argv[3])
bad = []
for k in range(42):
    x = e(p, k)
    want = [x[7], track[p]] + ([128, 128] if rows == 2 else [255, 255])
    if x[0] != rows or x[1:5] != want or x[8] != 1 or x[5] != 0:
        bad.append('k %d: %s' % (k, x[:11]))
types = [e(p, k)[1] for k in range(42)]
print('; '.join(bad) or ('ok %d types' % len(set(types))))
PY
}
mapcheck table > $D/map.txt
emu_check "voice track: each index's lock makes row (RT_LOCK_TYPE, BD), 42 types, freed after the clear" \
    "ok 42 types" "$(mapcheck 0 1)"
emu_check "voice track, fine: rows (RT_LOCK_TYPE, MT) and (128, 128)" "ok 42 types" "$(mapcheck 1 2)"
emu_check "FX track: each index's lock makes row (RT_LOCK_TYPE, FX); 38..41 share 18..21's types" \
    "ok 38 types" "$(mapcheck 2 1)"
emu_check "FX track, fine: rows (RT_LOCK_TYPE, FX) and (128, 128)" "ok 38 types" "$(mapcheck 3 2)"

# ---- 2. near the limit, through the device layer ----
emu_at $S0
cmd 3 70
emu_check "fill: 70 rows in use" "0 70" "$(res) $(used)"
F70=$EMU_SNAP
cmd 4 0                                              # TUN, sound index 17: one new row
emu_run -s 200M
emu_check "at 70 rows: a TUN lock on BD step 0 taken, 71 rows" "0 71 1 12800" "$(res) $(used) $(trig 0) $(lock 0 17)"
save_reload
emu_check "SAVE + RELOAD PATTERN: the lock taken at 70 rows is there" "71 1 12800" "$(used) $(trig 0) $(lock 0 17)"
before=$(bd)
cmd 4 1                                              # FIN, sound index 18: its own row type
emu_run -s 200M
emu_check "at 71 rows: a FIN lock refused, BD byte for byte as before (I8)" "-3 71 $before" "$(res) $(used) $(bd)"
# ---- 3. stock past its limit ----
cmd 5 18
emu_run -s 200M
emu_check "stock past the limit: the lock is set, no row made" "1 71 12800" "$(($(res) & 0xff)) $(used) $(lock 0 18)"
save_reload
emu_check "stock past the limit: SAVE + RELOAD PATTERN keep the lock, still with no row" "71 12800 12800" "$(used) $(lock 0 18) $(lock 0 17)"
# ---- fine: two rows ----
emu_at $F70
cmd 6 20                                             # BR, sound index 20: fine flag on, no lock left
emu_run -s 200M
emu_check "BR's fine flag on BD, no row made" "70" "$(used)"
before=$(bd)
cmd 4 2                                              # BR lane: needs two rows
emu_run -s 200M
emu_check "at 70 rows: a fine BR lock (two rows) refused, nothing written" "-3 70 $before" "$(res) $(used) $(bd)"
emu_at $S0
cmd 3 69
cmd 6 20
cmd 4 2
emu_run -s 200M
emu_check "at 69 rows: the fine BR lock taken, 71 rows" "0 71 12800" "$(res) $(used) $(lock 0 20)"
save_reload
emu_check "SAVE + RELOAD PATTERN: the fine lock taken at 69 rows is there" "71 12800" "$(used) $(lock 0 20)"
emu_done
