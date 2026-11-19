/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Syntakt OS 1.42 device layer (docs/plan-syntakt.md task S2). Addresses and offsets:
 * docs/firmware-notes-st142.md; what was measured: docs/status/S2.md. UI task only (ev_key, ev_enc,
 * ev_tick, ev_draw). The counterpart of rytm_ar2.h, with the same shape.
 *
 * st_seed_device() returns the struct seed_dev of design section 10 (lib/seed.h). Its state (the
 * snapshot of every track: 13 x (0x499 + 0x2447) bytes, about 136 KB) is too large for the mods' area
 * (plan SD2), so the mod hands it memory first: st_seed_attach(mem, ST_SEED_STATE_SIZE), from the
 * native heap on the device. Until then every entry returns SEED_E_NA.
 *
 * Lock lanes (SD4): a track's lockable parameters are its machine's controls and the pages its track
 * shows (device/st_params.h). Stock keeps a lock in one of 71 slots per step; a parameter's slot is a
 * byte of its entry in the main OS's parameter table (ST_SLOT). The caps the device returns are the
 * track's last asked about (current, read, write, context). The machine of a track is the mod's to
 * give (st_seed_set_machine); until it does, the track's list is the shared pages only.
 *
 * A host test (device/test_syntakt_st.c) defines ST_HOST and supplies the stock routines and the
 * accessors below them with a model of the track, so the write logic runs under the sanitizers. */
#ifndef SYNTAKT_ST_H
#define SYNTAKT_ST_H

#include <stdint.h>
#include "../lib/seed.h"
#include "st_params.h"

#ifndef ST_HOST
/* ---- stock routines. A routine that returns a pointer returns it in d0, so those are declared
 * as returning an integer (elekloader docs/ADAPTING.md). ---- */
#define ST_APP            ((unsigned (*)(void))0x4016e9c8)                  /* *0x444e23f4 */
#define ST_ACTIVE_PATTERN ((unsigned (*)(unsigned))0x40019000)              /* app + 0x3394 + 0x680 index */
#define ST_ACTIVE_TRACK   ((unsigned (*)(unsigned))0x40019036)              /* a track object */
#define ST_TRACK_LENGTH   ((int (*)(unsigned))0x40027f3a)
/* step writes: (track object, step, value); steps at or past the track's length are refused. Each
 * posts the step's change notice ({0x401c6b9c, step} to the track's vtable +0x10). */
#define ST_PLACE_TRIG     ((void (*)(unsigned, int, int))0x40029f32)        /* a note trig on */
/* (mask, on): or / and-not of flag bits. Setting bit 0 on a step without a trig first resets the
 * step; a step left without a trig is cleared (firmware notes section 4) */
#define ST_SET_FLAG       ((void (*)(unsigned, int, int, int))0x40029db4)
#define ST_SET_NOTE       ((void (*)(unsigned, int, int))0x4002817a)
#define ST_SET_NOT2       ((void (*)(unsigned, int, int))0x400281f4)
#define ST_SET_NOT3       ((void (*)(unsigned, int, int))0x4002826e)
#define ST_SET_NOT4       ((void (*)(unsigned, int, int))0x400282e8)
#define ST_SET_VEL        ((void (*)(unsigned, int, int))0x40028362)
#define ST_SET_LEN        ((void (*)(unsigned, int, int))0x400283dc)
#define ST_SET_UTIME      ((void (*)(unsigned, int, int))0x40028456)        /* value + 64 */
#define ST_SET_COND       ((void (*)(unsigned, int, int))0x40027dec)        /* -1, 0..99 */
#define ST_SET_RLEN       ((void (*)(unsigned, int, int))0x40028574)
#define ST_SET_RRATE      ((void (*)(unsigned, int, int))0x400284ea)
#define ST_SET_RVEL       ((void (*)(unsigned, int, int))0x400285fe)
/* parameter locks, at the lock model: (model, track 0..12, step 0..63, slot 0..70, value as stored,
 * 8.8) and clear (model, track, step, slot); they keep the step's count (word 71) and the track's
 * byte per slot, and post the model's notice */
#define ST_LOCK_SET       ((void (*)(unsigned, int, int, int, int))0x40017602)
#define ST_LOCK_CLEAR     ((void (*)(unsigned, int, int, int))0x4001741e)
/* a stock parameter id's lock slot: byte +7 of its entry in the parameter table (505 entries of 52
 * bytes at 0x4022d83c; S0.5); 255 for a parameter stock does not lock */
