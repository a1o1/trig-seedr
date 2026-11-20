#!/bin/sh
# Writing behind the playhead while the pattern plays (docs/plan-turing.md task T0b; TURING's live page, design L6,
# L8), through a test mod (mods/livetest-st on the hook-bus core): each UI tick it reads track 4's playhead
# (device/syntakt_st.h st_playhead) and writes the steps it passed since the last read (st_steps_passed,
# st_seed_write_steps) from a target. Track 4 (5 steps) is emptied with stock's keys; the target has trigs on steps 1
# and 4. PLAY for 12 s with track 4's output recorded (dtboot --dsp-wav): the track ends as the target, every onset
# falls on a target step (the first loop plays the empty original: a step is written after it has played), and no
# other track changes. Then, still playing, live stops and a restore gives back track 4 byte for byte (I7).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"
emu_init st45-live
FW=$(sh "$T/st_build.sh" livetest "$R/mods/livetest-st" | tail -1) || emu_die "st_build.sh livetest"
MAP=$FW.map.json DTEMU_FW=$FW
export DTEMU_FW
LT=$(emu_py sym "$MAP" livetest-st:lt) || emu_die "no symbol lt"

# lt FIELD: a word of the test mod's struct livetest
lt() {
    _lt_i=0
    for _lt_f in magic cmd arg res mem ticks track len live polls writes steps maxper stopped prev now; do
        [ "$_lt_f" = "$1" ] && break
        _lt_i=$((_lt_i + 1))
    done
    emu_py peek "$EMU_SNAP" $((LT + 4 * _lt_i)) 4 1 -s
}
# cmd N [ARG]: poke lt.arg and lt.cmd, run until the mod's tick has taken it
cmd() {
    EMU_POKES="$(printf '0x%08x' $((LT + 8))):4=${2:-0} $(printf '0x%08x' $((LT + 4))):4=$1"
    emu_run -s 300M
    [ "$(lt cmd)" = 0 ] || emu_die "command $1 not taken"
}
# onsets WAV: track 4's onset times in ms, one line (as 15-play)
onsets() {
    python3 "$T/voice.py" "$1" 3 "$1.t4.wav" > /dev/null
    python3 "$T/onsets.py" "$1.t4.wav" | head -1 | sed 's/^.*): *//; s/:[0-9]*//g'
}
# steps TRIGS ONSETS: "ok N" when every onset falls on a trig step of the 5-step loop and every trig step is heard
steps() {
    python3 - "$1" "$2" <<'PY'
import sys
trigs = [i for i, c in enumerate(sys.argv[1].split()) if c == 'x']
t = [float(x) for x in sys.argv[2].split()]
if not t:
    print('no onsets'); sys.exit()
step, n = 60000 / 135 / 4, 5
rel = [(x - t[0]) / step for x in t]
off = max(abs(r - round(r)) for r in rel)
pos = min((sorted({(c + round(r)) % n for r in rel}) for c in trigs), key=lambda p: p != trigs)
print('ok %d' % len(t) if pos == trigs and off < 0.2 else 'positions %s, off by %.2f of a step' % (pos, off))
PY
}
# others: every track but 4, record and lock store, as one digest
others() {
    _o_t=0
    while [ $_o_t -le 12 ]; do
        [ $_o_t = 3 ] || { st_mem rec $_o_t; st_mem locks $_o_t; }
        _o_t=$((_o_t + 1))
    done | shasum | cut -c1-16
}

# 1. boot; track 4 emptied with stock's keys (as fixtures.sh's fix_empty)
emu_boot
emu_check "the test mod runs" 1280725844 "$(lt magic)"
emu_rec 0
emu_keys TRK+T4
emu_rec 1
emu_keys T2 T4
emu_rec 0
emu_check "track 4 emptied with stock's keys" 0 "$(emu_dump 3 | grep '^trig' | grep -c x)"
ORIG=$(st_mem rec 3)$(st_mem locks 3)
OTHERS=$(others)

# 2. attach; live on track 4 towards trigs on steps 1 and 4 (bits 0 and 3)
cmd 1
emu_check "attach" 0 "$(lt res)"
cmd 2 9
emu_check "live on: track 4, 5 steps" "0 3 5 1" "$(lt res) $(lt track) $(lt len) $(lt live)"
emu_check "stopped: the playhead is -1, nothing written" "-1 0" "$(lt now) $(lt steps)"

# 3. PLAY for 12 s, track 4's output recorded
T0=$(lt ticks)
emu_run -s 2400M --press PLAY@+100M --dsp-wav "$D/live.wav"
TICKS=$(( $(lt ticks) - T0 ))
emu_check "the UI tick: about 30 a second (12.5 s run)" 1 "$([ $TICKS -ge 360 ] && [ $TICKS -le 390 ] && echo 1)"
emu_check "playing: the playhead is read (0..4)" 1 "$([ "$(lt now)" -ge 0 ] && [ "$(lt now)" -lt 5 ] && echo 1)"
emu_check "every step written at least once, at most 2 a tick at 135 BPM" 1 \
    "$([ "$(lt steps)" -ge 5 ] && [ "$(lt maxper)" -ge 1 ] && [ "$(lt maxper)" -le 2 ] && echo 1)"
emu_check "track 4 is the target: trigs on steps 1 and 4" "trig x . . x ." "$(emu_dump 3 | grep '^trig')"
O=$(onsets "$D/live.wav")
emu_check "PLAY: at least 30 onsets on track 4 in 12 s" 1 "$([ "$(echo $O | wc -w)" -ge 30 ] && echo 1)"
emu_check "PLAY: every onset on a target step, both heard" ok "$(steps 'x . . x .' "$O" | cut -d' ' -f1)"
# PLAY at 500 ms (100M), a loop is 5 x 111.1 ms: the first loop plays the empty original, so the first onset is the
# second loop's step 1, at 1055 ms
emu_check "the first loop is the original: the first onset on the second loop (>= 1040 ms)" 1 \
    "$([ "$(echo $O | cut -d' ' -f1 | cut -d. -f1)" -ge 1040 ] && echo 1)"
emu_check "no other track changed" "$OTHERS" "$(others)"

# 4. still playing: live off, restore: track 4 byte for byte as before
cmd 3
cmd 4
emu_check "still playing (the sequencer's running byte)" 1 "$(emu_py peek "$EMU_SNAP" 0x43bd83bf 1)"
emu_check "restore while playing: track 4's record and lock store byte for byte" "$ORIG" "$(st_mem rec 3)$(st_mem locks 3)"
emu_check "restore: no other track changed" "$OTHERS" "$(others)"
cmd 5
emu_done
