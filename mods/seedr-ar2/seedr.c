/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seedr (trig-seedr): a Hapax-style GENERATOR for the Analog Rytm MKII (OS 1.74), on the current track.
 *
 *   TRIG held + PAGE  open / close (closing keeps an audition's result). Stock uses every
 *                     FUNC + key (FUNC + PLAY MODE is its KIT menu); TRIG + PAGE does nothing
 *                     in stock 1.74 (checked in analog-emu)
 *   PAGE              VIEW -> GEN 1 -> GEN 2 -> GEN 3 -> GROOVE -> ROTATE -> VIEW
 *   VIEW              read-only steps: LEFT / RIGHT a step, UP / DOWN 16 steps
 *   GEN 1..3          knobs A..H set the parameters; FUNC + knob sets a range's low end.
 *                     GEN 3: sample locks (SMP% of the trigs, slots SMPL)
 *   GROOVE            a groove (tools/groove2h.py: swing amounts, Ableton .agr files) on the
 *                     track's trigs: µtime and velocity, after Live's Groove Pool
 *   YES               generate / apply the groove. The first YES snapshots the track (the
 *                     audition); every YES after it starts again from that original. A YES
 *                     with no knob turned since the page's last YES moves to the next seed
 *                     (a new take); after a knob turn it keeps the seed, to hear that change. YES on
 *                     another page keeps the audition first, so a GEN take can be grooved and
 *                     NO then undoes only the groove
 *   ROTATE            moves the track's steps (or one field) by STEPS, on this track or on
 *                     every track of the pattern; whole steps through stock's trig shift, so
 *                     their locks move with them
 *   NO                while auditioning: back to the original; otherwise: close
 *   FUNC + YES        keep the result (ends the audition)
 *
 * The audition also ends, keeping the result, when the pattern or the track changes. The
 * snapshot is RAM only. Parameters: docs/requirements.md; algorithm: lib/gen.c;
 * writes go through the stock step routines (device/rytm_ar2.h).
 */
#include <stdint.h>
#include "analog-rytm-mk2/screen.h"
#include "analog-rytm-mk2/store.h"
#include "analog-rytm-mk2/load.h"
#include "../../device/rytm_ar2.h"
/* the groove tables: tools/groove2h.py. grooves-live.h (from your own Ableton Live grooves) is
 * kept out of the repository; without it the swing-only grooves-swing.h is used */
#if __has_include("grooves-live.h")
#include "grooves-live.h"
#else
#include "grooves-swing.h"
#endif

#define K_FUNC 0x80
#define K_TRIG 0x30
#define K_PAGE 0x84
#define K_LEFT 0x40
#define K_RIGHT 0x41
#define K_UP 0x42
#define K_DOWN 0x43
#define K_YES 0x44
#define K_NO 0x45
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))
#define DOWN_BIT 1

