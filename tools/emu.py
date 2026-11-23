#!/usr/bin/env python3
"""Emulator helpers for tools/emu.sh. Stdlib only. DTPEEK and DTEMU_FW come from the environment.

    emu.py sym MAP NAME[+OFF]...           addresses (hex) from a patched OS's .map.json
    emu.py peek SNAP ADDR SIZE [N] [-s]    N big-endian integers of SIZE bytes (1, 2, 4) at ADDR
    emu.py pokes SNAP ADDR:SIZE=VALUE...   dtboot --poke arguments, merged into aligned 32-bit words
    emu.py dump SNAP [TRACK [LEN [PAT]]]   a track as seedref prints it (seedref.c: the format); LEN
                                           0 or none: the track's length
    emu.py plan CLOCK SETTLE ARG...        dtboot arguments with @+REL clocks made absolute; first
                                           line the --instr to run (the last clock + SETTLE)
    emu.py layout CC HEADER STRUCT FIELD...  offset and size of each field, from the cross compiler
    emu.py title SNAP                      a hash of the screen's title bar (the first 5 text rows)
    emu.py trigpage MAP SNAP               stock's TRIG page: 0 TRIG, 1 EUCLIDEAN, -1 not showing
"""
import hashlib, json, os, re, subprocess, sys, tempfile

REC0 = 0x407dd8e3                  # live pattern 1 (A01) BD's step record (firmware-notes-os174.md)
PAT_STRIDE, REC_SIZE, LOCKS, LOCK_SIZE = 0x14f00, 0x395, 0x2e91, 0x162a
SAMPLE_SLOT = 19                   # the lane-1 parameter on the Rytm: sound index 19


def num(t):
    """1.5G, 300M, 20k, 0x40, 1234 -> int"""
    m = re.fullmatch(r'([0-9.]+)([kMG]?)', t)
    if not m:
        return int(t, 0)
    return int(round(float(m.group(1)) * {'': 1, 'k': 1e3, 'M': 1e6, 'G': 1e9}[m.group(2)]))


def sym(mapfile, names):
    with open(mapfile) as fh:
        m = json.load(fh)
    out = []
    for n in names:
        off = 0
        if '+' in n:
            n, o = n.split('+', 1)
            off = int(o, 0)
        a = m.get(n, m.get('seedr:' + n))
        if a is None:
            sys.exit('emu.py: no symbol %s in %s' % (n, mapfile))
        out.append('0x%08x' % (int(a, 16) + off))
    return out


def mem(snap, addr, length):
    out = subprocess.run([os.environ['DTPEEK'], snap, 'mem', '%x' % addr, str(length)],
                         capture_output=True, text=True, check=True).stdout
    b = []
    for line in out.splitlines():
        if ':' in line:
            b += [int(x, 16) for x in line.split(':', 1)[1].split()]
    if len(b) < length:
        sys.exit('emu.py: dtpeek gave %d of %d bytes at %x' % (len(b), length, addr))
    return bytes(b[:length])


def ints(b, size, signed):
    return [int.from_bytes(b[i:i + size], 'big', signed=signed) for i in range(0, len(b), size)]


def peek(snap, addr, size, n=1, signed=False):
    return ints(mem(snap, addr, size * n), size, signed)


def pokes(snap, specs):
    words = {}                             # aligned address -> bytearray(4)
    for s in specs:
        at, value = s.split('=', 1)
        addr, size = at.split(':')
        addr, size, value = int(addr, 0), int(size), int(value, 0)
        raw = (value & ((1 << 8 * size) - 1)).to_bytes(size, 'big')
        for i, byte in enumerate(raw):
            a = addr + i
            w = a & ~3
            if w not in words:
                words[w] = bytearray(mem(snap, w, 4))
            words[w][a - w] = byte
    return ['--poke %x=%s' % (w, bytes(b).hex()) for w, b in sorted(words.items())]


