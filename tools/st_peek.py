#!/usr/bin/env python3
"""tools/st_peek.py: a Syntakt OS 1.42 snapshot read as trig-seedr needs it (docs/firmware-notes-st142.md).

    st_peek.py dump SNAP [TRACK [N [PAT]]] [--lane ID]   a track in tools/emu.py dump's format
    st_peek.py rec SNAP                                   1 while grid recording is on, else 0
    st_peek.py trigpage SNAP                              the TRIG view's sub-page, -1 if it is not shown
    st_peek.py euc SNAP [TRACK [PAT]]                     EUC and PL1 of a track
    st_peek.py slot SNAP ID                               the lock slot of stock parameter ID (255: none)

dump --seed prints cond as lib/'s code (the Rytm's), as the device layer reads it (device/syntakt_st.h
st_cond_to_seed), so the dump compares with tools/seedref --device st line by line; --lane takes a lock slot.

TRACK 0..12 (12 the FX track), PAT 0..127 (0 = A01), N the steps shown (default: the track's length).
The dump's rows are emu.py dump's: flags & 0x3f, cond the Syntakt's own code (not the Rytm's), lane1 the
lock of sound id ID on each step (value >> 8, '-' none; no ID: all '-'). Needs DTPEEK and DTEMU_FW, as
tools/emu.sh exports them.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import emu  # noqa: E402

APP_PTR = 0x444e23f4          # the application object's pointer (0x4016e9c8 returns it)
UI_PTR = 0x444e2334           # the UI controller's pointer (0x4016e044)
PATTERNS = 0x3394             # app + 0x3394 + 0x680 * pattern (0x40019000)
PAT_SIZE = 0x680
TRACK_OBJ = 0x70              # pattern + 0x70 * (track + 1) (0x40015fae); its data at +0x10
LOCK_MODEL = 0x620            # pattern + 0x620 (0x40016062); its data at +0x10
SETTINGS = 0x2c               # pattern + 0x2c (0x40016056); its data at +0x10
LOCK_TRACK = 0x2447           # lock store bytes per track
LOCK_STEP = 72                # int16 slots per step (ids 0..70, then the step's lock count)
TRIG_VIEW_VT = 0x401c9a9c


def u32(snap, a):
    return int.from_bytes(emu.mem(snap, a, 4), 'big')


def pattern(snap, pat):
    return u32(snap, APP_PTR) + PATTERNS + PAT_SIZE * pat


def record(snap, track, pat):
    return u32(snap, pattern(snap, pat) + TRACK_OBJ * (track + 1) + 0x10)


def length(snap, rec, pat):
    s = u32(snap, pattern(snap, pat) + SETTINGS + 0x10)
    per_track = emu.mem(snap, s + 0x19, 1)[0]
    raw = emu.mem(snap, rec + 0x3c9, 2) if per_track else emu.mem(snap, s + 0x14, 2)
    return int.from_bytes(raw, 'big', signed=True)


COND_OWN = 65


def cond_to_seed(s):
    """device/syntakt_st.h st_cond_to_seed: the Syntakt's condition code as lib/'s, -1 none"""
    if s < 0 or s > 99:
        return -1
    if s < 30:
        return s
    if s in (30, 31):
        return COND_OWN + s - 30
    if s in (32, 33):
        return 30 + s - 32
    if (s - 34) & 1:
        return COND_OWN + 2 + (s - 35) // 2
    b, k, x = 3, 34, 32
    while s >= k + 2 * b:
        k, x, b = k + 2 * b, x + b, b + 1
    return x + (s - k) // 2


def slot(snap, pid):
    """a stock parameter id's lock slot: byte +7 of its entry in the parameter table (S2)"""
    return emu.mem(snap, 0x4022d843 + 52 * pid, 1)[0] if pid < 505 else 255


def dump(snap, track=0, n=0, pat=0, lane=None, seed=False):
    r = record(snap, track, pat)
    rec = emu.mem(snap, r, 0x3d8)
    n = n or length(snap, r, pat)
    flags = emu.ints(rec[0:2 * n], 2, False)
    u8 = lambda off: list(rec[off:off + n])
    s8 = lambda off: [v - 256 if v > 127 else v for v in rec[off:off + n]]
    dflt = lambda v: '-' if v == 0xff else str(v)
    if lane is None:
        lane1 = ['-'] * n
    else:
        lk = emu.mem(snap, u32(snap, pattern(snap, pat) + LOCK_MODEL + 0x10) + LOCK_TRACK * track, 2 * LOCK_STEP * n)
        vals = [int.from_bytes(lk[2 * (LOCK_STEP * i + lane):2 * (LOCK_STEP * i + lane) + 2], 'big', signed=True)
                for i in range(n)]
        lane1 = ['-' if v < 0 else str(v >> 8) for v in vals]
    rows = [('length', [str(n)]),
            ('trig', ['x' if f & 1 else '.' for f in flags]),
            ('flags', ['%02x' % (f & 0x3f) for f in flags]),
            ('note', [dflt(v) for v in u8(0x2c0)]),
            ('vel', [dflt(v) for v in u8(0x080)]),
            ('len', [dflt(v) for v in u8(0x0c0)]),
            ('utime', [str(v) for v in s8(0x100)]),
            ('cond', ['-' if v < 0 else str(v) for v in
                      ([cond_to_seed(c) for c in s8(0x200)] if seed else s8(0x200))]),
            ('rlen', [str(v) for v in u8(0x140)]),
            ('rrate', [str(v) for v in u8(0x180)]),
            ('rvel', [str(v) for v in s8(0x1c0)]),
            ('lane1', lane1)]
    return '\n'.join(k + ' ' + ' '.join(v) for k, v in rows)


def main_screen(snap):
    node = u32(snap, u32(snap, UI_PTR) + 0x40 + 0x1c)
    return u32(snap, node + 8)


def rec_flag(snap):
    """Grid recording: the sequencer selection's +0x161 (active) and +0x168 (grid REC), as Synfact's ARP
    reads them (main screen +0xc0, its +0x6c)."""
    sel = u32(snap, u32(snap, main_screen(snap) + 0xc0) + 0x6c)
    b = emu.mem(snap, sel + 0x161, 8)
    return 1 if b[0] and b[7] else 0


def trigpage(snap):
    v = u32(snap, main_screen(snap) + 0xc0)
    if u32(snap, v) != TRIG_VIEW_VT or u32(snap, v + 0x88) != 0x13:
        return -1
    return u32(snap, v + 0x90)


def main(a):
    lane, seed = None, '--seed' in a
    a = [x for x in a if x != '--seed']
    if '--lane' in a:
        i = a.index('--lane')
        lane = int(a[i + 1], 0)
        a = a[:i] + a[i + 2:]
    cmd, snap, rest = a[0], a[1], [int(x, 0) for x in a[2:]]
    if cmd == 'dump':
        track, n, pat = (rest + [0, 0, 0])[:3]
        print(dump(snap, track, n, pat, lane, seed))
    elif cmd == 'rec':
        print(rec_flag(snap))
    elif cmd == 'trigpage':
        print(trigpage(snap))
    elif cmd == 'slot':
        print(slot(snap, rest[0]))
    elif cmd == 'euc':
        track, pat = (rest + [0, 0])[:2]
        b = emu.mem(snap, record(snap, track, pat) + 0x3d1, 2)
        print(b[0], b[1])
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
