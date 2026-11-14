#!/usr/bin/env python3
"""Pull one channel out of a multi-channel WAV into a mono WAV. Usage: voice.py IN.wav CH OUT.wav"""
import sys, wave
w = wave.open(sys.argv[1]); ch, sw = w.getnchannels(), w.getsampwidth(); k = int(sys.argv[2])
raw = w.readframes(w.getnframes()); fr = ch * sw
out = b''.join(raw[i + k * sw:i + (k + 1) * sw] for i in range(0, len(raw), fr))
o = wave.open(sys.argv[3], 'wb'); o.setnchannels(1); o.setsampwidth(sw); o.setframerate(w.getframerate()); o.writeframes(out)
print('%s: %d channels; channel %d -> %s' % (sys.argv[1], ch, k, sys.argv[3]))
