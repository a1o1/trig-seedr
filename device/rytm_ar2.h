/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Analog Rytm MKII OS 1.74 device layer. Addresses and offsets: docs/firmware-notes-os174.md.
 * UI task only (ev_key, ev_enc, ev_tick, ev_draw).
 *
 * rt_seed_device() returns the struct seed_dev of design section 10 (lib/seed.h; task 0.4,
 * docs/status/0.4.md), built on the track, lock-row and snapshot routines below. Everything is
 * static inline, so a mod pays only for what it calls.
 *
 * A host test (device/test_rytm_ar2.c) defines RT_HOST and supplies the stock routines and the
 * accessors below them with a model of the track, so the write logic runs under the sanitizers. */
#ifndef RYTM_AR2_H
#define RYTM_AR2_H

#include <stdint.h>
#include "../lib/seed.h"

#ifndef RT_HOST
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
/* (mask, on): or / and-not of any flag bits. Whenever the step is left without a note trig, stock
 * then resets it: fields to their defaults (0x400c0b18), every parameter lock (0x400c0624) and
 * the sound lock (0x400c070a). So a step's fields and locks are written after its flags. */
#define RT_SET_FLAG       ((void (*)(unsigned, int, int, int))0x400c2482)
#define RT_SET_NOTE       ((void (*)(unsigned, int, int))0x400c0776)
#define RT_SET_VEL        ((void (*)(unsigned, int, int))0x400c07f0)
#define RT_SET_LEN        ((void (*)(unsigned, int, int))0x400c086a)
#define RT_SET_UTIME      ((void (*)(unsigned, int, int))0x400c08e4)        /* value + 64 */
#define RT_SET_COND       ((void (*)(unsigned, int, int))0x400c03d6)
#define RT_SET_RLEN       ((void (*)(unsigned, int, int))0x400c0a02)
#define RT_SET_RRATE      ((void (*)(unsigned, int, int))0x400c0978)
#define RT_SET_RVEL       ((void (*)(unsigned, int, int))0x400c0a8c)
/* the step's sound lock (record +0x280): set (track, step, 0..128; voice tracks only, else it
 * clears), clear (track, step) */
#define RT_SET_SLOCK      ((void (*)(unsigned, int, int))0x400c0670)
#define RT_CLEAR_SLOCK    ((void (*)(unsigned, int))0x400c070a)
/* parameter locks: (track object, step, sound index, value as stored (8.8), fine flag), refused
 * past the track's length; clear: (track object, step, sound index). Sidecar's track_lock_set
 * (0x400c0520 in 1.73), found in 1.74 by its shape: the length check, then lock_pool_set
 * 0x400ac6dc with the pool (+0x2c) and the track (+0x40); the sound set's lock clear (0x400a99c2)
 * tail-calls the clear. Neither touches the flag word. */
#define RT_LOCK_SET       ((int (*)(unsigned, int, int, int, int))0x400c058c)
#define RT_LOCK_CLEAR     ((int (*)(unsigned, int, int))0x400c05e2)
/* sound index -> the stored lock row's type, voice tracks (1.73: 0x401c59cc, Sidecar's LOCK_TYPE;
 * 1.74: the only table of 38 types whose inverse, 0x401c5b4c, the row code next to it uses) */
