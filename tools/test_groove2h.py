#!/usr/bin/env python3
"""Checks groove2h.py's .mid reader: Live's plain .agr grooves written out as MIDI files (format 1 with a
tempo track and running status, and format 0) must give the same tables as the .agr itself.
    test_groove2h.py GROOVE_DIR"""
import importlib.util, os, sys, tempfile
spec = importlib.util.spec_from_file_location('g2h', os.path.join(os.path.dirname(__file__), 'groove2h.py'))
g2h = importlib.util.module_from_spec(spec)
sys.argv, argv = ['groove2h.py', '--dump'], sys.argv           # main() runs on import: give it nothing to do
spec.loader.exec_module(g2h)
sys.argv = argv

def vlq(n):
    out = [n & 0x7f]
    while n > 127:
        n >>= 7; out.insert(0, 0x80 | n & 0x7f)
    return bytes(out)

def smf(notes, length, fmt=1, div=960):
    ev = []
    for t, v in notes:
        ev.append((round(t * div), 0x90, 36, int(v)))
        ev.append((round((t + 0.0625) * div), 0x90, 36, 0))     # note-off as velocity 0 (running status)
    ev.sort()
    trk, last, run = b'', 0, None
    for t, st, a, b in ev:
        trk += vlq(t - last) + (bytes([st]) if st != run else b'') + bytes([a, b]); last, run = t, st
    trk += vlq(round(length * div) - last) + b'\xff\x2f\x00'
    tempo = b'\x00\xff\x58\x04\x04\x02\x18\x08' + b'\x00\xff\x51\x03\x07\xa1\x20' + b'\x00\xff\x2f\x00'
    chunk = lambda k, d: k + len(d).to_bytes(4, 'big') + d
    if fmt == 0:
        return chunk(b'MThd', (0).to_bytes(2, 'big') + (1).to_bytes(2, 'big') + div.to_bytes(2, 'big')) + \
            chunk(b'MTrk', tempo[:-4] + trk)
    return chunk(b'MThd', (1).to_bytes(2, 'big') + (2).to_bytes(2, 'big') + div.to_bytes(2, 'big')) + \
        chunk(b'MTrk', tempo) + chunk(b'MTrk', trk)

# A MIDI file holds whole velocities (Live's can be fractional) and whole ticks, so a value that
# falls on a rounding boundary may come out 1 different: allowed.
close = lambda x, y: len(x) == len(y) and all(abs(i - j) <= 1 for i, j in zip(x, y))
fails = checks = 0
d = argv[1]
tmp = tempfile.mkdtemp()
for rel in ['Swing/MPC/Swing MPC 3000 16ths 57.agr', 'Swing/MPC/Swing MPC 3000 8ths 61.agr',
            'Style/Funk Modern 16ths.agr', 'Style/Hip Hop Boom Bap 16ths 90 bpm.agr',
            'Swing/SP 1200/Swing SP 1200 8ths 67.agr']:
    a = g2h.table_agr(os.path.join(d, rel))
    info = a['info']
    for fmt in (0, 1):
        stem = os.path.basename(rel)[:-4]
        p = os.path.join(tmp, f'{stem} f{fmt}.mid')
        open(p, 'wb').write(smf(info['notes'], info['length'], fmt))
        m = g2h.table_agr(p, base=info['base'])
        ok = m['len'] == a['len'] and abs(m['vmax'] - a['vmax']) <= 1 and close(m['off'], a['off']) and close(m['vel'], a['vel'])
        checks += 1; fails += not ok
        print(('PASS' if ok else 'FAIL'), rel, 'format', fmt)
        if not ok:
            print('  agr', a['len'], a['off'], a['vel']); print('  mid', m['len'], m['off'], m['vel'])
    # Base from the file name when not given
    m = g2h.table_agr(p)
    ok = close(m['off'], a['off'])
    checks += 1; fails += not ok
    print(('PASS' if ok else 'FAIL'), rel, 'base from the name')
print(f'{checks} checks, {fails} failed')
sys.exit(fails != 0)
