#!/bin/sh
# Task T0a (docs/plan-turing.md): TURING's live page on the Rytm, proved with a test mod
# (mods/livetest-ar2): while the pattern plays, the steps of BD the playhead has passed are rewritten
# from the UI task through the device layer, and the sequencer plays each step as the track holds it.
#   1. a start pattern on BD (trigs on 0, 4, 8, 12, a lane-1 lock on 4); PLAY; live: every passed step
#      has its trig flipped and its velocity set, loop after loop
#   2. the interrupt's check: every step the clock made played the trig the record held (none missed),
#      rewritten steps among them; the record is the start pattern with each step flipped once per
#      rewrite; the other tracks unchanged
#   3. NO while playing (snapshot restore): BD's record and lock store and the lock rows byte for byte
#      as before the start, and nothing changes after it
#   4. 300 BPM: the tick keeps up (passed steps batched), still nothing missed
#   5. FUNC + YES while playing (keep): the track stays as rewritten, and no more writes
#   6. the cost of one step's rewrite, in instructions
# Snapshots, card and the build stay in build/emu/45-live/.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"
emu_init 45-live
EMU_QUIET=1 emu_build
M=$D/livetest
rm -rf "$M" && mkdir -p "$M" && cp "$R/mods/livetest-ar2/livetest.c" "$R/mods/livetest-ar2/mod.json" "$M/"
sed -i.bak 's|"../../device/rytm_ar2.h"|"'"$R"'/device/rytm_ar2.h"|' "$M/livetest.c" && rm -f "$M/livetest.c.bak"
FW=$D/livetest.syx MAP=$D/livetest.syx.map.json
(cd "$E/elekloader" && emu_elebuild "$M" "$M/out" && PYTHONPATH=$E/elekloader python3 -m elekloader.patch --stock "$STOCK" \
    --mod "$CORE" --mod "$M/out/livetest-0.1.elemod" --out "$FW" --lockfile "$D/livetest.lock.json" --version SLIV) \
    > "$D/livetest-build.log" 2>&1 || { cat "$D/livetest-build.log"; emu_die "build livetest"; }
export DTEMU_FW=$FW
LT=$(emu_py sym "$MAP" livetest:lt) || emu_die "no symbol lt"
LT_ONE=$(emu_py sym "$MAP" livetest:lt_rewrite_one) || emu_die "no symbol lt_rewrite_one"
LT_DONE=$(emu_py sym "$MAP" livetest:lt_rewrite_done) || emu_die "no symbol lt_rewrite_done"