#define ST_SLOT(id)       ((id) < 505 ? ((const uint8_t *)0x4022d843)[52 * (id)] : 255)
#else
unsigned st_host_app(void);
unsigned st_host_active_pattern(unsigned app);
unsigned st_host_active_track(unsigned app);
int st_host_track_length(unsigned obj);
void st_host_place_trig(unsigned obj, int i, int on);
void st_host_set_flag(unsigned obj, int i, int mask, int on);
void st_host_set_field(unsigned obj, int i, int value, int offset);
void st_host_lock_set(unsigned model, int track, int i, int k, int value);
void st_host_lock_clear(unsigned model, int track, int i, int k);
int st_host_slot(int id);
#define ST_APP            st_host_app
#define ST_ACTIVE_PATTERN st_host_active_pattern
#define ST_ACTIVE_TRACK   st_host_active_track
#define ST_TRACK_LENGTH   st_host_track_length
#define ST_PLACE_TRIG     st_host_place_trig
#define ST_SET_FLAG       st_host_set_flag
#define ST_SET_NOTE(o, i, v)  st_host_set_field(o, i, v, ST_NOTE)
#define ST_SET_NOT2(o, i, v)  st_host_set_field(o, i, v, ST_NOT2)
#define ST_SET_NOT3(o, i, v)  st_host_set_field(o, i, v, ST_NOT3)
#define ST_SET_NOT4(o, i, v)  st_host_set_field(o, i, v, ST_NOT4)
#define ST_SET_VEL(o, i, v)   st_host_set_field(o, i, v, ST_VEL)
#define ST_SET_LEN(o, i, v)   st_host_set_field(o, i, v, ST_LEN)
#define ST_SET_UTIME(o, i, v) st_host_set_field(o, i, (v) - 64, ST_UTIME)
#define ST_SET_COND(o, i, v)  st_host_set_field(o, i, v, ST_COND)
#define ST_SET_RLEN(o, i, v)  st_host_set_field(o, i, v, ST_RLEN)
#define ST_SET_RRATE(o, i, v) st_host_set_field(o, i, v, ST_RRATE)
#define ST_SET_RVEL(o, i, v)  st_host_set_field(o, i, v, ST_RVEL)
#define ST_LOCK_SET       st_host_lock_set
#define ST_LOCK_CLEAR     st_host_lock_clear
#define ST_SLOT(id)       st_host_slot(id)
#endif

#define ST_RECORD_SIZE 0x499
#define ST_LOCKS_SIZE  0x2447          /* a track's lock store: 64 steps of 72 int16, then a byte per slot */
#define ST_PAT_STRIDE  0x70            /* pattern + 0x70 * (track + 1): a track object */

#define ST_FLAGS   0x000
#define ST_VEL     0x080
#define ST_LEN     0x0c0
#define ST_UTIME   0x100
#define ST_RLEN    0x140
#define ST_RRATE   0x180
#define ST_RVEL    0x1c0
#define ST_COND    0x200
#define ST_B240    0x240               /* not identified (probably the sound lock); moves with the step */
#define ST_B280    0x280               /* not identified; moves with the step */
#define ST_NOTE    0x2c0
#define ST_NOT2    0x300               /* NOT2..NOT4: interval + 64; kept with the step (SD5) */
#define ST_NOT3    0x340
#define ST_NOT4    0x380
#define ST_DEF_VEL  0x3c0
#define ST_DEF_NOTE 0x3c5
#define ST_EUCLID   0x3d1
#define ST_EUCLID_TAB 0x3d8            /* Euclidean mode's pattern, 64 bytes: a step's pulse number, 0xFF none */

#define ST_STEP_LOCKS 72               /* int16 per step: slots 0..70, then the step's lock count */
#define ST_SLOTS 71
#define ST_LOCK_IDS (64 * ST_STEP_LOCKS * 2)   /* the track's byte per slot, after the steps */
/* the flag bits lib/ does not know (above SEED_KNOWN and SEED_SWING): stock's switches and their
 * per-step overrides (firmware notes section 2). A generated trig keeps the ones stock's place set. */
#define ST_STOCK_BITS 0xffc0u

/* The Syntakt's conditions (firmware notes section 3) against lib/seed.h's (the Rytm's): 0..29 are
 * the same; seed's A:B (30..64) are the Syntakt's 32, 33 (B = 2) and 34.. (B >= 3, each followed by
 * its negation). The Syntakt's own LST, !LST and negated A:B have no seed code: they are read as
 * 65..99 (ST_COND_OWN + their order), which lib/ never makes, so they come back as they were. */
