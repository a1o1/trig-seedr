/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Analog Rytm MKII OS 1.74 device layer for gen: read the current track into a struct
 * gen_track, write one back through the stock step routines, and snapshot / restore a track for
 * the audition undo. Addresses and offsets: rytm-step-layout-os174.md (checked in analog-emu).
 * UI task only (ev_key, ev_enc, ev_tick, ev_draw). */
#ifndef RYTM_AR2_H
#define RYTM_AR2_H

#include <stdint.h>
#include "../lib/gen.h"

/* ---- stock routines. A routine that returns a pointer returns it in d0, so those are declared
 * as returning an integer (elekloader docs/ADAPTING.md). ---- */
#define RT_APP            ((unsigned (*)(void))0x4017fa0a)
#define RT_APP_TRACKS     ((unsigned (*)(unsigned))0x400ae492)
#define RT_CURRENT_TRACK  ((int (*)(unsigned))0x400b693e)
#define RT_CURRENT_PAT    ((unsigned (*)(unsigned))0x400af0ac)
#define RT_TRACK_OBJ      ((unsigned (*)(unsigned, int))0x400ab29e)
#define RT_TRACK_LENGTH   ((int (*)(unsigned))0x400c0524)
#define RT_MEMCPY         ((void (*)(void *, const void *, unsigned))0x40098bc0)
/* step writes: (track object, step, value); steps past the track's length are refused */
#define RT_SET_TRIG       ((void (*)(unsigned, int, int))0x400c2620)        /* note trig on / off */
#define RT_SET_FLAG       ((void (*)(unsigned, int, int, int))0x400c2482)   /* (mask, on) */
#define RT_SET_NOTE       ((void (*)(unsigned, int, int))0x400c0776)
#define RT_SET_VEL        ((void (*)(unsigned, int, int))0x400c07f0)
#define RT_SET_LEN        ((void (*)(unsigned, int, int))0x400c086a)
#define RT_SET_UTIME      ((void (*)(unsigned, int, int))0x400c08e4)        /* value + 64 */
#define RT_SET_COND       ((void (*)(unsigned, int, int))0x400c03d6)
#define RT_SET_RLEN       ((void (*)(unsigned, int, int))0x400c0a02)
#define RT_SET_RRATE      ((void (*)(unsigned, int, int))0x400c0978)
#define RT_SET_RVEL       ((void (*)(unsigned, int, int))0x400c0a8c)
/* parameter locks: (track object, step, sound index, value as stored (8.8), fine flag), refused
 * past the track's length; clear: (track object, step, sound index). Sidecar's track_lock_set
 * (0x400c0520 in 1.73), found in 1.74 by its shape: the length check, then lock_pool_set
 * 0x400ac6dc with the pool (+0x2c) and the track (+0x40); the sound set's lock clear (0x400a99c2)
 * tail-calls the clear. */
#define RT_LOCK_SET       ((int (*)(unsigned, int, int, int, int))0x400c058c)
#define RT_LOCK_CLEAR     ((int (*)(unsigned, int, int))0x400c05e2)
/* sound index -> the stored lock row's type, voice tracks (1.73: 0x401c59cc, Sidecar's LOCK_TYPE;
 * 1.74: the only table of 38 types whose inverse, 0x401c5b4c, the row code next to it uses) */
#define RT_LOCK_TYPE(k)   (((const int32_t *)0x401c5be4)[k])

#define RT_RECORD_SIZE 0x395
#define RT_LOCKS_SIZE  0x162a          /* a track's lock store: 64 steps of 88 bytes, and its rows */

#define RT_FLAGS   0x000
#define RT_NOTE    0x080
#define RT_VEL     0x0c0
#define RT_LEN     0x100
#define RT_UTIME   0x140
#define RT_RLEN    0x180
#define RT_RRATE   0x1c0
#define RT_RVEL    0x200
#define RT_COND    0x240
#define RT_DEF_NOTE 0x2c0
#define RT_DEF_VEL  0x2c1
#define RT_EUCLID   0x2ca

/* a track's lock store: per step 88 bytes = 44 int16 values by sound index (-1: none), then per
 * sound index a fine-flag byte at +5632; the pattern keeps 72 stored rows of (type, track) */
#define RT_STEP_LOCKS 88
#define RT_LOCK_FINE  5632
#define RT_SOUND_LOCKS 42
#define RT_SAMPLE_SLOT 19              /* the SMPL page's sample slot: value slot << 8, 0 = OFF */
#define RT_ROWS 72
#define RT_ROW 66
#ifndef RT_ROWS_MAX                    /* (a test build sets it lower) */
#define RT_ROWS_MAX 71                 /* stock's mirror takes a new row while used + cost <= 71 */
#endif

/* an object's virtual method: *obj is its vtable */
#define RT_METHOD(obj, off) (*(unsigned (**)(unsigned, ...))(*(unsigned *)(obj) + (off)))

