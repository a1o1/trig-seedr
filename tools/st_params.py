#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""device/st_params.h from the stock Syntakt OS 1.42 file (plan-syntakt.md task S0.5).

    python3 -I tools/st_params.py STOCK.syx ELEKLOADER_DIR > device/st_params.h

Reads the parameter table of the main OS (section 3, loaded at 0x40000400): 505 entries of 52 bytes at
0x4022d83c, one per stock parameter id (the id stock's lock routines take). An entry is
    +0x00 owner (a machine type 0..0x2c, or a page: 0x2d amp, 0x2f filter, 0x35 / 0x36 the FX track's
          filter / amp, 0x3b / 0x3c LFO1 / LFO2, 0x3d trig, ...)
    +0x08 min, +0x0c max, +0x10 default (8.8 fixed point)
    +0x1c knob code (0x81..0x88: knob A..H on a machine's SYN page)
    +0x28 long name, +0x30 short name (pointers)
and the machine list at 0x4022b228 (long name, short name; the index is the machine type). Which machines
each track offers and which controls each page shows were read from analog-emu's screens
(tools/st_s05.sh). Prints nothing from the file but names, numbers and ranges."""
import hashlib, struct, sys

SHA = 'ded3c59fdb909eb9e2c3307b30a486452c8fa6947b12c14e0ce35a187b7f37c1'
BASE, PTAB, NPAR, MTAB, NMACH = 0x40000400, 0x4022d83c, 505, 0x4022b228, 45

# the machines each kind of track offers in stock's machine menu (FUNC + SYN), in menu order [measured]
DIGITAL = [0, 1, 6, 3, 7, 2, 37, 5, 4, 36, 38, 44, 35]                                   # tracks 1..8
DRUM = [8, 9, 10, 11, 12, 13, 41, 14, 15, 16, 17, 42, 18, 19, 32, 33, 20, 39, 40, 35]      # tracks 9..11
CYMBAL = [22, 23, 26, 27, 31, 43, 24, 28, 30, 25, 29, 32, 33, 35]                          # track 12
MIDI_TYPE, MIDI_VALS = 35, range(0xfe, 0x106)        # the MIDI machine: CC1..CC8 VAL (owner 0x39)

# the shared pages' controls stock shows, as (stock id, name) [measured on tracks 1, 9 and the FX track]
FLTR = [(0x40, 'FATK'), (0x41, 'FDEC'), (0x42, 'FSUS'), (0x43, 'FREL'), (0x3a, 'FRQ'), (0x3c, 'RES'),
        (0x44, 'FENV'), (0x3f, 'FDLY')]
BASEW = [(0x46, 'BASE'), (0x47, 'WDTH')]                                                   # digital only
AMP = [(0x49, 'AATK'), (0x4a, 'AHLD'), (0x4b, 'ADEC'), (0x4e, 'DEL'), (0x4f, 'REV'), (0x50, 'PAN'), (0x51, 'VOL')]
LFO = [(0x56, 'LSPD'), (0x58, 'LFAD'), (0x5c, 'LSPH'), (0x5e, 'LDEP'),
       (0x60, 'L2SP'), (0x62, 'L2FA'), (0x66, 'L2PH'), (0x68, 'L2DP')]
FX_FLTR = [(0x8a, 'FATK'), (0x8b, 'FDEC'), (0x8c, 'FSUS'), (0x8d, 'FREL'), (0x85, 'FRQ'), (0x86, 'RES'),
           (0x88, 'FENV'), (0x89, 'FDLY'), (0x8f, 'FPAN')]
FX_AMP = [(0x96, 'DRV'), (0x91, 'AATK'), (0x92, 'AHLD'), (0x93, 'ADEC'), (0x9b, 'ADEP'), (0x97, 'DEL'),
          (0x98, 'REV'), (0x99, 'PAN'), (0x9a, 'VOL')]
SHORTER = {'RADIO': 'RDIO', 'RATIO': 'RATO', 'DRIVE': 'DRV'}


def main(syx, eldir):
    sys.path.insert(0, eldir)
    from elekloader import syx as S
    raw = open(syx, 'rb').read()
    if hashlib.sha256(raw).hexdigest() != SHA:
        sys.exit('not the stock Syntakt OS 1.42 file')
    d = S.Syx.load(syx).section(3)
    w = lambda a: struct.unpack('>I', d[a - BASE:a - BASE + 4])[0]
    sx = lambda v: v - (1 << 32) if v & 0x80000000 else v

    def s(a):
        i = a - BASE
        return d[i:d.index(b'\0', i)].decode('latin1')

    par = []
    for i in range(NPAR):
        e = PTAB + 52 * i
        par.append(dict(id=i, owner=w(e), lo=sx(w(e + 8)) >> 8, hi=sx(w(e + 12)) >> 8, knob=w(e + 0x1c),
                        name=s(w(e + 0x30)), long=s(w(e + 0x28))))
    mach = [s(w(MTAB + 8 * t)).strip() for t in range(NMACH)]

    def p(i, name=None):
        q = par[i]
        n = name or SHORTER.get(q['name'], q['name'])
        assert len(n) <= 4, n
        return '{ "%s", 0x%03x, %d, %d }' % (n, i, q['lo'], q['hi'])

    def role(n):
        return ('SEED_ROLE_KICK' if n.startswith('BD') else 'SEED_ROLE_BACK' if n[:2] in ('SD', 'RS', 'CP')
                else 'SEED_ROLE_HAT' if n[:2] in ('CH', 'OH', 'HH') else 'SEED_ROLE_PERC' if n[:2] in ('CY', 'CB', 'PC', 'UT')
                else 'SEED_ROLE_METER')

    out = []
    for t in sorted(set(DIGITAL + DRUM + CYMBAL)):
        if t == MIDI_TYPE:
            ps = [par[i] for i in MIDI_VALS]
        else:
            ps = [q for q in par if q['owner'] == t and q['hi'] - q['lo'] > 1]
            ps.sort(key=lambda q: q['knob'] if 0x81 <= q['knob'] <= 0x88 else 0x100 + q['id'])
        names = [SHORTER.get(q['name'], q['name']) for q in ps]
        lane1 = names.index('SMP1') if 'SMP1' in names else 0
        tracks = (0xff if t in DIGITAL else 0) | (0x700 if t in DRUM else 0) | (0x800 if t in CYMBAL else 0)
        out.append('    { %2d, %-15s, 0x%03x, %s, %d, %d, { %s } }, \\' % (
            t, '"%s"' % mach[t], tracks, role(mach[t]), lane1, len(ps), ', '.join(p(q['id']) for q in ps)))

    def page(name, rows):
        return '#define %s { %s }\n#define %s_N %d' % (name, ', '.join(p(i, n) for i, n in rows), name, len(rows))

    print(HEAD % dict(n=len(out), machines='\n'.join(out),
                      pages='\n'.join([page('ST_PAGE_FLTR', FLTR), page('ST_PAGE_BASEW', BASEW), page('ST_PAGE_AMP', AMP),
                                       page('ST_PAGE_LFO', LFO), page('ST_PAGE_FX_FLTR', FX_FLTR),
                                       page('ST_PAGE_FX_AMP', FX_AMP)])))


HEAD = '''/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Syntakt's capabilities as data (OS 1.42; plan-syntakt.md task S0.5): the lockable parameters a
 * lock lane can take, per machine and per page, the lanes' defaults and the fields each track has. No
 * stock routines, so a host tool can use the same tables as the device layer.
 *
 * Made by tools/st_params.py from the stock file (docs/status/S0.5.md has how each fact was found). A
 * parameter's `id` here is stock's own parameter id: the index in the main OS's parameter table, the id
 * its lock routines take (Synfact's st_pattern_lock_get / _set, elekloader/sdk/include/syntakt/pattern.h).
 * Ids run to 504, past struct seed_param's uint8_t, so a track's list for lib/ is built by st_track_params,
 * which gives each entry its place in the list as seed_param.id and the stock ids beside it.
 *
 * Values are a lane's in whole units, the stored value's whole part (8.8 fixed point; TUNE 40..88 is
 * -24..+24 semitones). Left out, as on the Rytm: two-value switches (PNCH, the envelope resets, ROUT)
 * and selectors (filter TYPE; LFO MULT, WAVE, DEST, MODE; amp MODE); the amp page's SUS and REL, which
 * stock 1.42 does not show; the delay and reverb (not on any track's pages); CP CLASSIC, which no machine
 * menu offers. */
#ifndef ST_PARAMS_H
#define ST_PARAMS_H

#include "../lib/seed.h"

#define ST_TRACKS    13
#define ST_FX_TRACK  12
#define ST_DIGITAL   0x0ffu            /* tracks 1..8 (a bit per track) */
#define ST_DRUM      0x700u            /* tracks 9..11, the analog drum voices */
#define ST_CYMBAL    0x800u            /* track 12, the analog cymbal voice */

struct st_param { char name[SEED_NAME]; uint16_t id; int16_t lo, hi; };

/* a machine: its type (the Sound's), name, the tracks whose menu offers it, its sound's role for TOOLS
 * YOLO, lane 1's default (an index in p: SMP1 on SP TWINSHOT, else knob A) and its SYN page's controls,
 * knob A first */
struct st_machine {
    uint8_t type;
    char name[16];
    uint16_t tracks;
    uint8_t role, lane1, n;
    struct st_param p[8];
};

#define ST_NMACHINES %(n)d
#define ST_MACHINES { \\
%(machines)s
}

/* the pages every track with a machine shows (BASEW: digital tracks only), and the FX track's: its FX
 * Drive page (DRV) with its amp page, then its filter; every track has both LFOs. Lane 2's default is
 * the filter's frequency (FRQ). */
%(pages)s

/* the track's fields: no retrigs, accent or slide (the parameter table has none); the FX track has no
 * note or velocity on its TRIG page */
#define ST_VOICE_FIELDS (SEED_F_NOTE | SEED_F_VEL | SEED_F_LEN | SEED_F_UTIME | SEED_F_COND | SEED_F_LOCKS)
#define ST_FX_FIELDS    (SEED_F_LEN | SEED_F_UTIME | SEED_F_COND | SEED_F_LOCKS)
#define ST_UTIME_LO (-23)
#define ST_UTIME_HI 23

/* the machine of a type, or 0 */
static inline const struct st_machine *st_machine_of(const struct st_machine *m, int n, int type)
{
    int i;
    for (i = 0; i < n; i++)
        if (m[i].type == type)
            return &m[i];
    return 0;
}

/* A track's lockable parameters for lib/ (SD4): the machine's controls, then the pages its track shows
 * (m is ignored on the FX track). out[k].id = k; ids[k] = stock's id. Returns the count (at most
 * ST_TRACK_PARAMS_MAX) and sets lane[] to the lanes' defaults. */
#define ST_TRACK_PARAMS_MAX 34
static inline int st_add(struct seed_param *out, uint16_t *ids, int n, const struct st_param *p, int c)
{
    int i, k;
    for (i = 0; i < c; i++, n++) {
        for (k = 0; k < SEED_NAME; k++)
            out[n].name[k] = p[i].name[k];
        out[n].id = (uint8_t)n;
        out[n].lo = p[i].lo;
        out[n].hi = p[i].hi;
        ids[n] = p[i].id;
    }
    return n;
}
static inline int st_track_params(const struct st_machine *m, int track, struct seed_param *out, uint16_t *ids,
                                  int16_t lane[SEED_LANES])
{
    static const struct st_param fltr[] = ST_PAGE_FLTR, basew[] = ST_PAGE_BASEW, amp[] = ST_PAGE_AMP,
                                 lfo[] = ST_PAGE_LFO, fxf[] = ST_PAGE_FX_FLTR, fxa[] = ST_PAGE_FX_AMP;
    int n = 0;
    if (track == ST_FX_TRACK) {
        n = st_add(out, ids, n, fxa, ST_PAGE_FX_AMP_N);
        lane[0] = 0;                   /* DRV */
        lane[1] = (int16_t)(n + 4);    /* FRQ */
        n = st_add(out, ids, n, fxf, ST_PAGE_FX_FLTR_N);
        return st_add(out, ids, n, lfo, ST_PAGE_LFO_N);
    }
    if (m) {
        n = st_add(out, ids, n, m->p, m->n);
        lane[0] = m->lane1;
    } else
        lane[0] = 0;
    lane[1] = (int16_t)(n + 4);        /* FRQ */
    n = st_add(out, ids, n, fltr, ST_PAGE_FLTR_N);
    if (track >= 0 && track < 8)
        n = st_add(out, ids, n, basew, ST_PAGE_BASEW_N);
    n = st_add(out, ids, n, amp, ST_PAGE_AMP_N);
    return st_add(out, ids, n, lfo, ST_PAGE_LFO_N);
}

#endif'''

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