#define ST_COND_OWN 65
static inline int st_cond_ab(int a, int b)            /* the Syntakt's code of A:B */
{
    int k = 34, x;
    if (b == 2)
        return 32 + a - 1;
    for (x = 3; x < b; x++)
        k += 2 * x;
    return k + 2 * (a - 1);
}
static inline int st_cond_from_seed(int c)
{
    int b, a = c - 30;
    if (c < 0)
        return -1;
    if (c < 30)
        return c;
    if (c <= 64) {
        for (b = 2; a >= b; b++)
            a -= b;
        return st_cond_ab(a + 1, b);
    }
    if (c == ST_COND_OWN)
        return 30;
    if (c == ST_COND_OWN + 1)
        return 31;
    if (c <= ST_COND_OWN + 34)
        return 35 + 2 * (c - ST_COND_OWN - 2);     /* the negated A:B, in their order */
    return -1;
}
static inline int st_cond_to_seed(int s)
{
    int b, x, k;
    if (s < 0 || s > 99)
        return SEED_NO_COND;
    if (s < 30)
        return s;
    if (s == 30 || s == 31)
        return ST_COND_OWN + s - 30;
    if (s == 32 || s == 33)
        return 30 + s - 32;
    if ((s - 34) & 1)
        return ST_COND_OWN + 2 + (s - 35) / 2;
    for (b = 3, k = 34, x = 32; ; k += 2 * b, x += b, b++)   /* seed: 30 + 2 (B = 2) + 3 + ... */
        if (s < k + 2 * b)
            return x + (s - k) / 2;
}

/* ---- the objects ---- */
static inline unsigned st_track_obj(unsigned pattern, int n)
{
    return pattern + ST_PAT_STRIDE * (unsigned)(n + 1);
}
#ifndef ST_HOST
static inline uint8_t *st_record(unsigned obj)
{
    return *(uint8_t **)(obj + 0x10);
}
static inline unsigned st_lock_model(unsigned obj)
{
    return *(unsigned *)(obj + 0x2c);
}
/* the track's lock store: the lock model's data + track * 0x2447 */
static inline uint8_t *st_locks(unsigned obj)
{
    unsigned m = st_lock_model(obj);
    int n = *(int *)(obj + 0x38);
    uint8_t *base;
    if (!m || n < 0 || n >= ST_TRACKS)
        return 0;
    base = *(uint8_t **)(m + 0x10);
    return base ? base + n * ST_LOCKS_SIZE : 0;
}
static inline int st_track_no(unsigned obj)
{
    return *(int *)(obj + 0x38);
}
/* the step's change notice, as stock's setters post it */
static inline void st_notify_step(unsigned obj, int i)
{
    unsigned ev[2];
    ev[0] = 0x401c6b9cu;
    ev[1] = (unsigned)i;
    (*(void (**)(unsigned, void *))(*(unsigned *)obj + 0x10))(obj, ev);
}
/* Grid recording (RECORD lit, not live recording): the sequencer selection's +0x161 and +0x168,
 * from the UI controller (*0x444e2334) +0x40 -> node +0x1c -> main screen +8 (vtable 0x401c849c)
 * -> +0xc0 -> +0x6c (firmware notes section 6; measured) */
static inline int st_grid_recording(void)
{
    const uint8_t *ui = *(const uint8_t *const *)0x444e2334, *node, *main, *view, *sel;
    if (!ui || !(node = *(const uint8_t *const *)(ui + 0x40 + 0x1c)) || !(main = *(const uint8_t *const *)(node + 8)) ||
        *(const unsigned *)main != 0x401c849cu || !(view = *(const uint8_t *const *)(main + 0xc0)) ||
        !(sel = *(const uint8_t *const *)(view + 0x6c)))
        return 0;
    return sel[0x161] && sel[0x168];
}
#else
uint8_t *st_record(unsigned obj);
unsigned st_lock_model(unsigned obj);
uint8_t *st_locks(unsigned obj);
int st_track_no(unsigned obj);
void st_notify_step(unsigned obj, int i);
int st_grid_recording(void);
#endif

/* the current track (0..12, 12 the FX track), its object and the pattern it belongs to */
static inline int st_current(unsigned *obj, unsigned *pattern)
{
    unsigned app = ST_APP(), pat = ST_ACTIVE_PATTERN(app), t = ST_ACTIVE_TRACK(app);
    int n = t ? st_track_no(t) : -1;
    if (n < 0 || n >= ST_TRACKS)
        n = 0;
    *obj = st_track_obj(pat, n);
    if (pattern)
        *pattern = pat;
    return n;
}

static inline int st_euclid(unsigned obj)
{
    const uint8_t *r = st_record(obj);
    return r && r[ST_EUCLID];
}

static inline int16_t st_lock_at(const uint8_t *l, int step, int k)
{
    const uint8_t *p = l + 2 * (ST_STEP_LOCKS * step + k);
    return (int16_t)(p[0] << 8 | p[1]);
}

static inline uint16_t st_flags_at(const uint8_t *r, int step)
{
    return (uint16_t)(r[ST_FLAGS + 2 * step] << 8 | r[ST_FLAGS + 2 * step + 1]);
}

static inline void st_copy(uint8_t *to, const uint8_t *from, unsigned n)
{
    while (n--)
        *to++ = *from++;
}

/* ---- the audition snapshot: one track's record and lock store ---- */
struct st_snap {
    unsigned obj;
    int track, valid;
    uint8_t record[ST_RECORD_SIZE];
    uint8_t locks[ST_LOCKS_SIZE];
};