static inline uint8_t *rt_record(unsigned obj)
{
    return (uint8_t *)RT_METHOD(obj, 0x28)(obj);
}

/* the track's lock store: the pattern object's data (track object +0x2c) + track (+0x40) * 0x162a */
static inline uint8_t *rt_locks(unsigned obj)
{
    unsigned pat = *(unsigned *)(obj + 0x2c);
    int n = *(int *)(obj + 0x40);
    uint8_t *base;
    if (!pat || n < 0)
        return 0;
    base = (uint8_t *)RT_METHOD(pat, 0x28)(pat);
    return base ? base + n * RT_LOCKS_SIZE : 0;
}

/* the stock change notice: the whole track (and its pattern) changed */
static inline void rt_changed(unsigned obj)
{
    unsigned pat = *(unsigned *)(obj + 0x2c);
    RT_METHOD(obj, 0x10)(obj, 0);
    if (pat)
        RT_METHOD(pat, 0x10)(pat, 0);
}

/* the current track (0..12, 12 the FX track), its object and the pattern it belongs to */
static inline int rt_current(unsigned *obj, unsigned *pattern)
{
    unsigned app = RT_APP(), pat = RT_CURRENT_PAT(app);
    int n = RT_CURRENT_TRACK(RT_APP_TRACKS(app));
    *obj = RT_TRACK_OBJ(pat, n);
    if (pattern)
        *pattern = pat;
    return n;
}

static inline int rt_euclid(unsigned obj)
{
    const uint8_t *r = rt_record(obj);
    return r && r[RT_EUCLID];
}

/* read a track into t; 0 when the track has no data */
static inline int rt_read(unsigned obj, struct gen_track *t, uint8_t *def_vel)
{
    const uint8_t *r = rt_record(obj), *l = rt_locks(obj);
    int i;
    if (!r)
        return 0;
    t->length = RT_TRACK_LENGTH(obj);
    if (t->length < 1 || t->length > GEN_STEPS)
        t->length = 16;
    t->default_note = r[RT_DEF_NOTE];
    if (def_vel)
        *def_vel = r[RT_DEF_VEL];
    for (i = 0; i < GEN_STEPS; i++) {
        struct gen_step *s = &t->s[i];
        s->flags = (uint16_t)(r[RT_FLAGS + 2 * i] << 8 | r[RT_FLAGS + 2 * i + 1]);
        s->note = r[RT_NOTE + i];
        s->vel = r[RT_VEL + i];
        s->len = r[RT_LEN + i];
        s->utime = (int8_t)r[RT_UTIME + i];
        s->cond = (int8_t)r[RT_COND + i];
        s->rlen = r[RT_RLEN + i];
        s->rrate = r[RT_RRATE + i];
        s->rvel = (int8_t)r[RT_RVEL + i];
        s->slock = GEN_NO_LOCK;
        if (l) {
            int16_t v = (int16_t)(l[RT_STEP_LOCKS * i + 2 * RT_SAMPLE_SLOT] << 8 | l[RT_STEP_LOCKS * i + 2 * RT_SAMPLE_SLOT + 1]);
            s->slock = v < 0 ? GEN_NO_LOCK : (int16_t)(v >> 8);
        }
    }
    return 1;
}

/* the pattern's stored lock rows: used, and whether (type of sound index k, this track) has one */
static inline int rt_rows(unsigned obj, int k, int *have)
{
    unsigned pool = *(unsigned *)(obj + 0x2c);
    int track = *(int *)(obj + 0x40), used = 0, j;
    const uint8_t *m;
    *have = 0;
    if (!pool || !(m = *(const uint8_t **)(pool + 0x20)))
        return -1;
    for (j = 0; j < RT_ROWS; j++, m += RT_ROW) {
        if (m[0] == 0xff || m[1] == 0xff)
            continue;
        used++;
        if (m[0] == (uint8_t)RT_LOCK_TYPE(k) && m[1] == track)
            *have = 1;
    }
    return used;
}

/* 1 when a lock of sound index k can be added on this track and kept by a save: its row exists,
 * or a new one fits (2 rows for a fine parameter), as stock's mirror counts */
static inline int rt_lock_room(unsigned obj, int k)
{
    const uint8_t *l = rt_locks(obj);
    int have, used = rt_rows(obj, k, &have);
    if (used < 0 || !l)
        return 0;
    return have || used + (l[RT_LOCK_FINE + k] ? 2 : 1) <= RT_ROWS_MAX;
}

/* Write t's steps 0..t->length-1 into the track through the stock routines, changing only what
 * differs from what the track holds now. A trig removed or added goes through the place/remove
 * routine (which sets the SYN/SMP/ENV/LFO bits from the track's defaults); flags and fields go
 * through their setters. Returns the number of steps changed. */