#define RT_LOCK_TYPE(k)   (((const int32_t *)0x401c5be4)[k])
#else
unsigned rt_host_app(void);
unsigned rt_host_app_tracks(unsigned app);
int rt_host_current_track(unsigned tracks);
unsigned rt_host_current_pat(unsigned app);
unsigned rt_host_track_obj(unsigned pat, int n);
int rt_host_track_length(unsigned obj);
void rt_host_memcpy(void *to, const void *from, unsigned n);
void rt_host_set_trig(unsigned obj, int i, int on);
void rt_host_set_flag(unsigned obj, int i, int mask, int on);
void rt_host_set_field(unsigned obj, int i, int value, int offset);
void rt_host_set_slock(unsigned obj, int i, int v);
void rt_host_clear_slock(unsigned obj, int i);
int rt_host_lock_set(unsigned obj, int i, int k, int value, int fine);
int rt_host_lock_clear(unsigned obj, int i, int k);
int32_t rt_host_lock_type(int k);
#define RT_APP            rt_host_app
#define RT_APP_TRACKS     rt_host_app_tracks
#define RT_CURRENT_TRACK  rt_host_current_track
#define RT_CURRENT_PAT    rt_host_current_pat
#define RT_TRACK_OBJ      rt_host_track_obj
#define RT_TRACK_LENGTH   rt_host_track_length
#define RT_MEMCPY         rt_host_memcpy
#define RT_SET_TRIG       rt_host_set_trig
#define RT_SET_FLAG       rt_host_set_flag
#define RT_SET_NOTE(o, i, v)  rt_host_set_field(o, i, v, RT_NOTE)
#define RT_SET_VEL(o, i, v)   rt_host_set_field(o, i, v, RT_VEL)
#define RT_SET_LEN(o, i, v)   rt_host_set_field(o, i, v, RT_LEN)
#define RT_SET_UTIME(o, i, v) rt_host_set_field(o, i, (v) - 64, RT_UTIME)
#define RT_SET_COND(o, i, v)  rt_host_set_field(o, i, v, RT_COND)
#define RT_SET_RLEN(o, i, v)  rt_host_set_field(o, i, v, RT_RLEN)
#define RT_SET_RRATE(o, i, v) rt_host_set_field(o, i, v, RT_RRATE)
#define RT_SET_RVEL(o, i, v)  rt_host_set_field(o, i, v, RT_RVEL)
#define RT_SET_SLOCK      rt_host_set_slock
#define RT_CLEAR_SLOCK    rt_host_clear_slock
#define RT_LOCK_SET       rt_host_lock_set
#define RT_LOCK_CLEAR     rt_host_lock_clear
#define RT_LOCK_TYPE(k)   rt_host_lock_type(k)
#endif

#define RT_RECORD_SIZE 0x395
#define RT_LOCKS_SIZE  0x162a          /* a track's lock store: 64 steps of 88 bytes, and its rows */
#define RT_TRACKS      13              /* 12 voice tracks and the FX track */
#define RT_FX_TRACK    12

#define RT_FLAGS   0x000
#define RT_NOTE    0x080
#define RT_VEL     0x0c0
#define RT_LEN     0x100
#define RT_UTIME   0x140
#define RT_RLEN    0x180
#define RT_RRATE   0x1c0
#define RT_RVEL    0x200
#define RT_COND    0x240
#define RT_SLOCK   0x280
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

#ifndef RT_HOST
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

/* the pattern's stored lock rows (RT_ROWS of RT_ROW bytes: type, track, ...), behind the lock
 * pool (track object +0x2c) at +0x20 */
static inline const uint8_t *rt_row_table(unsigned obj)
{
    unsigned pool = *(unsigned *)(obj + 0x2c);
    return pool ? *(const uint8_t **)(pool + 0x20) : 0;
}

static inline int rt_track_no(unsigned obj)
{
    return *(int *)(obj + 0x40);
}

/* the stock change notice: the whole track (and its pattern) changed */
static inline void rt_changed(unsigned obj)
{
    unsigned pat = *(unsigned *)(obj + 0x2c);
    RT_METHOD(obj, 0x10)(obj, 0);
    if (pat)
        RT_METHOD(pat, 0x10)(pat, 0);
}
#else
uint8_t *rt_record(unsigned obj);
uint8_t *rt_locks(unsigned obj);
const uint8_t *rt_row_table(unsigned obj);
int rt_track_no(unsigned obj);
void rt_changed(unsigned obj);
#endif

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

static inline int16_t rt_lock_at(const uint8_t *l, int step, int k)
{
    const uint8_t *p = l + RT_STEP_LOCKS * step + 2 * k;
    return (int16_t)(p[0] << 8 | p[1]);
}