static inline int st_snap_take(struct st_snap *s, unsigned obj, int track)
{
    const uint8_t *r = st_record(obj), *l = st_locks(obj);
    if (!r || !l)
        return s->valid = 0;
    st_copy(s->record, r, ST_RECORD_SIZE);
    st_copy(s->locks, l, ST_LOCKS_SIZE);
    s->obj = obj;
    s->track = track;
    return s->valid = 1;
}

/* ---- a step as stock stores it: what a write aims at ---- */
struct st_raw {
    uint16_t flags;
    uint8_t note, vel, len, rlen, rrate, b240, b280, not2, not3, not4;
    int8_t utime, cond, rvel;
    int16_t lock[ST_SLOTS];
};

static inline void st_raw_of(struct st_raw *w, const uint8_t *r, const uint8_t *l, int i)
{
    int k;
    w->flags = st_flags_at(r, i);
    w->note = r[ST_NOTE + i];
    w->vel = r[ST_VEL + i];
    w->len = r[ST_LEN + i];
    w->utime = (int8_t)r[ST_UTIME + i];
    w->cond = (int8_t)r[ST_COND + i];
    w->rlen = r[ST_RLEN + i];
    w->rrate = r[ST_RRATE + i];
    w->rvel = (int8_t)r[ST_RVEL + i];
    w->b240 = r[ST_B240 + i];
    w->b280 = r[ST_B280 + i];
    w->not2 = r[ST_NOT2 + i];
    w->not3 = r[ST_NOT3 + i];
    w->not4 = r[ST_NOT4 + i];
    for (k = 0; k < ST_SLOTS; k++)
        w->lock[k] = st_lock_at(l, i, k);
}

/* Step i to w, through stock's routines: flags (a trig placed or cleared first), then the fields,
 * then the locks (a step left without a trig is cleared by stock's flag routine, so the locks come
 * last). The two bytes stock has no known setter for are written directly, with the step's notice.
 * `keep`: a generated trig keeps the stock bits stock's place gave it. 1 when it changed. */
static inline int st_apply(unsigned obj, int i, const struct st_raw *w, int keep)
{
    const uint8_t *r = st_record(obj), *l;
    uint8_t *rw;
    unsigned model = st_lock_model(obj);
    int track = st_track_no(obj), c = 0, k, direct = 0;
    uint16_t want = w->flags, have = st_flags_at(r, i), on, off;
    if ((want ^ have) & SEED_TRIG) {
        if (want & SEED_TRIG)
            ST_PLACE_TRIG(obj, i, 1);
        else
            ST_SET_FLAG(obj, i, SEED_TRIG, 0);
        c = 1;
        have = st_flags_at(st_record(obj), i);
        if ((want & SEED_TRIG) && keep)
            want = (uint16_t)((want & ~ST_STOCK_BITS) | (have & ST_STOCK_BITS));
    }
    on = (uint16_t)(want & ~have & ~SEED_TRIG);
    off = (uint16_t)(~want & have & ~SEED_TRIG);
    if (on) { ST_SET_FLAG(obj, i, on, 1); c = 1; }
    if (off) { ST_SET_FLAG(obj, i, off, 0); c = 1; }
    r = st_record(obj);
    if (r[ST_NOTE + i] != w->note) { ST_SET_NOTE(obj, i, w->note); c = 1; }
    if (r[ST_NOT2 + i] != w->not2) { ST_SET_NOT2(obj, i, w->not2); c = 1; }
    if (r[ST_NOT3 + i] != w->not3) { ST_SET_NOT3(obj, i, w->not3); c = 1; }
    if (r[ST_NOT4 + i] != w->not4) { ST_SET_NOT4(obj, i, w->not4); c = 1; }
    if (r[ST_VEL + i] != w->vel) { ST_SET_VEL(obj, i, w->vel); c = 1; }
    if (r[ST_LEN + i] != w->len) { ST_SET_LEN(obj, i, w->len); c = 1; }
    if ((int8_t)r[ST_UTIME + i] != w->utime) { ST_SET_UTIME(obj, i, w->utime + 64); c = 1; }
    if ((int8_t)r[ST_COND + i] != w->cond) { ST_SET_COND(obj, i, w->cond); c = 1; }
    if (r[ST_RLEN + i] != w->rlen) { ST_SET_RLEN(obj, i, w->rlen); c = 1; }
    if (r[ST_RRATE + i] != w->rrate) { ST_SET_RRATE(obj, i, w->rrate); c = 1; }
    if ((int8_t)r[ST_RVEL + i] != w->rvel) { ST_SET_RVEL(obj, i, w->rvel); c = 1; }
    rw = st_record(obj);
    if (rw[ST_B240 + i] != w->b240) { rw[ST_B240 + i] = w->b240; direct = 1; }
    if (rw[ST_B280 + i] != w->b280) { rw[ST_B280 + i] = w->b280; direct = 1; }
    if (direct) {
        st_notify_step(obj, i);
        c = 1;
    }
    l = st_locks(obj);
    for (k = 0; k < ST_SLOTS; k++) {
        if (w->lock[k] == st_lock_at(l, i, k))
            continue;
        if (w->lock[k] < 0)
            ST_LOCK_CLEAR(model, track, i, k);
        else
            ST_LOCK_SET(model, track, i, k, w->lock[k]);
        c = 1;
    }
    return c;
}