static inline int rt_write(unsigned obj, const struct gen_track *t)
{
    static const uint16_t masks[4] = { GEN_ACCENT, GEN_RETRIG, GEN_SLIDE, GEN_MUTE };
    struct gen_track now;
    int i, m, n = t->length, changed = 0;

    if (!rt_read(obj, &now, 0))
        return 0;
    if (n > now.length)
        n = now.length;
    for (i = 0; i < n; i++) {
        const struct gen_step *w = &t->s[i];
        const uint8_t *r;
        int c = 0;
        uint16_t f;
        if ((w->flags & GEN_TRIG) != (now.s[i].flags & GEN_TRIG)) {
            RT_SET_TRIG(obj, i, (w->flags & GEN_TRIG) != 0);
            c = 1;
        }
        r = rt_record(obj);
        f = (uint16_t)(r[RT_FLAGS + 2 * i] << 8 | r[RT_FLAGS + 2 * i + 1]);
        for (m = 0; m < 4; m++)
            if ((w->flags & masks[m]) != (f & masks[m])) {
                RT_SET_FLAG(obj, i, masks[m], (w->flags & masks[m]) != 0);
                c = 1;
            }
        r = rt_record(obj);
        if (r[RT_NOTE + i] != w->note) { RT_SET_NOTE(obj, i, w->note); c = 1; }
        if (r[RT_VEL + i] != w->vel) { RT_SET_VEL(obj, i, w->vel); c = 1; }
        if (r[RT_LEN + i] != w->len) { RT_SET_LEN(obj, i, w->len); c = 1; }
        if ((int8_t)r[RT_UTIME + i] != w->utime) { RT_SET_UTIME(obj, i, w->utime + 64); c = 1; }
        if ((int8_t)r[RT_COND + i] != w->cond) { RT_SET_COND(obj, i, w->cond); c = 1; }
        if (r[RT_RLEN + i] != w->rlen) { RT_SET_RLEN(obj, i, w->rlen); c = 1; }
        if (r[RT_RRATE + i] != w->rrate) { RT_SET_RRATE(obj, i, w->rrate); c = 1; }
        if ((int8_t)r[RT_RVEL + i] != w->rvel) { RT_SET_RVEL(obj, i, w->rvel); c = 1; }
        if (w->slock != now.s[i].slock) {
            const uint8_t *l = rt_locks(obj);
            if (w->slock == GEN_NO_LOCK)
                RT_LOCK_CLEAR(obj, i, RT_SAMPLE_SLOT);
            else
                RT_LOCK_SET(obj, i, RT_SAMPLE_SLOT, w->slock << 8, l && l[RT_LOCK_FINE + RT_SAMPLE_SLOT]);
            c = 1;
        }
        changed += c;
    }
    return changed;
}

/* ---- the audition snapshot: one track's record and lock store ---- */
struct rt_snap {
    unsigned obj, pattern;
    int track, valid;
    uint8_t record[RT_RECORD_SIZE];
    uint8_t locks[RT_LOCKS_SIZE];
};

static inline int rt_snap_take(struct rt_snap *s, unsigned obj, unsigned pattern, int track)
{
    const uint8_t *r = rt_record(obj), *l = rt_locks(obj);
    if (!r || !l)
        return s->valid = 0;
    RT_MEMCPY(s->record, r, RT_RECORD_SIZE);
    RT_MEMCPY(s->locks, l, RT_LOCKS_SIZE);
    s->obj = obj;
    s->pattern = pattern;
    s->track = track;
    return s->valid = 1;
}

static inline int rt_snap_restore(struct rt_snap *s)
{
    uint8_t *r, *l;
    if (!s->valid)
        return 0;
    r = rt_record(s->obj);
    l = rt_locks(s->obj);
    if (!r || !l)
        return 0;
    /* locks that differ go back through stock's set / clear first, so the pattern's stored rows
     * follow (a byte copy alone would leave a row a take added); then the bytes, as before */
    {
        int i, k;
        for (i = 0; i < 64; i++)
            for (k = 0; k < RT_SOUND_LOCKS; k++) {
                const uint8_t *a = l + RT_STEP_LOCKS * i + 2 * k, *b = s->locks + RT_STEP_LOCKS * i + 2 * k;
                int16_t was = (int16_t)(b[0] << 8 | b[1]);
                if (a[0] == b[0] && a[1] == b[1])
                    continue;
                if (was < 0)
                    RT_LOCK_CLEAR(s->obj, i, k);
                else
                    RT_LOCK_SET(s->obj, i, k, was, s->locks[RT_LOCK_FINE + k]);
            }
    }
    RT_MEMCPY(r, s->record, RT_RECORD_SIZE);
    RT_MEMCPY(l, s->locks, RT_LOCKS_SIZE);
    rt_changed(s->obj);
    return 1;
}

#endif