# struct lt's words, by index (livetest.c)
I_CMD=2 I_ARG=3 I_RES=4 I_LIVE=5 I_TRACK=6 I_IRQ_CALLS=7 I_IRQ_FIRST=8 I_CHECKED=9 I_MATCH=10 I_MISS=11
I_MATCH_RW=12 I_IRQ_STEP=13 I_TICKS=14 I_POLLS=15 I_MAX_BATCH=16 I_REWRITES=17 I_REFUSED=18 I_LOOPS=19
I_LAST=20 I_WRES=21 I_DISAGREE=22 I_REC=23 I_CNT=24
w() { emu_py peek "$EMU_SNAP" $((LT + 4 * $1)) 4 1 -s; }
cnts() { emu_py peek "$EMU_SNAP" $((LT + 4 * I_CNT)) 4 64 -s; }
# cmd CMD [ARG]: TEMPO with lt.cmd = CMD, lt.arg = ARG
cmd() {
    EMU_POKES="$(printf '0x%08x' $((LT + 4 * I_CMD))):4=$1 $(printf '0x%08x' $((LT + 4 * I_ARG))):4=${2:-0}"
    emu_run --press TEMPO@+100M
}
BD=0x407dd8e3 BDL=$((0x407dd8e3 + 0x2e91))         # A01 BD's record and lock store (firmware notes section 1)
# bd: a digest of BD's record and lock store; others: the other twelve tracks' records
bd() { python3 -c "import sys, hashlib; sys.path.insert(0, '$T'); import emu
print(hashlib.sha1(emu.mem('$EMU_SNAP', $BD, 0x395) + emu.mem('$EMU_SNAP', $BDL, 0x162a)).hexdigest())"; }
others() { python3 -c "import sys, hashlib; sys.path.insert(0, '$T'); import emu
print(hashlib.sha1(emu.mem('$EMU_SNAP', $BD + 0x395, 12 * 0x395)).hexdigest())"; }
# rows: a digest of the pattern's lock-row table (behind BD's pool, track object +0x2c, at +0x20)
ROWS_AT=
rows() { python3 -c "import sys, hashlib; sys.path.insert(0, '$T'); import emu
print(hashlib.sha1(emu.mem('$EMU_SNAP', $1, 72 * 4)).hexdigest())"; }

emu_boot

# ---- 1. the start pattern, PLAY, live ----
cmd 1 0
emu_check "start pattern written" 0 "$(w $I_RES)"
emu_dump > "$D/orig.txt"
emu_check "start pattern: trigs on 0, 4, 8, 12" "trig x . . . x . . . x . . . x . . ." "$(sed -n 2p "$D/orig.txt")"
BD0=$(bd) OT0=$(others)
emu_keys PLAY
cmd 2 0
emu_check "live started (snapshot taken)" "0 1" "$(w $I_RES) $(w $I_LIVE)"
emu_run -s 4G                            # about 20 s at 120 BPM: 160 sixteenths, 10 loops

# ---- 2. the sequencer plays what the record holds ----
emu_check "the interrupt saw the track's steps" 1 "$([ "$(w $I_CHECKED)" -gt 100 ] && echo 1)"
emu_check "every step made played the record's trig (no miss)" 0 "$(w $I_MISS)"
emu_check "rewritten steps played as rewritten" 1 "$([ "$(w $I_MATCH_RW)" -gt 50 ] && echo 1)"
emu_check "loops seen while live" 1 "$([ "$(w $I_LOOPS)" -ge 5 ] && echo 1)"
emu_check "no write refused" 0 "$(w $I_REFUSED)"
emu_check "the polled step and the interrupt's agree" 0 "$(w $I_DISAGREE)"
# the record: each step's trig is the start pattern's flipped once per rewrite, velocity 20 + count
emu_dump > "$D/live.txt"
emu_check "BD: every step flipped once per rewrite, velocity 20 + rewrites" ok "$(python3 - "$D/orig.txt" "$D/live.txt" "$(cnts)" <<'PY'
import sys
o = dict(l.split(None, 1) for l in open(sys.argv[1]))
n = dict(l.split(None, 1) for l in open(sys.argv[2]))
cnt = [int(x) for x in sys.argv[3].split()]
ot, nt, nv = o['trig'].split(), n['trig'].split(), n['vel'].split()
bad = []
for i in range(len(ot)):
    want = ot[i] if cnt[i] % 2 == 0 else ('.' if ot[i] == 'x' else 'x')
    if nt[i] != want:
        bad.append('trig %d' % i)
    if cnt[i] and nv[i] != str(20 + cnt[i] % 100):
        bad.append('vel %d %s' % (i, nv[i]))
print('ok' if not bad and sum(cnt) > 0 else 'bad: %s, counts %s' % (bad, cnt))
PY
)"
emu_check "other tracks unchanged" "$OT0" "$(others)"
emu_check "step 4's lane-1 lock kept through its rewrites" "$(sed -n 's/^lane1 //p' "$D/orig.txt" | cut -d' ' -f5)" \
    "$(sed -n 's/^lane1 //p' "$D/live.txt" | cut -d' ' -f5)"

# ---- 3. NO while playing ----
cmd 3
emu_run -s 600M
emu_check "NO while playing: BD byte for byte as before the start (record and locks)" "$BD0" "$(bd)"
R1=$(w $I_REWRITES)
emu_run -s 1G
emu_check "after NO: no more writes, BD unchanged" "$R1 $BD0" "$(w $I_REWRITES) $(bd)"

# ---- 4. 300 BPM ----
# the tempo (core_tempo, BPM x 120), poked; the clock follows it at its next tick
emu_poke tempo 0 2>/dev/null
EMU_POKES="0x40287ffc:4=36000"
emu_run -s 200M
emu_check "tempo 300 BPM" 36000 "$(emu_py peek "$EMU_SNAP" 0x40287ffc 4 1)"
cmd 2 0
emu_run -s 4G                            # 20 s at 300 BPM: 400 sixteenths
emu_check "300 BPM: no miss, no refusal" "0 0" "$(w $I_MISS) $(w $I_REFUSED)"
emu_check "300 BPM: rewrites keep pace (at least 300)" 1 "$([ "$(w $I_REWRITES)" -ge 300 ] && echo 1)"
echo "  300 BPM: largest batch of passed steps in one tick: $(w $I_MAX_BATCH); ticks $(w $I_TICKS), polls $(w $I_POLLS), rewrites $(w $I_REWRITES), loops $(w $I_LOOPS)"
emu_check "300 BPM: the tick batches the steps it missed (largest batch >= 1)" 1 "$([ "$(w $I_MAX_BATCH)" -ge 1 ] && echo 1)"

# ---- 5. FUNC + YES while playing: keep ----
cmd 4
emu_run -s 400M
K1=$(bd) R2=$(w $I_REWRITES)
emu_run -s 1G
emu_check "keep: no more writes, BD stays as rewritten" "$R2 $K1" "$(w $I_REWRITES) $(bd)"
emu_check "keep: BD differs from the start pattern" 1 "$([ "$K1" != "$BD0" ] && echo 1)"

# ---- 6. the cost of one step's rewrite ----
# DMA timer 0's counter, timed in the tick around each rewrite (lt.dt_*), against emulator instructions:
# the counter's rate from two ticks a known number of instructions apart. The emulator cannot break in
# the mods' area, so the mod times itself.
I_DT_NOW=88 I_DT_LAST=89 I_DT_MAX=90 I_DT_SUM=91 I_DT_N=92
u() { emu_py peek "$EMU_SNAP" $((LT + 4 * $1)) 4 1; }
N=$(u $I_DT_N) SUM=$(u $I_DT_SUM) MAX=$(u $I_DT_MAX)
C0=$EMU_CLK D0=$(u $I_DT_NOW)
emu_run -s 2G
C1=$EMU_CLK D1=$(u $I_DT_NOW)
emu_check "the timer counts" 1 "$([ "$N" -gt 0 ] && [ "$D1" != "$D0" ] && echo 1)"
python3 - "$N" "$SUM" "$MAX" "$C0" "$D0" "$C1" "$D1" <<'PY' | tee "$D/cost.txt"
import sys
n, s, mx, c0, d0, c1, d1 = (int(x) for x in sys.argv[1:])
rate = ((d1 - d0) % 2**32) / (c1 - c0)          # timer counts per emulator instruction
avg = s / n
print('  one step\'s rewrite at 300 BPM (read, write of the changed step, change notice): %d rewrites, '
      'average %.0f timer counts (%.0f instructions), largest %d (%.0f instructions); the timer runs %.3f '
      'counts an instruction; at 200M instructions a second the average is %.0f us' %
      (n, avg, avg / rate, mx, mx / rate, rate, avg / rate / 200))
PY
emu_done