/* back to the snapshot, byte-identical in its record and lock store (I7): every step that differs
 * through stock's routines (so the sequencer's copy and the lock model follow), then the bytes */
static inline int st_snap_restore(struct st_snap *s)
{
    struct st_raw w;
    uint8_t *r, *l;
    int i;
    if (!s->valid || !(r = st_record(s->obj)) || !(l = st_locks(s->obj)))
        return 0;
    for (i = 0; i < SEED_STEPS; i++) {
        st_raw_of(&w, s->record, s->locks, i);
        st_apply(s->obj, i, &w, 0);
    }
    st_copy(st_record(s->obj), s->record, ST_RECORD_SIZE);
    st_copy(st_locks(s->obj), s->locks, ST_LOCKS_SIZE);
    for (i = 0; i < SEED_STEPS; i++)
        st_notify_step(s->obj, i);
    return 1;
}

/* ---- ROTATE ALL: whole steps, with their locks ----
 * OS 1.42 has no whole-step shift that a mod could call (firmware notes section 8), so the track's first `len`
 * steps are rotated in a copy and written back as a restore is (st_snap_restore: every step through stock's
 * routines, then the bytes): step i goes to (i + k) mod len, as lib/'s seed_rotate and the Rytm's stock shift
 * do. Every per-step array of the record moves (flags, VEL .. NOT4, the unidentified +0x240 / +0x280 too) and
 * each step's 72 lock slots with it; the swing bit (0x0010) stays with the position. The track's defaults,
 * length, Euclidean settings and the lock store's byte per slot do not change (a lock on some step still is). */
static inline void st_reverse(uint8_t *base, unsigned size, int a, int b)
{
    unsigned j;
    for (; a < b; a++, b--)
        for (j = 0; j < size; j++) {
            uint8_t x = base[a * size + j];
            base[a * size + j] = base[b * size + j];
            base[b * size + j] = x;
        }
}

static inline void st_rot(uint8_t *base, unsigned size, int len, int k)
{
    st_reverse(base, size, 0, len - 1);
    st_reverse(base, size, 0, k - 1);
    st_reverse(base, size, k, len - 1);
}

/* tmp: scratch (a snapshot's size); 1 when the track was rotated */
static inline int st_rotate(struct st_snap *tmp, unsigned obj, int track, int len, int steps)
{
    uint8_t swing[SEED_STEPS];
    int k, i, a;
    if (len < 1 || len > SEED_STEPS || !st_snap_take(tmp, obj, track))
        return 0;
    k = ((steps % len) + len) % len;
    if (!k)
        return 1;
    for (i = 0; i < len; i++)
        swing[i] = (uint8_t)(tmp->record[ST_FLAGS + 2 * i + 1] & 0x10);
    st_rot(tmp->record + ST_FLAGS, 2, len, k);
    for (i = 0; i < len; i++)
        tmp->record[ST_FLAGS + 2 * i + 1] = (uint8_t)((tmp->record[ST_FLAGS + 2 * i + 1] & ~0x10) | swing[i]);
    for (a = ST_VEL; a <= ST_NOT4; a += SEED_STEPS)
        st_rot(tmp->record + a, 1, len, k);
    st_rot(tmp->locks, 2 * ST_STEP_LOCKS, len, k);
    st_snap_restore(tmp);
    tmp->valid = 0;
    return 1;
}

/* ==== version 1: struct seed_dev (lib/seed.h, design section 10) ==== */

static inline int st_seed_fields(int track)
{
    return track == ST_FX_TRACK ? ST_FX_FIELDS : ST_VOICE_FIELDS;
}

/* a track's lockable parameters (SD4): lib/'s list and stock's id of each */
struct st_list {
    int machine;                       /* the machine's type, or -1: not known (the shared pages) */
    int built;
    int n;
    struct seed_param p[ST_TRACK_PARAMS_MAX];
    uint16_t id[ST_TRACK_PARAMS_MAX];
    int16_t lane[SEED_LANES];
};

/* everything the layer keeps: the snapshot of every track (the audition's undo, and the source of
 * origins), and each track's list */
struct st_seed_state {
    struct st_snap snap[ST_TRACKS];
    uint8_t tmp[ST_TRACKS];            /* taken by a write without a snapshot, dropped after it */
    int written;                       /* steps the last write changed */
    struct st_list list[ST_TRACKS];
    struct st_snap rot;                /* ROTATE ALL's scratch (st_rotate) */
};
#define ST_SEED_STATE_SIZE sizeof(struct st_seed_state)

