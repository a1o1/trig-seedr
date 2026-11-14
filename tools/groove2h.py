#!/usr/bin/env python3
"""Groove tables for seedr (lib/groove.h) from swing amounts and Ableton Live .agr files.

    groove2h.py [--swing 54,58,62,66,71] [--base 1/16] [--beats N] [FILE.agr | FILE.mid | DIR ...] > ../mods/seedr-ar2/grooves-live.h
A .mid (e.g. a clip exported from Live with its groove committed) gives note times and velocities;
its Base comes from --base, or from the name ('8ths' -> 1/8, otherwise 1/16), and its length from
--beats, or the file's end rounded up to whole bars.
Files in Live's protected format (they start ab 1e 56 78) are skipped with a note, as are
Live's "Quantize ..." utility grooves (the QUANT knob does that).
    groove2h.py --dump FILE.agr          # print what was read from the file, and the table

One table entry per Rytm step (1/16 at speed 1x). An .agr is Live's groove: a MIDI clip of note
times (beats) and velocities, and a Base grid. Each base-grid line takes the displacement of the
groove note nearest to it (within half a grid); the steps between lines move in proportion
(a piecewise-linear time warp), which is how Live describes Base. A step on a grid line with a
groove note also takes that note's velocity; other steps keep theirs (velocity 0 in the table).
µtime: 1/24 of a step = 1/96 beat.
"""
import gzip, os, re, sys, xml.etree.ElementTree as ET

STEPS = 64

def table_swing(pct):
    """MPC-style: the second 16th of each 8th placed at pct % of the 8th."""
    off = round((pct / 100 * 2 - 1) * 24)
    return dict(name=f"SWING {pct}", len=2, vmax=0, off=[0, off], vel=[0, 0], src=f"swing {pct}%")

def read_agr(path):
    """Live 11's plain .agr: XML with Groove/Clip/Value/MidiClip (notes, loop) and Groove/Grid
    (the Base chooser: 0 1/4, 1 1/8, 2 1/8T, 3 1/16, 4 1/16T, 5 1/32 [1 and 3 checked against the
    files' names]). Files starting ab 1e 56 78 are Live's protected format and are not read."""
    raw = open(path, 'rb').read()
    if raw[:2] == b'\x1f\x8b':
        raw = gzip.decompress(raw)
    if raw[:4] == bytes.fromhex('ab1e5678'):
        raise ValueError("protected .agr format (not plain XML)")
    root = ET.fromstring(raw)
    groove = root.find('Groove')
    clip = groove.find('Clip/Value/MidiClip')
    def gval(tag):
        e = groove.find(tag)
        return None if e is None else e.get('Value')
    loop = clip.find('Loop')
    start = float(loop.find('LoopStart').get('Value'))
    end = float(loop.find('LoopEnd').get('Value'))
    notes = [(float(ev.get('Time')) - start, float(ev.get('Velocity', 100)))
             for ev in clip.iter('MidiNoteEvent') if ev.get('IsEnabled', 'true') == 'true']
    notes = sorted((t, v) for t, v in notes if 0 <= t < end - start + 1e-6)
    if not notes:
        raise ValueError("no notes")
    bases = {0: 1.0, 1: 0.5, 2: 1/3, 3: 0.25, 4: 1/6, 5: 0.125}
    code = gval('Grid')
    if code is None or int(code) not in bases:
        raise ValueError(f"unknown Grid {code}")
    return dict(notes=notes, length=end - start, base=bases[int(code)], base_code=code,
                name=(groove.find('Name').get('Value') if groove.find('Name') is not None else None),
                amounts={k: gval(k) for k in ('QuantizationAmount', 'TimingAmount', 'RandomAmount', 'VelocityAmount')})

ABBR = {'16ths': '16', '16th': '16', '8ths': '8', '4ths': '4', 'Accent': 'Acc', 'Utility': ''}

