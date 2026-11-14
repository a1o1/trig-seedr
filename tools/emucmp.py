#!/usr/bin/env python3
"""Read A01 BD's step record from an analog-emu snapshot and print it in genref's format.
    emucmp.py DTPEEK FIRMWARE SNAPSHOT [LENGTH [TRACK]]"""
import os, subprocess, sys
dtpeek, fw, snap = sys.argv[1:4]
n = int(sys.argv[4]) if len(sys.argv) > 4 else 16
trk = int(sys.argv[5]) if len(sys.argv) > 5 else 0
P = 0x407dd8e3                                   # live pattern object 1 = A01
R = P + 0x395 * trk                              # track trk's record (0 = BD)
def mem(addr, length):
    out = subprocess.run([dtpeek, snap, 'mem', '%x' % addr, str(length)], env=dict(os.environ, DTEMU_FW=fw),
                         capture_output=True, text=True, check=True).stdout
    b = []
    for line in out.splitlines():
        if ':' in line:
            b += [int(x, 16) for x in line.split(':', 1)[1].split()]
    return bytes(b)
flags = mem(R, 2 * n); vel = mem(R + 0xc0, n); ut = mem(R + 0x140, n); lk = mem(P + 0x2e91 + 0x162a * trk, 88 * n); mod = mem(0x405c8e4e, 1)[0]
print(' '.join(('x' if (flags[2*i] << 8 | flags[2*i+1]) & 1 else '.') + '%02x' % vel[i] for i in range(n)) + ' ')
print('flags ' + ' '.join('%04x' % (flags[2*i] << 8 | flags[2*i+1]) for i in range(n)))
print('utime ' + ' '.join(str(b - 256 if b > 127 else b) for b in ut))
print('slock ' + ' '.join(str(-1 if v >= 0x8000 else v >> 8) for v in (lk[88 * i + 38] << 8 | lk[88 * i + 39] for i in range(n))))
print('modified %d' % mod)