static inline struct st_seed_state **st_seed_state_ptr(void)
{
    static struct st_seed_state *s;
    return &s;
}
static inline struct st_seed_state *st_seed_state(void)
{
    return *st_seed_state_ptr();
}

/* the layer's memory (ST_SEED_STATE_SIZE bytes, any alignment of 4); 0 when too small */
static inline int st_seed_attach(void *mem, unsigned size)
{
    struct st_seed_state *s = (struct st_seed_state *)mem;
    int t;
    if (!mem || size < ST_SEED_STATE_SIZE)
        return 0;
    for (t = 0; t < ST_TRACKS; t++) {
        s->snap[t].valid = 0;
        s->tmp[t] = 0;
        s->list[t].machine = -1;
        s->list[t].built = 0;
    }
    s->written = 0;
    *st_seed_state_ptr() = s;
    return 1;
}

static inline const struct st_list *st_seed_list(int track)
{
    static const struct st_machine machines[ST_NMACHINES] = ST_MACHINES;
    struct st_list *l = &st_seed_state()->list[track];
    if (!l->built) {
        const struct st_machine *m = l->machine < 0 ? 0 : st_machine_of(machines, ST_NMACHINES, l->machine);
        l->n = st_track_params(m, track, l->p, l->id, l->lane);
        l->built = 1;
    }
    return l;
}

/* the mod's word on a track's machine (its type, -1 not known): the track's list follows */
static inline void st_seed_set_machine(int track, int type)
{
    struct st_seed_state *s = st_seed_state();
    if (!s || track < 0 || track >= ST_TRACKS || s->list[track].machine == type)
        return;
    s->list[track].machine = type;
    s->list[track].built = 0;
}

static inline const struct seed_dev *st_seed_device(void);

/* the caps of a track: its list of parameters and lane defaults (the device's caps, which are not
 * const: st_seed_select points them at the track's list) */
static inline struct seed_caps *st_seed_caps(void)
{
    return (struct seed_caps *)st_seed_device()->caps;
}
static inline void st_seed_select(int track)
{
    struct seed_caps *c = st_seed_caps();
    const struct st_list *l;
    if (!st_seed_state() || track < 0 || track >= ST_TRACKS)
        return;
    l = st_seed_list(track);
    c->nparams = (uint8_t)l->n;
    c->params = l->p;
    c->lane_parm[0] = l->lane[0];
    c->lane_parm[1] = l->lane[1];
}

static inline unsigned st_seed_obj(int track)
{
    if (track < 0 || track >= ST_TRACKS)
        return 0;
    return st_track_obj(ST_ACTIVE_PATTERN(ST_APP()), track);
}

static inline int st_seed_length(unsigned obj)
{
    int n = ST_TRACK_LENGTH(obj);
    return n < 1 || n > SEED_STEPS ? 16 : n;
}

/* the lock slot of lane `lane` (parm: the lanes' parameters, indexes in the track's list), or -1:
 * no lane on this track */
static inline int st_seed_lane_k(int track, const int16_t *parm, int lane)
{
    const struct st_list *l;
    int k;
    if (!parm || !(st_seed_fields(track) & SEED_F_LOCKS) || parm[lane] < 0)
        return -1;
    l = st_seed_list(track);
    if (parm[lane] >= l->n)
        return -1;
    k = ST_SLOT(l->id[parm[lane]]);
    return k < ST_SLOTS ? k : -1;
}

/* read: every step, origin = its index, lanes as the stored value's whole part */
static inline int st_seed_read(int track, const int16_t *parm, struct seed_track *t)
{
    unsigned obj;
    const uint8_t *r, *l;
    int i, j, kk[SEED_LANES];
    if (!st_seed_state() || !(obj = st_seed_obj(track)) || !(r = st_record(obj)) || !(l = st_locks(obj)))
        return SEED_E_NA;
    st_seed_select(track);
    t->length = (int16_t)st_seed_length(obj);
    t->fields = (uint16_t)st_seed_fields(track);
    t->default_note = r[ST_DEF_NOTE];
    t->default_vel = r[ST_DEF_VEL];
    for (j = 0; j < SEED_LANES; j++)
        kk[j] = st_seed_lane_k(track, parm, j);
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = st_flags_at(r, i);
        s->note = r[ST_NOTE + i];
        s->vel = r[ST_VEL + i];
        s->len = r[ST_LEN + i];
        s->utime = (int8_t)r[ST_UTIME + i];
        s->cond = (int8_t)st_cond_to_seed((int8_t)r[ST_COND + i]);
        s->rlen = r[ST_RLEN + i];
        s->rrate = r[ST_RRATE + i];
        s->rvel = (int8_t)r[ST_RVEL + i];
        s->origin = (int8_t)i;
        s->spare = 0;
        for (j = 0; j < SEED_LANES; j++) {
            int16_t v = kk[j] >= 0 ? st_lock_at(l, i, kk[j]) : -1;
            s->lane[j] = v < 0 ? SEED_NO_LOCK : (int16_t)(v >> 8);
        }
    }
    return SEED_OK;
}