static inline uint16_t rt_flags_at(const uint8_t *r, int step)
{
    return (uint16_t)(r[RT_FLAGS + 2 * step] << 8 | r[RT_FLAGS + 2 * step + 1]);
}

/* the pattern's stored lock rows: used, and whether (type of sound index k, this track) has one */
static inline int rt_rows(unsigned obj, int k, int *have)
{
    int track = rt_track_no(obj), used = 0, j;
    const uint8_t *m = rt_row_table(obj);
    *have = 0;
    if (!m)
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

/* ==== version 1: struct seed_dev (lib/seed.h, design section 10) ==== */

/* the lockable parameters (RT_PARAMS), lane defaults and track fields: rytm_params.h */
#include "rytm_params.h"

static inline int rt_seed_fields(int track)
{
    return track == RT_FX_TRACK ? RT_FX_FIELDS : RT_VOICE_FIELDS;
}

static inline const struct seed_dev *rt_seed_device(void);

static inline const struct seed_param *rt_seed_params(void)
{
    return rt_seed_device()->caps->params;
}

/* the snapshot of every track (the audition's undo, and the source of origins) */
struct rt_seed_state {
    struct rt_snap snap[RT_TRACKS];
    uint8_t tmp[RT_TRACKS];            /* taken by a write without a snapshot, dropped after it */
    int written;                       /* steps the last write changed */
};

static inline struct rt_seed_state *rt_seed_state(void)
{
    static struct rt_seed_state s;
    return &s;
}

static inline unsigned rt_seed_obj(int track)
{
    if (track < 0 || track >= RT_TRACKS)
        return 0;
    return RT_TRACK_OBJ(RT_CURRENT_PAT(RT_APP()), track);
}

static inline int rt_seed_length(unsigned obj)
{
    int n = RT_TRACK_LENGTH(obj);
    return n < 1 || n > SEED_STEPS ? 16 : n;
}

/* the sound index of lane `lane` (parm: the lanes' parameters), or -1: no lane on this track */
static inline int rt_seed_lane_k(int track, const int16_t *parm, int lane)
{
    if (!parm || !(rt_seed_fields(track) & SEED_F_LOCKS) || parm[lane] < 0 || parm[lane] >= RT_NPARAMS)
        return -1;
    return rt_seed_params()[parm[lane]].id;
}

static inline void rt_seed_track_init(struct seed_track *t, int track, int length, const uint8_t *r)
{
    t->length = (int16_t)length;
    t->fields = (uint16_t)rt_seed_fields(track);
    t->default_note = r[RT_DEF_NOTE];
    t->default_vel = r[RT_DEF_VEL];
}

/* read: every step, origin = its index, lanes as the stored value's whole part */
static inline int rt_seed_read(int track, const int16_t *parm, struct seed_track *t)
{
    unsigned obj = rt_seed_obj(track);
    const uint8_t *r, *l;
    int i, j;
    if (!obj || !(r = rt_record(obj)))
        return SEED_E_NA;
    l = rt_locks(obj);
    rt_seed_track_init(t, track, rt_seed_length(obj), r);
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = rt_flags_at(r, i);
        s->note = r[RT_NOTE + i];
        s->vel = r[RT_VEL + i];
        s->len = r[RT_LEN + i];
        s->utime = (int8_t)r[RT_UTIME + i];
        s->cond = (int8_t)r[RT_COND + i];
        s->rlen = r[RT_RLEN + i];
        s->rrate = r[RT_RRATE + i];
        s->rvel = (int8_t)r[RT_RVEL + i];
        s->origin = (int8_t)i;
        s->spare = 0;
        for (j = 0; j < SEED_LANES; j++) {
            int k = rt_seed_lane_k(track, parm, j);
            int16_t v = k >= 0 && l ? rt_lock_at(l, i, k) : -1;
            s->lane[j] = v < 0 ? SEED_NO_LOCK : (int16_t)(v >> 8);
        }
    }
    return SEED_OK;
}

/* The snapshot a write copies origins from: the audition's, or (no audition) one taken now and
 * marked temporary. 0 when the track has no data. */
