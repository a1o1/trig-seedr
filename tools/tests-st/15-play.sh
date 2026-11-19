#!/bin/sh
# The sequencer plays what seedr wrote (plan-syntakt.md S4; status/S2.md's open point). Track 4 (5 steps,
# emptied with stock's keys) gets EUCLID 2 of 5 (steps 1 and 4), kept; then PLAY for 12 s of analog-emu's clock
# with the second CPU's per-track output recorded (dtboot --dsp-wav, channel 4 = track 4). Its onsets
# (tools/onsets.py) must all fall on the take's trig steps at the preset's 135 BPM (a 16th is 111.1 ms) and hit
# both of them; with the empty original the channel is silent.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st15-play
st_build

# onsets WAV: track 4's onset times in ms, one line
onsets() {
    python3 "$T/voice.py" "$1" 3 "$1.t4.wav" > /dev/null
    python3 "$T/onsets.py" "$1.t4.wav" | head -1 | sed 's/^.*): *//; s/:[0-9]*//g'
}
# steps TRIGS ONSETS: "ok N" when every onset falls on a trig step of the 5-step loop (positions taken from the
# first onset) and every trig step is heard; else the positions
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
# the first onset is one of the trig steps: the alignment that puts every onset on one
pos = min((sorted({(c + round(r)) % n for r in rel}) for c in trigs), key=lambda p: p != trigs)
ok = pos == trigs and off < 0.2
print('ok %d' % len(t) if ok else 'positions %s, off by %.2f of a step' % (pos, off))
PY
}

emu_fixture empty fix_empty
S0=$EMU_SNAP
emu_run -s 2400M --press PLAY@+100M --dsp-wav "$D/empty.wav"
emu_check "the empty original: track 4 is silent" "" "$(onsets "$D/empty.wav")"

emu_at "$S0"
EMU_TRACK=3
save_orig empty
emu_recipe 3 src=euclid eu_steps=5 eu_puls=2
emu_open
emu_yes
emu_keep
emu_no
check_take "EUCLID 2 of 5, kept: seedref's take" src=euclid eu_steps=5 eu_puls=2 orig="$D/empty.txt"
TRIGS=$(emu_dump 3 | sed -n 's/^trig //p')
emu_run -s 2400M --press PLAY@+100M --dsp-wav "$D/take.wav"
O=$(onsets "$D/take.wav")
emu_check "PLAY: at least 30 onsets on track 4 in 12 s (two a 5-step loop)" 1 "$([ "$(echo $O | wc -w)" -ge 30 ] && echo 1)"
emu_check "PLAY: every onset on a trig step of the take, both steps heard" ok "$(steps "$TRIGS" "$O" | cut -d' ' -f1)"
emu_done
