#!/usr/bin/env python3
# tools/st_views.py: the views on a Syntakt OS 1.42 view controller's list, from a snapshot (task P1), with
# their RTTI class names, as elekloader core-ar2's core_stock_overlay walks them on the Rytm: the controller's
# +28 the first node, +20 the end; a node's +0 the next, +8 its view; a view's +0 its vtable; vtable - 4 the
# typeinfo, its +4 the name ("<len><name>").
#     st_views.py SNAP CTRL      one line per view: vtable, class
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import emu  # noqa: E402

CODE_LO, CODE_HI = 0x40000000, 0x40400000


def u32(snap, a):
    return int.from_bytes(emu.mem(snap, a, 4), 'big')


def class_name(snap, vt):
    if vt < CODE_LO + 4 or vt >= CODE_HI or vt & 1:
        return '?'
    ti = u32(snap, vt - 4)
    if ti < CODE_LO or ti >= CODE_HI or ti & 1:
        return '?'
    nm = u32(snap, ti + 4)
    if nm < CODE_LO or nm >= CODE_HI:
        return '?'
    s = emu.mem(snap, nm, 64).split(b'\0')[0].decode('latin-1')
    return s.lstrip('0123456789')


def views(snap, ctrl):
    out, n = [], u32(snap, ctrl + 28)
    for _ in range(32):
        if not n or n == ctrl + 20:
            break
        v = u32(snap, n + 8)
        if v:
            vt = u32(snap, v)
            out.append((vt, class_name(snap, vt)))
        n = u32(snap, n)
    return out


if __name__ == '__main__':
    for vt, name in views(sys.argv[1], int(sys.argv[2], 0)):
        print('%08x %s' % (vt, name))