def dump(snap, track=0, n=0, pat=0):
    """The step record's fields in seedref's dump format (tools/seedref.c)."""
    r = REC0 + PAT_STRIDE * pat + REC_SIZE * track
    rec = mem(snap, r, 0x2c8)
    n = n or rec[0x2c5]                    # default: the track's length
    lk = mem(snap, REC0 + PAT_STRIDE * pat + LOCKS + LOCK_SIZE * track, 88 * n)
    flags = ints(rec[0:2 * n], 2, False)
    u8 = lambda off: list(rec[off:off + n])
    s8 = lambda off: [v - 256 if v > 127 else v for v in rec[off:off + n]]
    dflt = lambda v: '-' if v == 0xff else str(v)
    lane1 = [int.from_bytes(lk[88 * i + 2 * SAMPLE_SLOT:88 * i + 2 * SAMPLE_SLOT + 2], 'big', signed=True)
             for i in range(n)]
    rows = [('length', [str(rec[0x2c5])]),
            ('trig', ['x' if f & 1 else '.' for f in flags]),
            ('flags', ['%02x' % (f & 0x3f) for f in flags]),
            ('note', [dflt(v) for v in u8(0x80)]),
            ('vel', [dflt(v) for v in u8(0xc0)]),
            ('len', [dflt(v) for v in u8(0x100)]),
            ('utime', [str(v) for v in s8(0x140)]),
            ('cond', ['-' if v < 0 else str(v) for v in s8(0x240)]),
            ('rlen', [str(v) for v in u8(0x180)]),
            ('rrate', [str(v) for v in u8(0x1c0)]),
            ('rvel', [str(v) for v in s8(0x200)]),
            ('lane1', ['-' if v < 0 else str(v >> 8) for v in lane1])]
    return '\n'.join(k + ' ' + ' '.join(v) for k, v in rows)


def plan(clock, settle, args):
    last, out = clock, []
    for a in args:
        def absolute(m):
            nonlocal last
            t = clock + num(m.group(1))
            if m.group(2):                       # xN/I: N events, I apart; the run covers the last
                t_last = t + (int(m.group(2)) - 1) * num(m.group(3))
            else:
                t_last = t
            last = max(last, t_last)
            return '@%d' % t + (m.group(0)[m.group(0).index('x'):] if m.group(2) else '')
        out.append(re.sub(r'@\+([0-9.]+[kMG]?)(?:x([0-9]+)/([0-9.]+[kMG]?))?', absolute, a))
    return [str(last - clock + settle)] + out


def layout(cc, header, struct, fields):
    src = '#include "%s"\n' % os.path.abspath(header)
    for i, f in enumerate(fields):
        src += 'const int o%d = __builtin_offsetof(struct %s, %s), s%d = sizeof(((struct %s *)0)->%s);\n' % (
            i, struct, f, i, struct, f)
    src += 'const int z = sizeof(struct %s);\n' % struct
    with tempfile.TemporaryDirectory() as d:
        c = os.path.join(d, 'l.c')
        with open(c, 'w') as fh:
            fh.write(src)
        asm = subprocess.run([cc, '-mcpu=54455', '-ffreestanding', '-O0', '-S', '-o', '-', c], capture_output=True, text=True,
                             check=True).stdout
    vals = {}
    for m in re.finditer(r'^(\w+):\s*\n\s*\.(long\s+(-?\d+)|zero\s+4)', asm, re.M):   # 0 is ".zero 4"
        vals[m.group(1)] = int(m.group(3) or 0)
    out = ['%s %d %d' % (f, vals['o%d' % i], vals['s%d' % i]) for i, f in enumerate(fields)]
    return out + ['sizeof %d' % vals['z']]


def trigpage(mapfile, snap):
    """Stock's TRIG page sub-page (0 TRIG, 1 EUCLIDEAN), or -1 when it is not showing: seedr.c's
    trig_subpage(), from the view controller the mod keeps in ui_ctrl (firmware-notes-os174.md 5)."""
    ctrl = peek(snap, int(sym(mapfile, ['ui_ctrl'])[0], 16), 4)[0]
    n = peek(snap, ctrl + 28, 4)[0] if ctrl else 0
    for _ in range(16):
        if not n or n == ctrl + 20:
            break
        v = peek(snap, n + 8, 4)[0]
        if v and peek(snap, v, 4)[0] == 0x401e3148:
            return peek(snap, v + 0x8c, 4, 1, True)[0]
        n = peek(snap, n, 4)[0]
    return -1


def title(snap):
    out = subprocess.run([os.environ['DTPEEK'], snap, 'fb'], capture_output=True, text=True).stdout
    return hashlib.sha1('\n'.join(out.splitlines()[:5]).encode()).hexdigest()[:8]


def main(a):
    cmd = a[0] if a else ''
    if cmd == 'sym':
        print('\n'.join(sym(a[1], a[2:])))
    elif cmd == 'peek':
        signed = '-s' in a
        a = [x for x in a if x != '-s']
        print(' '.join(map(str, peek(a[1], int(a[2], 0), int(a[3]), int(a[4]) if len(a) > 4 else 1, signed))))
    elif cmd == 'pokes':
        print(' '.join(pokes(a[1], a[2:])))
    elif cmd == 'dump':
        print(dump(a[1], *[int(x) for x in a[2:5]]))
    elif cmd == 'plan':
        print('\n'.join(plan(num(a[1]), num(a[2]), a[3:])))
    elif cmd == 'layout':
        print('\n'.join(layout(a[1], a[2], a[3], a[4:])))
    elif cmd == 'title':
        print(title(a[1]))
    elif cmd == 'trigpage':
        print(trigpage(a[1], a[2]))
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
