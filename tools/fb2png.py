#!/usr/bin/env python3
"""dtpeek SNAP fb (half-block text, 2 pixel rows a line) -> a scaled PNG, OLED-style. Stdlib only.
    DTEMU_FW=os.syx dtpeek snap fb | python3 fb2png.py out.png [scale]"""
import sys, zlib, struct
rows = []
for line in sys.stdin.read().splitlines():
    if not line.strip('\n'):
        continue
    top = [c in '▀█' for c in line]   # upper half or full block
    bot = [c in '▄█' for c in line]   # lower half or full block
    rows += [top, bot]
rows = rows[:64]
w = max(len(r) for r in rows); h = len(rows); s = int(sys.argv[2]) if len(sys.argv) > 2 else 4
on, off = (0xf2, 0xf2, 0xf2), (0x10, 0x10, 0x14)
raw = b''
for r in rows:
    line = b''.join(bytes(on if (x < len(r) and r[x]) else off) * s for x in range(w))
    raw += (b'\x00' + line) * s
def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w * s, h * s, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
open(sys.argv[1], 'wb').write(png)
print('%s: %dx%d' % (sys.argv[1], w * s, h * s))