/* GCC calls memcpy for large struct copies even when freestanding, and core exports none by
 * that name: a plain one, kept from being turned back into a memcpy call. */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void *memcpy(void *dst, const void *src, unsigned long n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

enum { P_VIEW, P_GEN1, P_GEN2, P_GEN3, P_GROOVE, P_ROT, P_COUNT };
enum { A_NONE, A_GEN, A_GROOVE, A_ROT };

static volatile int open, func_held, trig_held, page, cursor;
static struct gen_track trk, work, orig;
static struct gen_params prm;
static int seed_ui = 1;                 /* -1 = FREE */
static int len_def = 1;                 /* LEN: leave the track default */
static int inited, auditioning, last_written, last_seed, last_enc, last_delta;
static struct gen_groove_params gprm;
static int gsel, gseed_ui = 1;          /* the groove; its seed (-1 = FREE) */
static uint8_t orig_defvel;
static int gen_dirty = 1, grv_dirty = 1;  /* a knob turned since that page's last YES */
static const char *msg;
static unsigned msg_until, ticks;
/* the audition's snapshots: one per track it covers (ROTATE ALL: every track) */
static struct rt_snap snaps[13];
static unsigned aud_mask, aud_pat;
static int aud_track;
/* ROTATE */
static int rot_steps = 1, rot_all, rot_what, rot_grid;
static char status[32];
static void say(const char *m);
#define RT_SHIFT ((void (*)(unsigned, int, int))0x400c1d7a)   /* (track, +-1, length or -1) */

static const char *const names[13] = {
    "BD", "SD", "RS", "CP", "BT", "LT", "MT", "HT", "CH", "OH", "CY", "CB", "FX"
};
static const uint8_t prob[22] = {
    1, 3, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99, 100
};
static const char *const special[8] = { "FILL", "!FIL", "PRE", "!PRE", "NEI", "!NEI", "1ST", "!1ST" };
static const char *const rates[17] = {
    "1/1", "1/2", "1/3", "1/4", "1/5", "1/6", "1/8", "1/10", "1/12", "1/16", "1/20", "1/24",
    "1/32", "1/40", "1/48", "1/64", "1/80"
};
static const char *const ctypes[5] = { "X%", "A:B", "1ST", "FILL", "MIX" };

/* ---- text ---- */

static char *put(char *o, const char *s)
{
    while (*s)
        *o++ = *s++;
    return o;
}

static char *num(char *o, int v)
{
    char t[8];
    int n = 0;
    unsigned u;
    if (v < 0) {
        *o++ = '-';
        u = (unsigned)-v;
    } else
        u = (unsigned)v;
    do t[n++] = (char)('0' + u % 10); while ((u /= 10) && n < 8);
    while (n)
        *o++ = t[--n];
    return o;
}

static char *snum(char *o, int v)
{
    if (v > 0)
        *o++ = '+';
    return num(o, v);
}

static char *hex4(char *o, unsigned v)
{
    int i;
    for (i = 12; i >= 0; i -= 4)
        *o++ = "0123456789ABCDEF"[(v >> i) & 15];
    return o;
}

static char *cond_text(char *o, int c)
{
    int b, base;
    if (c < 0)
        return put(o, "-");
    if (c <= 21) {
        o = num(o, prob[c]);
        return put(o, "%");
    }
    if (c <= 29)
        return put(o, special[c - 22]);
    for (b = 8; b >= 2; b--) {
        base = 29 + b * (b - 1) / 2;
        if (c >= base && c < base + b) {
            o = num(o, c - base + 1);
            *o++ = ':';
            return num(o, b);
        }
    }
    return put(o, "?");
}

static char *val(char *o, const char *label, int v)
{
    o = put(o, label);
    return v == GEN_DEFAULT ? put(o, "-") : num(o, v);
}

static char *range(char *o, int lo, int hi)
{
    if (lo == hi)
        return num(o, lo);
    o = num(o, lo);
    *o++ = '-';
    return num(o, hi);
}

/* a LEN code as stock's stepped scale has it (libanalogrytm pattern.h): in 1/16 of a step, then
 * as a note value where it is one (14 = 1/16, 30 = 1/8 ... 126 = 8/1), else in steps (1.25) */
static unsigned len_units(int c)
{
    static const uint8_t lo[8] = { 0, 14, 30, 46, 62, 78, 94, 110 };
    static const uint16_t base[8] = { 2, 16, 32, 64, 128, 256, 512, 1024 };
    int i = 7;
    while (i > 0 && c < lo[i])
        i--;
    return base[i] + (unsigned)(c - lo[i]) * (i ? 1u << (i - 1) : 1u);
}

static char *len_text(char *o, int c)
{
    static const char *const names[11] = {
        "1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "8/1"
    };
    unsigned u, k;
    if (c >= 127)
        return put(o, "INF");
    u = len_units(c < 0 ? 0 : c);
    for (k = 0; k < 11; k++)
        if (u == 2u << k)
            return put(o, names[k]);
    o = num(o, (int)(u / 16));                 /* steps, two decimals */
    k = ((u % 16) * 100 + 8) / 16;          /* rounded: 15/16 = .94 */
    *o++ = '.';
    *o++ = (char)('0' + k / 10);
    *o++ = (char)('0' + k % 10);
    return o;
}

/* ---- pixels (elekloader examples/screen-check-ar2) ---- */

static uint8_t *px(void *bmp)
{
    return *(uint8_t **)((uint8_t *)bmp + 16);
}

static void dot(uint8_t *p, int x, int r)
{
    if (x >= 0 && x < 128 && r >= 0 && r < 64)
        p[8 * x + 7 - r / 8] |= (uint8_t)(1 << (r % 8));
}

static void box(uint8_t *p, int x0, int r0, int x1, int r1, int fill)
{
    int x, r;
    for (x = x0; x <= x1; x++)
        for (r = r0; r <= r1; r++)
            if (fill || x == x0 || x == x1 || r == r0 || r == r1)
                dot(p, x, r);
}

static void text_at(void *bmp, int x, int top, const char *s, int flags)
{
    int h = core_font_height(CORE_FONT_MAIN);
    core_text_draw(bmp, CORE_FONT_MAIN, x, 64 - h - top, CORE_ALIGN_LEFT | flags, s);
}

/* ---- parameters ---- */

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void init(void)
{
    gen_defaults(&prm);
    prm.vel_lo = 60;
    prm.vel_hi = 120;
    gprm.timing = 100;
    inited = 1;
}

static int grid_next(int g, int d)
{
    static const int gs[4] = { 1, 2, 4, 8 };
    int i = g == 2 ? 1 : g == 4 ? 2 : g == 8 ? 3 : 0;
    return gs[clampi(i + (d > 0 ? 1 : -1), 0, 3)];
}

/* knob k (0..7) turned by d on the current GEN page; lo: FUNC held (a range's low end) */
static void turn(int k, int d, int lo)
{
    int *a, *b;
    if (page == P_GROOVE)
        grv_dirty = 1;
    else
        gen_dirty = 1;
    if (page == P_GEN1) {
        switch (k) {
        case 0: prm.grid = grid_next(prm.grid, d); break;
        case 1: prm.dens = clampi(prm.dens + d, 0, 100); break;
        case 2:
            a = lo ? &prm.vel_lo : &prm.vel_hi;
            *a = clampi(*a + d, 1, 127);
            if (prm.vel_lo > prm.vel_hi) { if (lo) prm.vel_hi = prm.vel_lo; else prm.vel_lo = prm.vel_hi; }
            break;
        case 3:
            if (len_def) {
                if (d > 0) { len_def = 0; prm.len_lo = prm.len_hi = 14; }
                break;
            }
            a = lo ? &prm.len_lo : &prm.len_hi;
            if (*a + d < 0) { len_def = 1; break; }
            *a = clampi(*a + d, 0, 127);
            if (prm.len_lo > prm.len_hi) { if (lo) prm.len_hi = prm.len_lo; else prm.len_lo = prm.len_hi; }
            {
                char *o = put(status, "LEN ");
                o = len_text(o, prm.len_lo);
                if (prm.len_hi != prm.len_lo) {
                    o = put(o, " TO ");
                    o = len_text(o, prm.len_hi);
                }
                *o = 0;
                say(status);
            }
            break;
        case 4:
            a = lo ? &prm.note_lo : &prm.note_hi;
            *a = clampi(*a + d, -24, 24);
            if (prm.note_lo > prm.note_hi) { if (lo) prm.note_hi = prm.note_lo; else prm.note_lo = prm.note_hi; }
            break;
        case 5: prm.utim = clampi(prm.utim + d, 0, 100); break;
        case 6: seed_ui = clampi(seed_ui + (lo ? 100 * d : d), -1, 9999); break;
        case 7: prm.rot = clampi(prm.rot + d, -63, 63); break;
        }
    } else if (page == P_GEN2) {
        switch (k) {
        case 0: prm.acc = clampi(prm.acc + d, 0, 100); break;
        case 1: prm.cond = clampi(prm.cond + d, 0, 100); break;
        case 2: prm.ctyp = (enum gen_ctyp)clampi((int)prm.ctyp + (d > 0 ? 1 : -1), GEN_CT_PROB, GEN_CT_MIX); break;
        case 3:
            if (prm.ctyp == GEN_CT_AB) {
                prm.cval_lo = clampi(prm.cval_lo + (d > 0 ? 1 : -1), 2, 8);
                break;
            }
            a = lo ? &prm.cval_lo : &prm.cval_hi;
            b = lo ? &prm.cval_hi : &prm.cval_lo;
            *a = clampi(*a + d, 0, 21);
            if (prm.cval_lo > prm.cval_hi) *b = *a;
            break;
        case 4: prm.rtrg = clampi(prm.rtrg + d, 0, 100); break;
        case 5:
            a = lo ? &prm.rate_lo : &prm.rate_hi;
            *a = clampi(*a + (d > 0 ? 1 : -1), 0, 16);
            if (prm.rate_lo > prm.rate_hi) { if (lo) prm.rate_hi = prm.rate_lo; else prm.rate_lo = prm.rate_hi; }
            break;
        case 6: prm.rcur = clampi(prm.rcur + d, -128, 127); break;
        default: break;
        }
    } else if (page == P_GEN3) {
        switch (k) {
        case 0: prm.smp = clampi(prm.smp + d, 0, 100); break;
        case 1:
            a = lo ? &prm.smp_lo : &prm.smp_hi;
            *a = clampi(*a + d, 0, 127);
            if (prm.smp_lo > prm.smp_hi) { if (lo) prm.smp_hi = prm.smp_lo; else prm.smp_lo = prm.smp_hi; }
            break;
        default: break;
        }
    } else if (page == P_ROT) {
        switch (k) {
        case 0: rot_steps = clampi(rot_steps + d, -63, 63); break;
        case 1: rot_all = d > 0; break;
        case 2: rot_what = clampi(rot_what + (d > 0 ? 1 : -1), GEN_W_ALL, GEN_W_ACC); break;
        case 3: rot_grid = d > 0; break;
        default: break;
        }
    } else if (page == P_GROOVE) {
        switch (k) {
        case 0: gsel = clampi(gsel + (d > 0 ? 1 : -1), 0, N_GROOVES - 1); break;
        case 1: gprm.timing = clampi(gprm.timing + d, -100, 100); break;
        case 2: gprm.velocity = clampi(gprm.velocity + d, -100, 100); break;
        case 3: gprm.quant = clampi(gprm.quant + d, 0, 100); break;
        case 4: gprm.random = clampi(gprm.random + d, 0, 100); break;
        case 5: gprm.shift = clampi(gprm.shift + d, -63, 63); break;
        case 6: gseed_ui = clampi(gseed_ui + (lo ? 100 * d : d), -1, 9999); break;
        default: break;
        }
    }
}

static void say(const char *m)
{
    msg = m;
    msg_until = ticks + 40;            /* two seconds at the Rytm's 20 Hz tick */
}

/* ---- actions ---- */

static void keep(void);

/* Put every snapshotted track back (each take starts from the true original). */
static void restore_all(void)
{
    int t;
    for (t = 0; t < 13; t++)
        if (aud_mask & (1u << t))
            rt_snap_restore(&snaps[t]);
}

/* Start or continue an audition of kind `kind` over the tracks in `mask` of the current pattern:
 * the first call snapshots them, later ones put the originals back first. An audition of another
 * kind, or over other tracks, is kept first (its result becomes the new original). */
static int audition_tracks(int kind, unsigned mask)
{
    unsigned obj, pat;
    int n = rt_current(&obj, &pat), t;
    if (auditioning && (auditioning != kind || aud_mask != mask))
        keep();
    if (auditioning) {
        restore_all();
        return 1;
    }
    for (t = 0; t < 13; t++) {
        if (!(mask & (1u << t)))
            continue;
        if (!rt_snap_take(&snaps[t], RT_TRACK_OBJ(pat, t), pat, t)) {
            say("NO TRACK DATA");
            while (--t >= 0)
                snaps[t].valid = 0;
            return 0;
        }
    }
    aud_mask = mask;
    aud_pat = pat;
    aud_track = n;
    auditioning = kind;
    return 1;
}

/* the current track only, as GEN and GROOVE use it; reads its original into orig */
static int audition(int kind, unsigned *objp)
{
    unsigned obj, pat;
    int n = rt_current(&obj, &pat);
    if (n == 12) {
        say("NOT ON THE FX TRACK");
        return 0;
    }
    if (rt_euclid(obj)) {
        say("EUCLID IS ON: TURN IT OFF");
        return 0;
    }
    if (!audition_tracks(kind, 1u << n))
        return 0;
    if (!rt_read(obj, &orig, &orig_defvel)) {
        say("NO TRACK DATA");
        return 0;
    }
    /* orig must be the snapshot's state: on a continued audition the track was just restored */
    *objp = obj;
    return 1;
}

/* ROTATE: the current track, or every track (Euclid tracks are skipped) */
static void rotate(void)
{
    unsigned obj, pat;
    int n = rt_current(&obj, &pat), t, done = 0;
    unsigned mask = rot_all ? 0x1fffu : 1u << n;
    if (!rot_all && rt_euclid(obj)) {
        say("EUCLID IS ON: TURN IT OFF");
        return;
    }
    if (!audition_tracks(A_ROT, mask))
        return;
    for (t = 0; t < 13; t++) {
        unsigned o;
        int len, k, i;
        if (!(mask & (1u << t)))
            continue;
        o = RT_TRACK_OBJ(pat, t);
        if (rt_euclid(o))
            continue;
        len = RT_TRACK_LENGTH(o);
        if (len < 1 || len > 64)
            continue;
        if (rot_what == GEN_W_ALL) {
            /* stock's trig shift: one step a call, wraps at the length, carries the step's
             * fields and locks, the swing bits stay */
            k = ((rot_steps % len) + len) % len;
            if (k > len / 2)
                for (i = 0; i < len - k; i++)
                    RT_SHIFT(o, -1, -1);
            else
                for (i = 0; i < k; i++)
                    RT_SHIFT(o, 1, -1);
        } else {
            if (!rt_read(o, &work, 0))
                continue;
            gen_rotate(&work, 0, work.length, rot_steps, (enum gen_what)rot_what,
                       rot_grid ? GEN_R_GRID : GEN_R_TRIG);
            rt_write(o, &work);
        }
        rt_changed(o);
        done++;
    }
    if (rot_all) {
        char *o = put(status, "ROTATED ");
        o = num(o, done);
        o = put(o, " TRACKS");
        *o = 0;
        say(status);
    }
    core_project_edited();
}

static void groove(void)
{
    unsigned obj;
    if (!audition(A_GROOVE, &obj))
        return;
    work = orig;
    if (!grv_dirty && gseed_ui >= 0)
        gseed_ui = gseed_ui < 9999 ? gseed_ui + 1 : 0;   /* YES again: the next seed */
    grv_dirty = 0;
    gprm.seed = gseed_ui >= 0 ? (uint32_t)gseed_ui : (core_block ^ (ticks << 16)) % 10000u;
    last_seed = (int)gprm.seed;
    gen_groove_apply(&work, 0, work.length, &grooves[gsel], &gprm, orig_defvel);
    last_written = rt_write(obj, &work);
    core_project_edited();
}

static void generate(void)
{
    unsigned obj;
    if (prm.smp > 0) {
        rt_current(&obj, 0);
        if (!rt_lock_room(obj, RT_SAMPLE_SLOT)) {
            say("NO LOCK ROWS LEFT");   /* the pattern's 72: a new lock would be lost at the save */
            return;
        }
    }
    if (!audition(A_GEN, &obj))
        return;
    work = orig;
    prm.len_lo = len_def ? (int)GEN_DEFAULT : prm.len_lo;
    prm.len_hi = len_def ? (int)GEN_DEFAULT : prm.len_hi;
    if (!gen_dirty && seed_ui >= 0)
        seed_ui = seed_ui < 9999 ? seed_ui + 1 : 0;      /* YES again: the next seed */
    gen_dirty = 0;
    prm.seed = seed_ui >= 0 ? (uint32_t)seed_ui : (core_block ^ (ticks << 16)) % 10000u;
    last_seed = (int)prm.seed;
    gen_generate(&work, &prm);
    last_written = rt_write(obj, &work);
    if (len_def) {
        prm.len_lo = prm.len_hi = 14;
    }
    core_project_edited();
}

static void drop_snaps(void)
{
    int t;
    for (t = 0; t < 13; t++)
        snaps[t].valid = 0;
    aud_mask = 0;
}

static void revert(void)
{
    if (auditioning) {
        restore_all();
        core_project_edited();
        say("BACK TO THE ORIGINAL");
    }
    auditioning = 0;
    drop_snaps();
}

static void keep(void)
{
    if (auditioning)
        say("KEPT");
    auditioning = 0;
    drop_snaps();
}

/* ---- events ---- */

int gr_key(void *brain, void *ev)
{
    int k = EV_INT(ev, 12), down = EV_INT(ev, 16) & DOWN_BIT;
    (void)brain;
    if (!inited)
        init();
    if (k == K_FUNC)
        func_held = down;                       /* FUNC always goes on to stock too */
    if (k == K_TRIG)
        trig_held = down;                       /* TRIG too (stock opens its TRIG page) */
    if (k == K_PAGE && trig_held) {
        if (down) {
            open = !open;
            if (!open)
                keep();
        }
        return 1;
    }
    if (!open)
        return 0;
    switch (k) {
    case K_PAGE:
        if (down)
            page = (page + 1) % P_COUNT;
        return 1;
    case K_YES:
        if (down && page != P_VIEW) {
            if (func_held)
                keep();
            else if (page == P_GROOVE)
                groove();
            else if (page == P_ROT)
                rotate();
            else
                generate();
        }
        return 1;
    case K_NO:
        if (down) {
            if (auditioning)
                revert();
            else
                open = 0;
        }
        return 1;
    case K_LEFT: case K_RIGHT: case K_UP: case K_DOWN:
        if (page != P_VIEW)
            return 0;
        if (down)
            cursor = (cursor + GEN_STEPS + (k == K_LEFT ? -1 : k == K_RIGHT ? 1 : k == K_UP ? -16 : 16)) % GEN_STEPS;
        return 1;
    default:
        return 0;
    }
}

/* The Rytm's knobs as stock takes them (analog-emu's measurements, OS 1.73): ev_enc brings raw
 * counts (knob 0..7 = A..H at ev+12, not 1..8 as on the Digitone); a knob idle for about 2 s is
 * asleep and its first two turn events only wake it; then 4 counts make one value step, so a
 * faster turn moves further. */
#define KNOB_AWAKE_TICKS 40               /* 2 s at the 20 Hz tick: stock keeps a knob awake 2..4 s */
static unsigned knob_last[8];
static int knob_wake[8], knob_acc[8];

int gr_enc(void *brain, void *ev)
{
    int e = EV_INT(ev, 12), d = EV_INT(ev, 16), steps;
    (void)brain;
    if (!open || page == P_VIEW)
        return 0;
    last_enc = e;
    last_delta = d;
    if (e < 0 || e > 7 || !d)
        return 1;
    if (!knob_last[e] || ticks - knob_last[e] > KNOB_AWAKE_TICKS) {
        knob_wake[e] = 2;                 /* asleep: the next two turns wake it */
        knob_acc[e] = 0;
    }
    knob_last[e] = ticks ? ticks : 1;
    if (knob_wake[e] > 0) {
        knob_wake[e]--;
        return 1;
    }
    if ((knob_acc[e] > 0 && d < 0) || (knob_acc[e] < 0 && d > 0))
        knob_acc[e] = 0;                  /* a change of direction starts afresh */
    knob_acc[e] += d;
    steps = knob_acc[e] / 4;
    knob_acc[e] -= steps * 4;
    while (steps > 0) { turn(e, 1, func_held); steps--; }
    while (steps < 0) { turn(e, -1, func_held); steps++; }
    return 1;
}

void gr_tick(void *ctrl)
{
    ticks++;
    if (auditioning) {
        unsigned obj, pat;
        int n = rt_current(&obj, &pat);
        if (n != aud_track || pat != aud_pat)
            keep();                             /* another track or pattern: the result stays */
    }
    if (open)
        *((volatile uint8_t *)ctrl + 0x20) = 1;
}

/* ---- drawing ---- */

static void draw_title(void *bmp, uint8_t *p, const char *what, int n)
{
    char line[40], *o = line;
    box(p, 0, 0, 127, 9, 1);
    o = put(o, what);
    *o++ = ' ';
    o = put(o, names[n >= 0 && n < 13 ? n : 12]);
    if (auditioning)
        o = put(o, "  AUDITION");
    *o = 0;
    text_at(bmp, 2, 1, line, CORE_TEXT_INVERT);
}

static void draw_view(void *bmp, uint8_t *p, int n)
{
    char line[48], *o;
    uint8_t defvel = 0;
    unsigned obj;
    int i, pg;
    const struct gen_step *s;

    rt_current(&obj, 0);
    if (!rt_read(obj, &trk, &defvel))
        return;
    if (cursor >= trk.length)
        cursor = trk.length - 1;
    o = put(line, "STEPS");
    *o = 0;
    draw_title(bmp, p, line, n);
    o = put(line, "S");
    o = num(o, cursor + 1);
    *o++ = '/';
    o = num(o, trk.length);
    *o = 0;
    text_at(bmp, 92, 1, line, CORE_TEXT_INVERT);

    pg = cursor / 16 * 16;
    for (i = 0; i < 16; i++) {
        int x = 2 + 8 * i, st = pg + i;
        if (st >= trk.length)
            break;
        s = &trk.s[st];
        if (s->flags & GEN_TRIG) {
            if (s->flags & 0x0380)
                box(p, x, 14, x + 5, 20, 1);
            else
                box(p, x + 2, 16, x + 3, 18, 1);
        }
        if (s->flags & GEN_ACCENT)
            box(p, x, 22, x + 5, 22, 1);
        if (s->flags & GEN_RETRIG)
            box(p, x + 2, 12, x + 3, 12, 1);
        if (st == cursor) {
            box(p, x + 2, 24, x + 3, 24, 1);
            box(p, x + 1, 25, x + 4, 25, 1);
        }
    }
    s = &trk.s[cursor];
    o = val(line, "N", s->note);
    o = val(o, " V", s->vel);
    o = val(o, " L", s->len);
    o = put(o, " U");
    o = snum(o, s->utime);
    *o = 0;
    text_at(bmp, 2, 28, line, 0);
    o = put(line, "C");
    o = cond_text(o, s->cond);
    o = put(o, " R");
    if (s->flags & GEN_RETRIG) {
        o = put(o, rates[s->rrate <= 16 ? s->rrate : 16]);
        *o++ = ' ';
        o = num(o, s->rvel);
    } else
        o = put(o, "-");
    *o = 0;
    text_at(bmp, 2, 39, line, 0);
    o = put(line, "F");
    o = hex4(o, s->flags);
    o = put(o, " DN");
    o = num(o, trk.default_note);
    o = put(o, " DV");
    o = num(o, defvel);
    *o = 0;
    text_at(bmp, 2, 50, line, 0);
}

/* one knob's label and value, in a 32-pixel column, two rows of four */
static void cell(void *bmp, int k, const char *label, const char *value)
{
    int x = 1 + 32 * (k & 3), top = k < 4 ? 12 : 33;
    text_at(bmp, x, top, label, 0);
    text_at(bmp, x, top + 10, value, 0);
}

static char *pcts(char *o, int v)
{
    o = snum(o, v);
    return put(o, "%");
}

static void draw_gen(void *bmp, uint8_t *p, int n)
{
    char v[16], *o;
    static const char *const whats[8] = { "ALL", "VEL", "NOTE", "LEN", "UTIM", "COND", "RTRG", "ACC" };
    draw_title(bmp, p, page == P_GEN1 ? "GEN 1" : page == P_GEN2 ? "GEN 2" : page == P_GEN3 ? "GEN 3" :
               page == P_ROT ? "ROTATE" : "GROOVE", n);
    if (page == P_ROT) {
        o = v; o = snum(o, rot_steps); *o = 0;
        cell(bmp, 0, "STEPS", v);
        cell(bmp, 1, "SCOPE", rot_all ? "ALL" : "TRACK");
        cell(bmp, 2, "WHAT", whats[rot_what & 7]);
        cell(bmp, 3, "MODE", rot_what == GEN_W_ALL || rot_what == GEN_W_ACC ? "STEP" : rot_grid ? "GRID" : "TRIG");
    } else if (page == P_GEN3) {
        o = v; o = num(o, prm.smp); o = put(o, "%"); *o = 0;
        cell(bmp, 0, "SMP%", v);
        o = v;
        if (prm.smp_lo == 0 && prm.smp_hi == 0)
            o = put(o, "OFF");
        else
            o = range(o, prm.smp_lo, prm.smp_hi);
        *o = 0;
        cell(bmp, 1, "SMPL", v);
    } else if (page == P_GROOVE) {
        o = v; o = num(o, gsel + 1); *o++ = '/'; o = num(o, N_GROOVES); *o = 0;
        cell(bmp, 0, "GRV", v);
        o = v; o = pcts(o, gprm.timing); *o = 0;
        cell(bmp, 1, "TIME", v);
        o = v; o = pcts(o, gprm.velocity); *o = 0;
        cell(bmp, 2, "VEL", v);
        o = v; o = num(o, gprm.quant); o = put(o, "%"); *o = 0;
        cell(bmp, 3, "QUANT", v);
        o = v; o = num(o, gprm.random); o = put(o, "%"); *o = 0;
        cell(bmp, 4, "RAND", v);
        o = v; o = snum(o, gprm.shift); *o = 0;
        cell(bmp, 5, "SHIFT", v);
        o = v; o = gseed_ui < 0 ? put(o, "FREE") : num(o, gseed_ui); *o = 0;
        cell(bmp, 6, "SEED", v);
    } else if (page == P_GEN1) {
        o = v; o = put(o, prm.grid == 1 ? "1/16" : prm.grid == 2 ? "1/8" : prm.grid == 4 ? "1/4" : "1/2"); *o = 0;
        cell(bmp, 0, "GRID", v);
        o = v; o = num(o, prm.dens); o = put(o, "%"); *o = 0;
        cell(bmp, 1, "DENS", v);
        o = v; o = range(o, prm.vel_lo, prm.vel_hi); *o = 0;
        cell(bmp, 2, "VEL", v);
        o = v;
        if (len_def)
            o = put(o, "DEF");
        else {
            o = len_text(o, prm.len_lo);
            if (prm.len_hi != prm.len_lo)
                *o++ = '+';                     /* a range: the status line shows both ends */
        }
        *o = 0;
        cell(bmp, 3, "LEN", v);
        o = v;
        if (prm.note_lo == prm.note_hi)
            o = snum(o, prm.note_lo);
        else {
            o = snum(o, prm.note_lo);
            o = snum(o, prm.note_hi);
        }
        *o = 0;
        cell(bmp, 4, "NOTE", v);
        o = v; o = num(o, prm.utim); o = put(o, "%"); *o = 0;
        cell(bmp, 5, "UTIM", v);
        o = v; o = seed_ui < 0 ? put(o, "FREE") : num(o, seed_ui); *o = 0;
        cell(bmp, 6, "SEED", v);
        o = v; o = snum(o, prm.rot); *o = 0;
        cell(bmp, 7, "ROT", v);
    } else {
        o = v; o = num(o, prm.acc); o = put(o, "%"); *o = 0;
        cell(bmp, 0, "ACC", v);
        o = v; o = num(o, prm.cond); o = put(o, "%"); *o = 0;
        cell(bmp, 1, "COND", v);
        cell(bmp, 2, "CTYP", ctypes[prm.ctyp]);
        o = v;
        if (prm.ctyp == GEN_CT_AB) {
            *o++ = '/';
            o = num(o, clampi(prm.cval_lo, 2, 8));
        } else if (prm.ctyp == GEN_CT_PROB || prm.ctyp == GEN_CT_MIX) {
            o = num(o, prob[clampi(prm.cval_lo, 0, 21)]);
            *o++ = '-';
            o = num(o, prob[clampi(prm.cval_hi, 0, 21)]);
        } else
            o = put(o, "-");
        *o = 0;
        cell(bmp, 3, "CVAL", v);
        o = v; o = num(o, prm.rtrg); o = put(o, "%"); *o = 0;
        cell(bmp, 4, "RTRG", v);
        o = v;
        o = put(o, rates[clampi(prm.rate_lo, 0, 16)]);
        if (prm.rate_hi != prm.rate_lo) { *o++ = '-'; o = put(o, rates[clampi(prm.rate_hi, 0, 16)] + 2); }
        *o = 0;
        cell(bmp, 5, "RATE", v);
        o = v; o = snum(o, prm.rcur); *o = 0;
        cell(bmp, 6, "RCUR", v);
    }
    /* the status line */
    o = v;
    if (msg && (int)(msg_until - ticks) > 0) {
        text_at(bmp, 1, 54, msg, 0);
        return;
    }
    if (page == P_GROOVE) {
        char line[40];
        o = put(line, grooves[gsel].name);
        *o = 0;
        text_at(bmp, 1, 54, line, 0);
        return;
    }
    if (auditioning) {
        char line[40];
        o = line;
        if (auditioning != A_ROT) {
            o = put(o, "S");
            o = num(o, last_seed);
            *o++ = ' ';
        }
        o = put(o, "YES NO F+YES");
        *o = 0;
        text_at(bmp, 1, 54, line, 0);
    }
}

void gr_draw(void *bmp, void *ctrl)
{
    uint8_t *p;
    unsigned obj;
    int n, i;
    (void)ctrl;
    if (!open || core_stock_overlay())
        return;
    n = rt_current(&obj, 0);
    p = px(bmp);
    for (i = 0; i < 1024; i++)
        p[i] = 0;
    if (page == P_VIEW)
        draw_view(bmp, p, n);
    else
        draw_gen(bmp, p, n);
}