static inline struct rt_snap *rt_seed_source(int track, unsigned obj)
{
    struct rt_seed_state *st = rt_seed_state();
    struct rt_snap *s = &st->snap[track];
    if (s->valid && s->obj == obj)
        return s;
    if (!rt_snap_take(s, obj, RT_CURRENT_PAT(RT_APP()), track))
        return 0;
    st->tmp[track] = 1;
    return s;
}

static inline void rt_seed_source_done(int track)
{
    struct rt_seed_state *st = rt_seed_state();
    if (st->tmp[track]) {
        st->tmp[track] = 0;
        st->snap[track].valid = 0;
    }
}

/* The lock (as stored, -1: none) step i of t ends with at sound index k: the origin's, or for a
 * step with no origin the one stored at i (status 0.1, decision 5); then the lanes. A lane value
 * equal to the source's whole part keeps the source's fine part. */
static inline int rt_seed_target_lock(const struct rt_snap *src, int track, const int16_t *parm,
                                      const struct seed_step *w, int i, int k)
{
    int16_t v = rt_lock_at(src->locks, w->origin >= 0 ? w->origin : i, k);
    int j;
    for (j = 0; j < SEED_LANES; j++)
        if (rt_seed_lane_k(track, parm, j) == k) {
            if (w->lane[j] == SEED_NO_LOCK)
                v = -1;
            else if (v < 0 || v >> 8 != w->lane[j])
                v = (int16_t)(w->lane[j] << 8);
        }
    return v;
}

static inline int rt_seed_steps(const struct seed_track *t, unsigned obj)
{
    int n = t->length, len = rt_seed_length(obj);
    return n > len ? len : n < 0 ? 0 : n;
}

/* Room for t's locks (I8), against src. A sound index the write would set somewhere needs its
 * stored row: voice tracks, the row exists or the new ones fit (stock's count, rt_lock_room);
 * the FX track (no row table here), the track holds a lock of that index already. */
static inline int rt_seed_room_src(int track, unsigned obj, const int16_t *parm, const struct seed_track *t,
                                   const struct rt_snap *src)
{
    const uint8_t *l = rt_locks(obj);
    uint8_t counted[RT_SOUND_LOCKS];
    int n = rt_seed_steps(t, obj), i, j, k, cost = 0, used = 0, have;
    if (!l)
        return SEED_E_NA;
    for (i = 0; i < n; i++)
        if (t->s[i].origin >= n || t->s[i].origin < SEED_NO_ORIGIN)
            return SEED_E_ARG;
    for (j = 0; j < SEED_LANES; j++) {
        int p = parm ? parm[j] : -1;
        if (rt_seed_lane_k(track, parm, j) < 0)
            continue;
        for (i = 0; i < n; i++)
            if (t->s[i].lane[j] != SEED_NO_LOCK &&
                (t->s[i].lane[j] < rt_seed_params()[p].lo || t->s[i].lane[j] > rt_seed_params()[p].hi))
                return SEED_E_ARG;
    }
    for (k = 0; k < RT_SOUND_LOCKS; k++) {
        int need = 0, held = 0;
        counted[k] = 0;
        for (i = 0; i < n && !need; i++) {
            int16_t v = rt_seed_target_lock(src, track, parm, &t->s[i], i, k);
            need = v >= 0 && v != rt_lock_at(l, i, k);
        }
        if (!need)
            continue;
        if (track == RT_FX_TRACK) {
            for (i = 0; i < SEED_STEPS && !held; i++)
                held = rt_lock_at(l, i, k) >= 0 || rt_lock_at(src->locks, i, k) >= 0;
            if (!held)
                return SEED_E_ROOM;
            continue;
        }
        used = rt_rows(obj, k, &have);
        if (used < 0)
            return SEED_E_ROOM;
        if (have)
            continue;
        for (j = 0; j < k; j++)          /* two sound indexes of one row type cost it once */
            if (counted[j] && RT_LOCK_TYPE(j) == RT_LOCK_TYPE(k))
                break;
        if (j < k)
            continue;
        counted[k] = 1;
        cost += l[RT_LOCK_FINE + k] ? 2 : 1;
    }
    return cost && used + cost > RT_ROWS_MAX ? SEED_E_ROOM : SEED_OK;
}