/* The snapshot a write copies origins from: the audition's, or (no audition) one taken now and
 * marked temporary. 0 when the track has no data. */
static inline struct st_snap *st_seed_source(int track, unsigned obj)
{
    struct st_seed_state *st = st_seed_state();
    struct st_snap *s = &st->snap[track];
    if (s->valid && s->obj == obj)
        return s;
    if (!st_snap_take(s, obj, track))
        return 0;
    st->tmp[track] = 1;
    return s;
}

static inline void st_seed_source_done(int track)
{
    struct st_seed_state *st = st_seed_state();
    if (st->tmp[track]) {
        st->tmp[track] = 0;
        st->snap[track].valid = 0;
    }
}

static inline int st_seed_steps(const struct seed_track *t, unsigned obj)
{
    int n = t->length, len = st_seed_length(obj);
    return n > len ? len : n < 0 ? 0 : n;
}

/* step i of t as stock will store it: the origin's whole step (all its locks, chord notes and the
 * bytes lib/ does not know) or, with no origin, what position i holds; then t's fields and lanes.
 * A lane value equal to the source's whole part keeps the source's fine part. */
static inline void st_seed_target(struct st_raw *w, const struct st_snap *src, int track, const int16_t *parm,
                                  const struct seed_step *s, int i)
{
    int from = s->origin >= 0 ? s->origin : i, j;
    st_raw_of(w, src->record, src->locks, from);
    w->flags = (uint16_t)((s->flags & ~SEED_SWING) | (st_flags_at(src->record, i) & SEED_SWING));
    if (s->origin < 0)
        w->flags = (uint16_t)((w->flags & ~ST_STOCK_BITS) | (st_flags_at(src->record, i) & ST_STOCK_BITS));
    w->note = s->note;
    w->vel = s->vel;
    w->len = s->len;
    w->utime = s->utime;
    w->cond = (int8_t)st_cond_from_seed(s->cond);
    w->rlen = s->rlen;
    w->rrate = s->rrate;
    w->rvel = s->rvel;
    for (j = 0; j < SEED_LANES; j++) {
        int k = st_seed_lane_k(track, parm, j);
        int16_t v;
        if (k < 0)
            continue;
        v = w->lock[k];
        if (s->lane[j] == SEED_NO_LOCK)
            v = -1;
        else if (v < 0 || v >> 8 != s->lane[j])
            v = (int16_t)(s->lane[j] << 8);
        w->lock[k] = v;
    }
}

/* Every lock in t can be stored (I8): the Syntakt keeps a slot for every parameter on every step
 * (no row pool, firmware notes section 5), so only the settings are checked: origins in range, a
 * lane's values in its parameter's range, a lane on a parameter stock locks. */
static inline int st_seed_room_src(int track, unsigned obj, const int16_t *parm, const struct seed_track *t)
{
    const struct st_list *l = st_seed_list(track);
    int n = st_seed_steps(t, obj), i, j;
    for (i = 0; i < n; i++)
        if (t->s[i].origin >= n || t->s[i].origin < SEED_NO_ORIGIN)
            return SEED_E_ARG;
    for (j = 0; j < SEED_LANES; j++) {
        int p = parm ? parm[j] : -1;
        if (p < 0 || !(st_seed_fields(track) & SEED_F_LOCKS))
            continue;
        if (p >= l->n || st_seed_lane_k(track, parm, j) < 0)
            return SEED_E_ARG;
        for (i = 0; i < n; i++)
            if (t->s[i].lane[j] != SEED_NO_LOCK && (t->s[i].lane[j] < l->p[p].lo || t->s[i].lane[j] > l->p[p].hi))
                return SEED_E_ARG;
    }
    if (parm && parm[0] >= 0 && parm[0] == parm[1] && (st_seed_fields(track) & SEED_F_LOCKS))
        return SEED_E_ARG;             /* two lanes on one parameter: one would be lost */
    return SEED_OK;
}

static inline int st_seed_room(int track, const int16_t *parm, const struct seed_track *t)
{
    unsigned obj;
    if (!st_seed_state() || !(obj = st_seed_obj(track)) || !st_record(obj) || !st_locks(obj))
        return SEED_E_NA;
    st_seed_select(track);
    return st_seed_room_src(track, obj, parm, t);
}

/* write: the settings first (nothing is written when one is refused), then each step below the
 * track's length that differs */
