#!/usr/bin/env python3
"""Count note onsets in a WAV (stdlib only): an onset is the envelope rising past a threshold
after at least `gap` ms below it. Prints the onsets' times in ms. Usage: onsets.py FILE.wav"""
import sys, wave, struct
w = wave.open(sys.argv[1]); ch, sw, rate, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
raw = w.readframes(n)
fmt = {2: 'h', 4: 'i'}[sw]
vals = struct.unpack('<%d%s' % (len(raw) // sw, fmt), raw)
mono = [abs(v) for v in vals[::ch]]
peak = max(mono) or 1
thr, gap = 0.2 * peak, int(0.06 * rate)
onsets, below = [], gap
for i, v in enumerate(mono):
    if v >= thr:
        if below >= gap:
            onsets.append(i)
        below = 0
    else:
        below += 1
lv = [max(mono[o:o + int(0.05 * rate)]) * 100 // peak for o in onsets]
print('%d onsets (time ms:level %%): %s' % (len(onsets), ' '.join('%d:%d' % (1000 * o / rate, l) for o, l in zip(onsets, lv))))
print('peak %d of %d' % (peak, 2 ** (8 * sw - 1)))