static inline int rt_seed_room(int track, const int16_t *parm, const struct seed_track *t)
{
    unsigned obj = rt_seed_obj(track);
    struct rt_snap *src;
    int e;
    if (!obj || !(src = rt_seed_source(track, obj)))
        return SEED_E_NA;
    e = rt_seed_room_src(track, obj, parm, t, src);
    rt_seed_source_done(track);
    return e;
}

/* Write step i: flags, then fields, then the sound lock and every parameter lock (a step left
 * without a trig is reset by stock's flag routines, so the locks come last). 1 when it changed. */
static inline int rt_seed_write_step(unsigned obj, int track, const int16_t *parm, const struct seed_step *w,
                                     int i, const struct rt_snap *src)
{
    const int from = w->origin >= 0 ? w->origin : i;
    const uint8_t *r = rt_record(obj), *l;
    uint16_t want, have, on, off;
    int c = 0, k;
    uint8_t sl;

    have = rt_flags_at(r, i);
    want = (uint16_t)((w->flags & ~SEED_SWING) | (have & SEED_SWING));
    if ((want ^ have) & SEED_TRIG) {
        RT_SET_TRIG(obj, i, (want & SEED_TRIG) != 0);
        c = 1;
        have = rt_flags_at(rt_record(obj), i);
    }
    on = (uint16_t)(want & ~have & ~SEED_TRIG);
    off = (uint16_t)(~want & have & ~SEED_TRIG);
    if (on) { RT_SET_FLAG(obj, i, on, 1); c = 1; }
    if (off) { RT_SET_FLAG(obj, i, off, 0); c = 1; }
    r = rt_record(obj);
    if (r[RT_NOTE + i] != w->note) { RT_SET_NOTE(obj, i, w->note); c = 1; }
    if (r[RT_VEL + i] != w->vel) { RT_SET_VEL(obj, i, w->vel); c = 1; }
    if (r[RT_LEN + i] != w->len) { RT_SET_LEN(obj, i, w->len); c = 1; }
    if ((int8_t)r[RT_UTIME + i] != w->utime) { RT_SET_UTIME(obj, i, w->utime + 64); c = 1; }
    if ((int8_t)r[RT_COND + i] != w->cond) { RT_SET_COND(obj, i, w->cond); c = 1; }
    if (r[RT_RLEN + i] != w->rlen) { RT_SET_RLEN(obj, i, w->rlen); c = 1; }
    if (r[RT_RRATE + i] != w->rrate) { RT_SET_RRATE(obj, i, w->rrate); c = 1; }
    if ((int8_t)r[RT_RVEL + i] != w->rvel) { RT_SET_RVEL(obj, i, w->rvel); c = 1; }
    sl = src->record[RT_SLOCK + from];
    if (rt_record(obj)[RT_SLOCK + i] != sl) {
        if (sl == 0xff)
            RT_CLEAR_SLOCK(obj, i);
        else
            RT_SET_SLOCK(obj, i, sl);
        c = 1;
    }
    l = rt_locks(obj);
    for (k = 0; k < RT_SOUND_LOCKS; k++) {
        int16_t v = rt_seed_target_lock(src, track, parm, w, i, k);
        if (v == rt_lock_at(l, i, k))
            continue;
        if (v < 0)
            RT_LOCK_CLEAR(obj, i, k);
        else
            RT_LOCK_SET(obj, i, k, v, l[RT_LOCK_FINE + k]);
        c = 1;
    }
    return c;
}

/* write: room first (nothing is written when a lock cannot be stored), then each step below the
 * track's length that differs; one change notice at the end */