static inline int st_seed_write(int track, const int16_t *parm, const struct seed_track *t)
{
    struct st_seed_state *st = st_seed_state();
    unsigned obj;
    struct st_snap *src;
    struct st_raw w;
    int i, n, e, changed = 0;
    if (!st || !(obj = st_seed_obj(track)) || !st_record(obj) || !st_locks(obj))
        return SEED_E_NA;
    st->written = 0;
    st_seed_select(track);
    if (!(src = st_seed_source(track, obj)))
        return SEED_E_NA;
    n = st_seed_steps(t, obj);
    e = st_seed_room_src(track, obj, parm, t);
    if (e == SEED_OK)
        for (i = 0; i < n; i++) {
            st_seed_target(&w, src, track, parm, &t->s[i], i);
            changed += st_apply(obj, i, &w, t->s[i].origin < 0);
        }
    st_seed_source_done(track);
    st->written = changed;
    return e;
}

static inline int st_seed_current(void)
{
    unsigned obj;
    int n = st_current(&obj, 0);
    if (st_seed_state())
        st_seed_select(n);
    return n;
}

/* the pattern: each track's length and the trigs it plays (a track in Euclidean mode: the pattern
 * that mode plays); caps, track and tracks. The mod fills shared, grooves and ngrooves. */
static inline void st_seed_context(struct seed_ctx *c, int track)
{
    int t, i;
    if (st_seed_state())
        st_seed_select(track);
    c->caps = st_seed_device()->caps;
    c->track = track;
    c->tracks = ST_TRACKS;
    for (t = 0; t < ST_TRACKS; t++) {
        unsigned obj = st_seed_obj(t);
        const uint8_t *r = obj ? st_record(obj) : 0;
        for (i = 0; i < SEED_STEPS / 8; i++)
            c->trig[t][i] = 0;
        c->length[t] = 0;
        if (!r)
            continue;
        c->length[t] = (uint8_t)st_seed_length(obj);
        for (i = 0; i < c->length[t]; i++)
            if (r[ST_EUCLID] ? r[ST_EUCLID_TAB + i] != 0xff : (st_flags_at(r, i) & SEED_TRIG) != 0)
                c->trig[t][i >> 3] |= (uint8_t)(1 << (i & 7));
    }
}

static inline void st_seed_drop(void)
{
    struct st_seed_state *st = st_seed_state();
    int t;
    if (!st)
        return;
    for (t = 0; t < ST_TRACKS; t++) {
        st->snap[t].valid = 0;
        st->tmp[t] = 0;
    }
}

/* snapshot: one bit per track in scope; any earlier snapshot is dropped first */
static inline int st_seed_snapshot(uint16_t tracks)
{
    struct st_seed_state *st = st_seed_state();
    unsigned pat;
    int t;
    if (!st)
        return SEED_E_NA;
    pat = ST_ACTIVE_PATTERN(ST_APP());
    st_seed_drop();
    for (t = 0; t < ST_TRACKS; t++)
        if (tracks >> t & 1 && !st_snap_take(&st->snap[t], st_track_obj(pat, t), t)) {
            st_seed_drop();
            return SEED_E_NA;
        }
    return SEED_OK;
}

/* restore: every track in the snapshot back, byte-identical in its record and lock store (I7);
 * the snapshot stays, so the audition goes on */
static inline void st_seed_restore(void)
{
    struct st_seed_state *st = st_seed_state();
    int t;
    if (!st)
        return;
    for (t = 0; t < ST_TRACKS; t++)
        if (!st->tmp[t])
            st_snap_restore(&st->snap[t]);
}

static inline void st_seed_changed(int track)
{
    unsigned obj = st_seed_obj(track);
    int i;
    if (obj)
        for (i = 0; i < SEED_STEPS; i++)
            st_notify_step(obj, i);
}

/* ROTATE ALL on a track: its whole steps by `steps`, with their locks (st_rotate); 0 when skipped (a track in
 * stock's Euclidean mode, or no state) */
static inline int st_seed_rotate(int track, int steps)
{
    struct st_seed_state *st = st_seed_state();
    unsigned obj = st_seed_obj(track);
    const uint8_t *r;
    if (!st || !obj || !(r = st_record(obj)) || r[ST_EUCLID])
        return 0;
    return st_rotate(&st->rot, obj, track, st_seed_length(obj), steps);
}

static inline const struct seed_dev *st_seed_device(void)
{
    static struct seed_caps caps = {
        ST_TRACKS, SEED_STEPS, ST_VOICE_FIELDS, ST_UTIME_LO, ST_UTIME_HI, SEED_LANES, 0, 0, { 0, 0 }
    };
    static const struct seed_dev d = {
        &caps, st_seed_current, st_seed_read, st_seed_room, st_seed_write, st_seed_context,
        st_seed_snapshot, st_seed_restore, st_seed_drop, st_seed_changed
    };
    return &d;
}

/* steps the last write changed (for the mod's message) */
static inline int st_seed_written(void)
{
    return st_seed_state() ? st_seed_state()->written : 0;
}

#endif