def short_name(path):
    """'Swing MPC 3000 16ths 54.agr' -> 'MPC 16 54'; drops tempos ('127 bpm') and model numbers;
    at most 20 characters (the GROOVE page's bottom line)."""
    stem = re.sub(r'\.(agr|midi?)$', '', path.split('/')[-1], flags=re.I)
    stem = re.sub(r'\s\d+ bpm$', '', stem)
    toks = [t for t in stem.split() if not re.fullmatch(r'\d{3,}', t)]
    if toks and toks[0] == 'Swing':
        toks = toks[1:]
    toks = [ABBR.get(t, t) for t in toks]
    out = ' '.join(t for t in toks if t)
    return out[:20]

def read_mid(path, base=None, beats=None):
    """A Standard MIDI File (format 0 or 1): every note-on's time (beats) and velocity, all tracks
    and channels together. Base: given, or from the name ('8ths' 1/8, '4ths' 1/4, '16T' 1/16T,
    '8T' 1/8T, otherwise 1/16). Length: given, or the end of the file (End of Track, or the last
    note) rounded up to whole bars of the file's first time signature (4/4 when it has none)."""
    d = open(path, 'rb').read()
    if d[:4] != b'MThd':
        raise ValueError("not a Standard MIDI File")
    hlen = int.from_bytes(d[4:8], 'big')
    ntrk, div = int.from_bytes(d[10:12], 'big'), int.from_bytes(d[12:14], 'big')
    if div & 0x8000:
        raise ValueError("SMPTE time division is not supported")
    pos, notes, end, num, den = 8 + hlen, [], 0, None, None
    for _ in range(ntrk):
        if d[pos:pos + 4] != b'MTrk':
            raise ValueError("bad track chunk")
        tlen = int.from_bytes(d[pos + 4:pos + 8], 'big')
        i, stop, t, run = pos + 8, pos + 8 + tlen, 0, 0
        while i < stop:
            delta = 0
            while True:
                c = d[i]; i += 1
                delta = delta << 7 | c & 0x7f
                if not c & 0x80:
                    break
            t += delta
            st = d[i]
            if st & 0x80:
                i += 1
            else:
                st = run                      # running status
            if st == 0xff:
                kind = d[i]; i += 1
                ln = 0
                while True:
                    c = d[i]; i += 1
                    ln = ln << 7 | c & 0x7f
                    if not c & 0x80:
                        break
                if kind == 0x58 and num is None:
                    num, den = d[i], 2 ** d[i + 1]
                if kind == 0x2f:
                    end = max(end, t)
                i += ln
            elif st in (0xf0, 0xf7):
                ln = 0
                while True:
                    c = d[i]; i += 1
                    ln = ln << 7 | c & 0x7f
                    if not c & 0x80:
                        break
                i += ln
            else:
                run = st
                n = 1 if st & 0xf0 in (0xc0, 0xd0) else 2
                a = d[i:i + n]; i += n
                if st & 0xf0 == 0x90 and a[1] > 0:
                    notes.append((t / div, float(a[1])))
        pos = stop
    if not notes:
        raise ValueError("no notes")
    bar = (num or 4) * 4 / (den or 4)
    last = max(end / div, max(t for t, _ in notes) + 1e-6)
    length = beats if beats else bar * max(1, -(-round(last * 1000) // round(bar * 1000)))
    stem = os.path.basename(path).lower()
    if base is None:
        tok = lambda w: re.search(r'(?<![0-9])' + w + r'(?![a-z])', stem)
        base = (1/6 if tok('16t') else 1/3 if tok('8t') else 0.5 if tok('8ths?') else
                1.0 if tok('4ths?') else 0.25)
    return dict(notes=sorted(notes), length=length, base=base, base_code=f"{base:g} beat",
                amounts={}, name=None)

def table_agr(path, name=None, base=None, beats=None):
    g = read_mid(path, base, beats) if path.lower().endswith(('.mid', '.midi')) else read_agr(path)
    L, b = g['length'], g['base']
    lines = max(1, round(L / b))
    disp, gvel = [0.0] * (lines + 1), [0] * (lines + 1)
    for k in range(lines):
        near = [(abs(t - k * b), t, v) for t, v in g['notes'] if abs(t - k * b) <= b / 2 + 1e-9]
        if near:
            _, t, v = min(near)
            disp[k], gvel[k] = t - k * b, int(round(v))
    disp[lines] = disp[0]                               # the loop: the end maps like the start
    n = min(STEPS, max(1, round(L * 4)))
    if round(L * 4) > STEPS:
        print(f"warning: {path}: {L} beats is more than {STEPS} steps; cut to {STEPS}", file=sys.stderr)
    off, vel = [], []
    for s in range(n):
        t = s * 0.25
        k = min(int(t // b + 1e-9), lines - 1)
        f = (t - k * b) / b
        d = disp[k] + (disp[k + 1] - disp[k]) * f
        off.append(max(-127, min(127, round(d * 96))))
        vel.append(gvel[k] if abs(f) < 1e-9 else 0)
    nm = (name or short_name(path)).upper()[:20]
    return dict(name=nm, len=n, vmax=max(vel), off=off, vel=vel, src=path, info=g)

def c_table(tabs):
    out = ["/* generated by tools/groove2h.py: do not edit */", "#include \"../../lib/groove.h\"", "",
           "static const struct gen_groove grooves[] = {"]
    for t in tabs:
        reps = lambda xs: [xs[i % t['len']] for i in range(STEPS)]
        out.append(f"    /* {t['src']} */")
        out.append(f"    {{ \"{t['name']}\", {t['len']}, {t['vmax']},")
        out.append("      { " + ", ".join(map(str, reps(t['off']))) + " },")
        out.append("      { " + ", ".join(map(str, reps(t['vel']))) + " } },")
    out += ["};", f"#define N_GROOVES {len(tabs)}", ""]
    return "\n".join(out)

def main(a):
    if a[:1] == ['--dump']:
        for p in a[1:]:
            t = table_agr(p)
            g = t['info']
            print(f"{p}: {len(g['notes'])} notes over {g['length']} beats, Base code {g['base_code']} -> {g['base']} beat")
            print("  amounts", g['amounts'])
            print("  notes  ", " ".join(f"{tm:.3f}:{v:.0f}" for tm, v in g['notes'][:24]))
            print(f"  table  {t['name']} len {t['len']} vmax {t['vmax']}")
            print("  off    ", t['off'])
            print("  vel    ", t['vel'])
        return
    swings, files, base, beats = [], [], None, None
    i = 0
    while i < len(a):
        if a[i] == '--swing':
            swings = [int(x) for x in a[i + 1].split(',') if x]; i += 2
        elif a[i] == '--base':                  # for .mid: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32
            base = {'1/4': 1.0, '1/8': 0.5, '1/8T': 1/3, '1/16': 0.25, '1/16T': 1/6, '1/32': 0.125}[a[i + 1]]; i += 2
        elif a[i] == '--beats':                 # for .mid: the groove's length in beats
            beats = float(a[i + 1]); i += 2
        else:
            files.append(a[i]); i += 1
    paths = []
    for f in files:
        if os.path.isdir(f):
            for dp, dn, fn in sorted(os.walk(f)):
                paths += [os.path.join(dp, x) for x in sorted(fn) if x.lower().endswith(('.agr', '.mid', '.midi'))]
        else:
            paths.append(f)
    tabs = [table_swing(s) for s in swings]
    for p in paths:
        if re.match(r'Quantize ', os.path.basename(p)):
            continue                      # Live's quantize-only grooves: QUANT does that
        try:
            tabs.append(table_agr(p, base=base, beats=beats))
        except (ValueError, ET.ParseError) as e:
            print(f"skipped {p}: {e}", file=sys.stderr)
    names = {}
    for t in tabs:
        names.setdefault(t['name'], []).append(t['src'])
    for n, srcs in names.items():
        if len(srcs) > 1:
            print(f"warning: name {n!r} used {len(srcs)} times: {srcs}", file=sys.stderr)
    for i, t in enumerate(tabs):
        print(f"{i + 1:3d} {t['name']:<20} len {t['len']:2d}  {t['src']}", file=sys.stderr)
    print(c_table(tabs))

main(sys.argv[1:])