static inline int rt_seed_write(int track, const int16_t *parm, const struct seed_track *t)
{
    struct rt_seed_state *st = rt_seed_state();
    unsigned obj = rt_seed_obj(track);
    struct rt_snap *src;
    int i, n, e, changed = 0;

    st->written = 0;
    if (!obj || !rt_record(obj) || !rt_locks(obj) || !(src = rt_seed_source(track, obj)))
        return SEED_E_NA;
    n = rt_seed_steps(t, obj);
    e = rt_seed_room_src(track, obj, parm, t, src);
    if (e == SEED_OK) {
        for (i = 0; i < n; i++)
            changed += rt_seed_write_step(obj, track, parm, &t->s[i], i, src);
        if (changed)
            rt_changed(obj);
    }
    rt_seed_source_done(track);
    st->written = changed;
    return e;
}

static inline int rt_seed_current(void)
{
    unsigned obj;
    return rt_current(&obj, 0);
}

/* the pattern as stored: each track's length and trigs; caps, track and tracks. The mod fills
 * shared, grooves and ngrooves. */
static inline void rt_seed_context(struct seed_ctx *c, int track)
{
    int t, i;
    c->caps = rt_seed_device()->caps;
    c->track = track;
    c->tracks = RT_TRACKS;
    for (t = 0; t < RT_TRACKS; t++) {
        unsigned obj = rt_seed_obj(t);
        const uint8_t *r = obj ? rt_record(obj) : 0;
        for (i = 0; i < SEED_STEPS / 8; i++)
            c->trig[t][i] = 0;
        c->length[t] = 0;
        if (!r)
            continue;
        c->length[t] = (uint8_t)rt_seed_length(obj);
        for (i = 0; i < c->length[t]; i++)
            if (rt_flags_at(r, i) & SEED_TRIG)
                c->trig[t][i >> 3] |= (uint8_t)(1 << (i & 7));
    }
}

static inline void rt_seed_drop(void)
{
    struct rt_seed_state *st = rt_seed_state();
    int t;
    for (t = 0; t < RT_TRACKS; t++) {
        st->snap[t].valid = 0;
        st->tmp[t] = 0;
    }
}

/* snapshot: one bit per track in scope; any earlier snapshot is dropped first */
static inline int rt_seed_snapshot(uint16_t tracks)
{
    struct rt_seed_state *st = rt_seed_state();
    unsigned pat = RT_CURRENT_PAT(RT_APP());
    int t;
    rt_seed_drop();
    for (t = 0; t < RT_TRACKS; t++)
        if (tracks >> t & 1 && !rt_snap_take(&st->snap[t], RT_TRACK_OBJ(pat, t), pat, t)) {
            rt_seed_drop();
            return SEED_E_NA;
        }
    return SEED_OK;
}

/* restore: every track in the snapshot back, byte-identical in its record and lock store (I7);
 * the snapshot stays, so the audition goes on */
static inline void rt_seed_restore(void)
{
    struct rt_seed_state *st = rt_seed_state();
    int t;
    for (t = 0; t < RT_TRACKS; t++)
        if (!st->tmp[t])
            rt_snap_restore(&st->snap[t]);
}

static inline void rt_seed_changed(int track)
{
    unsigned obj = rt_seed_obj(track);
    if (obj)
        rt_changed(obj);
}

static inline const struct seed_dev *rt_seed_device(void)
{
    static const struct seed_param params[RT_NPARAMS] = RT_PARAMS;
    static const struct seed_caps caps = {
        RT_TRACKS, SEED_STEPS, RT_VOICE_FIELDS, -23, 23, SEED_LANES, RT_NPARAMS, params, { RT_P_SMP, RT_P_FRQ }
    };
    static const struct seed_dev d = {
        &caps, rt_seed_current, rt_seed_read, rt_seed_room, rt_seed_write, rt_seed_context,
        rt_seed_snapshot, rt_seed_restore, rt_seed_drop, rt_seed_changed
    };
    return &d;
}

/* steps the last write changed (for the mod's message) */
static inline int rt_seed_written(void)
{
    return rt_seed_state()->written;
}

#endif
