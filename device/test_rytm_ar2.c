/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for the Rytm device layer's version 1 interface (rytm_ar2.h, rt_seed_*), against a
 * model of the stock routines: their records, lock stores and row table, the reset stock's flag
 * routines make when a step is left without a trig (docs/status/0.4.md), and stock's row count.
 * The model is what the disassembly says, not the firmware: the emulator test checks the real
 * one (tools/tests/04-device.sh). Covers read, identity (I1), moves by origin with every lock
 * (design section 4), lanes, the room check (I8), the snapshot (I7), length and scope (I6). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RT_HOST
#define RT_ROWS_MAX rows_max
static int rows_max = 71;
#include "rytm_ar2.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---- the model: 13 tracks, object = track + 1, pattern 100 ---- */
static uint8_t rec[RT_TRACKS][RT_RECORD_SIZE], lks[RT_TRACKS][RT_LOCKS_SIZE], rows[RT_ROWS * RT_ROW];
static int len[RT_TRACKS], cur = 0, calls, notices;

static int T(unsigned obj) { return (int)obj - 1; }
unsigned rt_host_app(void) { return 1; }
unsigned rt_host_app_tracks(unsigned app) { (void)app; return 2; }
int rt_host_current_track(unsigned tracks) { (void)tracks; return cur; }
unsigned rt_host_current_pat(unsigned app) { (void)app; return 100; }
unsigned rt_host_track_obj(unsigned pat, int n) { (void)pat; return (unsigned)n + 1; }
int rt_host_track_length(unsigned obj) { return len[T(obj)]; }
void rt_host_memcpy(void *to, const void *from, unsigned n) { memcpy(to, from, n); }
uint8_t *rt_record(unsigned obj) { return rec[T(obj)]; }
uint8_t *rt_locks(unsigned obj) { return lks[T(obj)]; }
const uint8_t *rt_row_table(unsigned obj) { (void)obj; return rows; }
int rt_track_no(unsigned obj) { return T(obj); }
void rt_changed(unsigned obj) { (void)obj; notices++; }
/* the row type of (track, sound index), shaped as stock's (task D1) but not its tables: on a voice
 * track each index its own type, not the index itself; on the FX track 38..41 share 18..21's */
int rt_host_lock_type(int track, int k) { return track == RT_FX_TRACK ? (k < 38 ? k : k - 20) : 41 - k; }

static void put16(uint8_t *p, int v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static int flags(int t, int i) { return rec[t][2 * i] << 8 | rec[t][2 * i + 1]; }
static void set_flags(int t, int i, int f) { put16(&rec[t][2 * i], f); }
static int lock(int t, int i, int k) { return (int16_t)(lks[t][RT_STEP_LOCKS * i + 2 * k] << 8 | lks[t][RT_STEP_LOCKS * i + 2 * k + 1]); }

static int rows_used(void)
{
    int j, n = 0;
    for (j = 0; j < RT_ROWS; j++)
        n += rows[RT_ROW * j] != 0xff && rows[RT_ROW * j + 1] != 0xff;
    return n;
}

/* stock's mirror: a new (type, track) row while used + cost <= 71, for a fine parameter followed
 * by a row of type and track 0x80; freed after the last lock of its type on the track goes */
static void take_row(int t, int k)
{
    int j, fine = lks[t][RT_LOCK_FINE + k] != 0;
    for (j = 0; j < RT_ROWS; j++)
        if (rows[RT_ROW * j] == rt_host_lock_type(t, k) && rows[RT_ROW * j + 1] == t)
            return;
    if (rows_used() + 1 + fine > 71)
        return;
    for (j = 0; j < RT_ROWS - fine; j++)
        if (rows[RT_ROW * j] == 0xff && (!fine || rows[RT_ROW * (j + 1)] == 0xff)) {
            rows[RT_ROW * j] = (uint8_t)rt_host_lock_type(t, k);
            rows[RT_ROW * j + 1] = (uint8_t)t;
            if (fine)
                rows[RT_ROW * (j + 1)] = rows[RT_ROW * (j + 1) + 1] = 0x80;
            return;
        }
}

static void free_row(int t, int k)
{
    int i, kk, j, type = rt_host_lock_type(t, k);
    for (i = 0; i < SEED_STEPS; i++)
        for (kk = 0; kk < RT_SOUND_LOCKS; kk++)
            if (rt_host_lock_type(t, kk) == type && lks[t][RT_STEP_LOCKS * i + 2 * kk] != 0xff)
                return;
    for (j = 0; j < RT_ROWS; j++)
        if (rows[RT_ROW * j] == type && rows[RT_ROW * j + 1] == t) {
            rows[RT_ROW * j] = rows[RT_ROW * j + 1] = 0xff;
            if (j + 1 < RT_ROWS && rows[RT_ROW * (j + 1)] == 0x80 && rows[RT_ROW * (j + 1) + 1] == 0x80)
                rows[RT_ROW * (j + 1)] = rows[RT_ROW * (j + 1) + 1] = 0xff;
        }
}

/* 0x400c0b18, 0x400c0624, 0x400c070a: a step left without a trig */
static void reset(int t, int i)
{
    int k;
    rec[t][RT_NOTE + i] = rec[t][RT_VEL + i] = rec[t][RT_LEN + i] = 0xff;
    rec[t][RT_UTIME + i] = 0;
    rec[t][RT_RLEN + i] = SEED_NEW_RLEN;
    rec[t][RT_RRATE + i] = SEED_NEW_RRATE;
    rec[t][RT_RVEL + i] = 0;
    rec[t][RT_SLOCK + i] = 0xff;
    for (k = 0; k < 44; k++)
        put16(&lks[t][RT_STEP_LOCKS * i + 2 * k], -1);
    for (k = 0; k < RT_SOUND_LOCKS; k++)
        free_row(t, k);
}

void rt_host_set_trig(unsigned obj, int i, int on)
{
    int t = T(obj);
    calls++;
    if (i >= len[t])
        return;
    if (on) {
        if (!(flags(t, i) & 1)) {
            set_flags(t, i, flags(t, i) | 0x0381);
            rec[t][RT_COND + i] = 0xff;
        }
    } else {
        set_flags(t, i, flags(t, i) & ~1);
        reset(t, i);
    }
}

void rt_host_set_flag(unsigned obj, int i, int mask, int on)
{
    int t = T(obj);
    calls++;
    if (i >= len[t])
        return;
    set_flags(t, i, on ? flags(t, i) | mask : flags(t, i) & ~mask);
    if (!(flags(t, i) & 1))
        reset(t, i);
}

void rt_host_set_field(unsigned obj, int i, int value, int offset)
{
    calls++;
    if (i < len[T(obj)])
        rec[T(obj)][offset + i] = (uint8_t)value;
}

void rt_host_set_slock(unsigned obj, int i, int v)
{
    calls++;
    if (i < len[T(obj)])
        rec[T(obj)][RT_SLOCK + i] = T(obj) == RT_FX_TRACK ? 0xff : (uint8_t)v;
}

void rt_host_clear_slock(unsigned obj, int i)
{
    calls++;
    if (i < len[T(obj)])
        rec[T(obj)][RT_SLOCK + i] = 0xff;
}

int rt_host_lock_set(unsigned obj, int i, int k, int value, int fine)
{
    int t = T(obj);
    calls++;
    if (i > len[t])                    /* stock's check lets step == length through */
        return 0;
    put16(&lks[t][RT_STEP_LOCKS * i + 2 * k], value);
    lks[t][RT_LOCK_FINE + k] = (uint8_t)fine;
    take_row(t, k);
    return 1;
}

int rt_host_lock_clear(unsigned obj, int i, int k)
{
    calls++;
    if (i > len[T(obj)])
        return 0;
    put16(&lks[T(obj)][RT_STEP_LOCKS * i + 2 * k], -1);
    free_row(T(obj), k);
    return 1;
}

/* ---- fixtures ---- */

static uint32_t rng = 1;
static int rnd(int n) { rng = rng * 1103515245u + 12345u; return (int)(rng >> 16) % n; }

/* a pattern with trigs, fields, swing and other flag bits, sound locks, parameter locks on
 * several sound indexes (some fine), and locks on trigless steps */
static void fixture_n(uint32_t seed)
{
    int t, i, k;
    memset(rec, 0, sizeof rec);
    memset(lks, 0xff, sizeof lks);
    memset(rows, 0xff, sizeof rows);
    rng = seed;
    for (t = 0; t < RT_TRACKS; t++) {
        len[t] = 16;
        memset(&lks[t][RT_LOCK_FINE], 0, RT_LOCKS_SIZE - RT_LOCK_FINE);
        lks[t][RT_LOCK_FINE + 30] = 1;
        rec[t][RT_DEF_NOTE] = 60;
        rec[t][RT_DEF_VEL] = 100;
        for (i = 0; i < SEED_STEPS; i++) {
            int trig = rnd(3) == 0;
            set_flags(t, i, (trig ? 0x0381 : 0) | (i & 1 ? SEED_SWING : 0) | (trig && rnd(4) == 0 ? SEED_ACCENT : 0) |
                            (trig && rnd(5) == 0 ? 0x0800 : 0));
            rec[t][RT_NOTE + i] = trig ? (uint8_t)(48 + rnd(24)) : 0xff;
            rec[t][RT_VEL + i] = trig ? (uint8_t)(1 + rnd(127)) : 0xff;
            rec[t][RT_LEN + i] = 0xff;
            rec[t][RT_UTIME + i] = (uint8_t)(trig ? rnd(47) - 23 : 0);
            rec[t][RT_RLEN + i] = SEED_NEW_RLEN;
            rec[t][RT_RRATE + i] = SEED_NEW_RRATE;
            rec[t][RT_RVEL + i] = 0;
            rec[t][RT_COND + i] = (uint8_t)(trig && rnd(3) == 0 ? rnd(22) : -1);
            rec[t][RT_SLOCK + i] = trig && t != RT_FX_TRACK && rnd(6) == 0 ? (uint8_t)rnd(128) : 0xff;
            if (trig || rnd(8) == 0)
                for (k = 0; k < 3; k++) {
                    int kk = (int[]){ 19, 30, 35 }[rnd(3)];
                    put16(&lks[t][RT_STEP_LOCKS * i + 2 * kk], rnd(128) << 8 | (kk == 30 ? rnd(256) : 0));
                    take_row(t, kk);
                }
        }
    }
}

static void fixture(void) { fixture_n(7); }

static uint8_t rec0[RT_TRACKS][RT_RECORD_SIZE], lks0[RT_TRACKS][RT_LOCKS_SIZE], rows0[sizeof rows];
static void save(void) { memcpy(rec0, rec, sizeof rec); memcpy(lks0, lks, sizeof lks); memcpy(rows0, rows, sizeof rows); }
static int same(void) { return !memcmp(rec0, rec, sizeof rec) && !memcmp(lks0, lks, sizeof lks); }
static int same_track(int t) { return !memcmp(rec0[t], rec[t], sizeof rec[t]) && !memcmp(lks0[t], lks[t], sizeof lks[t]); }

static const int16_t lanes_smp_frq[SEED_LANES] = { RT_P_SMP, RT_P_FRQ };
static struct seed_track a, b;

/* ---- tests ---- */

static void test_caps(void)
{
    const struct seed_dev *d = rt_seed_device();
    const struct seed_caps *c = d->caps;
    int i, j;
    CHECK(c->tracks == 13 && c->steps == 64 && c->lanes == 2);
    CHECK(c->utime_lo == -23 && c->utime_hi == 23);
    CHECK(c->nparams == RT_NPARAMS);
    CHECK(c->params[c->lane_parm[0]].id == RT_SAMPLE_SLOT && !strcmp(c->params[c->lane_parm[0]].name, "SMP"));
    CHECK(c->params[c->lane_parm[1]].id == 30 && !strcmp(c->params[c->lane_parm[1]].name, "FRQ"));
    for (i = 0; i < c->nparams; i++) {
        CHECK(c->params[i].id > 0 && c->params[i].id < RT_SOUND_LOCKS);
        CHECK(strlen(c->params[i].name) >= 2 && strlen(c->params[i].name) < SEED_NAME);
        CHECK(c->params[i].lo == 0 && c->params[i].hi == 127);
        for (j = 0; j < i; j++)
            CHECK(c->params[i].id != c->params[j].id && strcmp(c->params[i].name, c->params[j].name));
    }
}

static void test_read(void)
{
    int i;
    fixture();
    len[2] = 24;
    CHECK(rt_seed_read(2, lanes_smp_frq, &a) == SEED_OK);
    CHECK(a.length == 24 && a.fields == RT_VOICE_FIELDS && a.default_note == 60 && a.default_vel == 100);
    for (i = 0; i < SEED_STEPS; i++) {
        CHECK(a.s[i].flags == flags(2, i) && a.s[i].origin == i && a.s[i].spare == 0);
        CHECK(a.s[i].vel == rec[2][RT_VEL + i] && a.s[i].utime == (int8_t)rec[2][RT_UTIME + i]);
        CHECK(a.s[i].lane[0] == (lock(2, i, 19) < 0 ? SEED_NO_LOCK : lock(2, i, 19) >> 8));
        CHECK(a.s[i].lane[1] == (lock(2, i, 30) < 0 ? SEED_NO_LOCK : lock(2, i, 30) >> 8));
    }
    CHECK(rt_seed_read(RT_FX_TRACK, lanes_smp_frq, &a) == SEED_OK);
    CHECK(a.fields == RT_FX_FIELDS && a.s[0].lane[0] == SEED_NO_LOCK && a.s[5].lane[1] == SEED_NO_LOCK);
    CHECK(rt_seed_read(13, lanes_smp_frq, &a) == SEED_E_NA && rt_seed_read(-1, 0, &a) == SEED_E_NA);
}

/* I1: what was read, written back, calls nothing and changes nothing, on every track and length */
static void test_identity(void)
{
    int t;
    fixture();
    len[4] = 64;
    len[5] = 3;
    save();
    for (t = 0; t < RT_TRACKS; t++) {
        CHECK(rt_seed_read(t, lanes_smp_frq, &a) == SEED_OK);
        calls = notices = 0;
        CHECK(rt_seed_write(t, lanes_smp_frq, &a) == SEED_OK);
        CHECK(calls == 0 && notices == 0 && rt_seed_written() == 0);
    }
    CHECK(same() && !memcmp(rows0, rows, sizeof rows));
    CHECK(!rt_seed_state()->snap[0].valid);           /* the write's own snapshot is gone */
}

/* steps i get origin (i - k) mod n: a rotation as the chain's PLACE gives it; each step must equal
 * its origin as it was, all 42 locks, the sound lock and the flag bits, swing excepted */
static void rotate_origins(struct seed_track *t, int k)
{
    int i, n = t->length;
    struct seed_track o = *t;
    for (i = 0; i < n; i++) {
        int f = ((i - k) % n + n) % n;
        t->s[i] = o.s[f];
        t->s[i].flags = (uint16_t)((o.s[f].flags & ~SEED_SWING) | (o.s[i].flags & SEED_SWING));
    }
}

static int flags0(int t, int i) { return rec0[t][2 * i] << 8 | rec0[t][2 * i + 1]; }

/* step i of track t is what step f was, all of it; swing is position i's */
static int step_is(int t, int i, int f)
{
    static const int off[] = { RT_NOTE, RT_VEL, RT_LEN, RT_UTIME, RT_RLEN, RT_RRATE, RT_RVEL, RT_COND, RT_SLOCK };
    int k, ok = (flags(t, i) & ~SEED_SWING) == (flags0(t, f) & ~SEED_SWING) &&
                (flags(t, i) & SEED_SWING) == (flags0(t, i) & SEED_SWING);
    for (k = 0; k < (int)(sizeof off / sizeof off[0]); k++)
        ok &= rec[t][off[k] + i] == rec0[t][off[k] + f];
    return ok && !memcmp(&lks[t][RT_STEP_LOCKS * i], &lks0[t][RT_STEP_LOCKS * f], RT_STEP_LOCKS);
}

static void test_move(void)
{
    int t, i, k, n, ok;
    static const int shifts[] = { 1, -1, 5, 15 };
    for (t = 0; t < RT_TRACKS; t++)
        for (k = 0; k < 4; k++) {
            fixture();
            len[t] = (int[]){ 16, 13, 64, 16 }[k];
            save();
            CHECK(rt_seed_read(t, lanes_smp_frq, &a) == SEED_OK);
            rotate_origins(&a, shifts[k]);
            CHECK(rt_seed_write(t, lanes_smp_frq, &a) == SEED_OK);
            n = len[t];
            for (ok = 1, i = 0; i < n; i++)
                ok &= step_is(t, i, ((i - shifts[k]) % n + n) % n);
            CHECK(ok);
            for (ok = 1, i = n; i < SEED_STEPS; i++)           /* I6: past the length, untouched */
                ok &= step_is(t, i, i);
            CHECK(ok);
            for (i = 0; i < RT_TRACKS; i++)                    /* I6: other tracks untouched */
                if (i != t)
                    CHECK(same_track(i));
            CHECK(!memcmp(rows0, rows, sizeof rows));          /* a move needs no new row */
            CHECK(!memcmp(&lks[t][RT_LOCK_FINE], &lks0[t][RT_LOCK_FINE], RT_LOCKS_SIZE - RT_LOCK_FINE));
            /* and back: rotating the other way gives the original, byte for byte */
            CHECK(rt_seed_read(t, lanes_smp_frq, &a) == SEED_OK);
            rotate_origins(&a, -shifts[k]);
            CHECK(rt_seed_write(t, lanes_smp_frq, &a) == SEED_OK);
            CHECK(same());
        }
}

/* the case the emulator test makes by hand: one step with three locks, moved by one */
static void test_three_locks(void)
{
    int i;
    fixture();
    for (i = 0; i < 16; i++) {
        set_flags(0, i, i & 1 ? SEED_SWING : 0);
        reset(0, i);
    }
    set_flags(0, 0, 0x0381);
    rec[0][RT_VEL + 0] = 90;
    put16(&lks[0][2 * 30], 70 << 8 | 0x40);
    put16(&lks[0][2 * 31], 20 << 8);
    put16(&lks[0][2 * 27], 99 << 8);
    save();
    CHECK(rt_seed_read(0, lanes_smp_frq, &a) == SEED_OK);
    rotate_origins(&a, 1);
    CHECK(rt_seed_write(0, lanes_smp_frq, &a) == SEED_OK && rt_seed_written() == 2);
    CHECK((flags(0, 1) & 1) && !(flags(0, 0) & 1) && rec[0][RT_VEL + 1] == 90);
    CHECK(lock(0, 1, 30) == (70 << 8 | 0x40) && lock(0, 1, 31) == 20 << 8 && lock(0, 1, 27) == 99 << 8);
    CHECK(lock(0, 0, 30) == -1 && lock(0, 0, 31) == -1 && lock(0, 0, 27) == -1);
    CHECK(flags(0, 1) & SEED_SWING);
}

static void test_lanes(void)
{
    int i, ok;
    fixture();
    save();
    CHECK(rt_seed_read(1, lanes_smp_frq, &a) == SEED_OK);
    for (i = 0; i < 16; i++) {
        a.s[i].lane[0] = a.s[i].flags & SEED_TRIG ? (int16_t)(i + 1) : SEED_NO_LOCK;
        /* lane 2 as read: its fine part must stay */
    }
    CHECK(rt_seed_write(1, lanes_smp_frq, &a) == SEED_OK);
    for (ok = 1, i = 0; i < 16; i++) {
        ok &= lock(1, i, 19) == (flags(1, i) & 1 ? (i + 1) << 8 : -1);
        ok &= lock(1, i, 30) == (int16_t)(lks0[1][RT_STEP_LOCKS * i + 60] << 8 | lks0[1][RT_STEP_LOCKS * i + 61]);
        ok &= lock(1, i, 35) == (int16_t)(lks0[1][RT_STEP_LOCKS * i + 70] << 8 | lks0[1][RT_STEP_LOCKS * i + 71]);
    }
    CHECK(ok);
    /* a lane on a new parameter takes its row; a value outside the range is refused */
    CHECK(rt_seed_read(1, lanes_smp_frq, &a) == SEED_OK);
    {
        const int16_t parm[SEED_LANES] = { 0, RT_P_FRQ };      /* TUN, sound index 17 */
        a.s[2].lane[0] = 40;
        CHECK(rt_seed_write(1, parm, &a) == SEED_OK && lock(1, 2, 17) == 40 << 8);
        save();
        a.s[3].lane[0] = 128;
        CHECK(rt_seed_write(1, parm, &a) == SEED_E_ARG && same());
        a.s[3].lane[0] = -1;
        CHECK(rt_seed_write(1, parm, &a) == SEED_E_ARG && same());
    }
    /* the FX track has no lanes: a lane value there is ignored */
    fixture();
    save();
    CHECK(rt_seed_read(RT_FX_TRACK, lanes_smp_frq, &a) == SEED_OK);
    a.s[0].lane[0] = 5;
    calls = 0;
    CHECK(rt_seed_write(RT_FX_TRACK, lanes_smp_frq, &a) == SEED_OK && calls == 0 && same());
}

/* I8: a lock that cannot be stored is refused before anything is written */
static void test_room(void)
{
    const int16_t tun[SEED_LANES] = { 0, RT_P_FRQ }, fin[SEED_LANES] = { 1, RT_P_FRQ };
    int j;
    fixture();
    /* fill the row table to 70 of 71 */
    for (j = 0; rows_used() < 70; j++)
        if (rows[RT_ROW * j] == 0xff) {
            rows[RT_ROW * j] = 0x50;
            rows[RT_ROW * j + 1] = (uint8_t)j;
        }
    save();
    CHECK(rt_seed_read(3, tun, &a) == SEED_OK);
    a.s[0].lane[0] = 10;                                   /* TUN (17): one new row, fits */
    a.s[4].flags ^= SEED_ACCENT;
    CHECK(rt_seed_room(3, tun, &a) == SEED_OK && same());
    rows_max = 70;                                         /* now it does not */
    calls = notices = 0;
    CHECK(rt_seed_room(3, tun, &a) == SEED_E_ROOM);
    CHECK(rt_seed_write(3, tun, &a) == SEED_E_ROOM && calls == 0 && notices == 0 && same());
    CHECK(!rt_seed_state()->snap[3].valid);
    rows_max = 71;
    /* FIN (18) has a row type of its own on a voice track: with the table full it is refused */
    CHECK(rt_seed_write(3, tun, &a) == SEED_OK && rows_used() == 71 && lock(3, 0, 17) == 10 << 8);
    CHECK(rt_seed_read(3, fin, &b) == SEED_OK);
    b.s[1].lane[0] = 3;
    save();
    CHECK(rt_seed_write(3, fin, &b) == SEED_E_ROOM && same() && !memcmp(rows0, rows, sizeof rows));
    /* a lock already stored on the track needs no new row even with the table full */
    CHECK(rt_seed_read(3, lanes_smp_frq, &b) == SEED_OK);
    rows_max = 0;
    rotate_origins(&b, 2);
    CHECK(rt_seed_write(3, lanes_smp_frq, &b) == SEED_OK);
    rows_max = 71;
}

/* I8 by stock's own counts (task D1): the type is (track, index), not the index; a fine parameter
 * takes two rows; the FX track keeps rows too, and its indexes 38..41 share 18..21's types */
static void test_room_rows(void)
{
    const int16_t tun[SEED_LANES] = { 0, RT_P_FRQ };
    int j, filled;
    for (filled = 69; filled <= 70; filled++) {
        fixture();
        for (j = 0; rows_used() < filled; j++)
            if (rows[RT_ROW * j] == 0xff) {
                rows[RT_ROW * j] = 0x50;
                rows[RT_ROW * j + 1] = (uint8_t)j;
            }
        lks[3][RT_LOCK_FINE + 17] = 1;                      /* TUN fine on SD: two rows */
        save();
        CHECK(rt_seed_read(3, tun, &a) == SEED_OK);
        a.s[0].lane[0] = 10;
        if (filled == 70) {
            CHECK(rt_seed_write(3, tun, &a) == SEED_E_ROOM && same() && !memcmp(rows0, rows, sizeof rows));
        } else {
            CHECK(rt_seed_write(3, tun, &a) == SEED_OK && lock(3, 0, 17) == 10 << 8 && rows_used() == 71);
        }
    }
    /* a row of another index's type on the track is not this index's row: the type of TUN (17) on
     * a voice track is 24; a row (17, track) must not count */
    fixture();
    for (j = 0; rows_used() < 70; j++)
        if (rows[RT_ROW * j] == 0xff) {
            rows[RT_ROW * j] = 0x50;
            rows[RT_ROW * j + 1] = (uint8_t)j;
        }
    while (rows[RT_ROW * j] != 0xff)
        j++;
    rows[RT_ROW * j] = 17;
    rows[RT_ROW * j + 1] = 3;
    save();
    CHECK(rt_seed_read(3, tun, &a) == SEED_OK);
    a.s[0].lane[0] = 10;
    CHECK(rows_used() == 71 && rt_seed_write(3, tun, &a) == SEED_E_ROOM && same());
    rows[RT_ROW * j] = (uint8_t)rt_host_lock_type(3, 17);
    CHECK(rt_seed_write(3, tun, &a) == SEED_OK && lock(3, 0, 17) == 10 << 8);
    /* FX: a lock with no row (made past the limit) moved by origins takes a row when one fits, and
     * is refused when none does; one sharing a stored type needs none */
    fixture();
    put16(&lks[RT_FX_TRACK][2 * 10], 5 << 8);              /* no take_row: not stored */
    save();
    CHECK(rt_seed_read(RT_FX_TRACK, 0, &a) == SEED_OK);
    rotate_origins(&a, 3);
    rows_max = rows_used();
    CHECK(rt_seed_write(RT_FX_TRACK, 0, &a) == SEED_E_ROOM && same());
    rows_max = 71;
    CHECK(rt_seed_write(RT_FX_TRACK, 0, &a) == SEED_OK && lock(RT_FX_TRACK, 3, 10) == 5 << 8);
    fixture();
    put16(&lks[RT_FX_TRACK][2 * 18], 7 << 8);
    take_row(RT_FX_TRACK, 18);
    put16(&lks[RT_FX_TRACK][RT_STEP_LOCKS + 2 * 38], 6 << 8);  /* 38: 18's type, stored with it */
    save();
    CHECK(rt_seed_read(RT_FX_TRACK, 0, &a) == SEED_OK);
    rotate_origins(&a, 3);
    rows_max = 0;
    CHECK(rt_seed_write(RT_FX_TRACK, 0, &a) == SEED_OK && lock(RT_FX_TRACK, 4, 38) == 6 << 8);
    rows_max = 71;
    /* two new indexes of one type (FX 21 and 41, no row yet) cost one row */
    fixture();
    put16(&lks[RT_FX_TRACK][2 * 21], 7 << 8);
    put16(&lks[RT_FX_TRACK][RT_STEP_LOCKS + 2 * 41], 6 << 8);
    save();
    CHECK(rt_seed_read(RT_FX_TRACK, 0, &a) == SEED_OK);
    rotate_origins(&a, 3);
    rows_max = rows_used();
    CHECK(rt_seed_write(RT_FX_TRACK, 0, &a) == SEED_E_ROOM && same());
    rows_max = rows_used() + 1;
    CHECK(rt_seed_write(RT_FX_TRACK, 0, &a) == SEED_OK && lock(RT_FX_TRACK, 3, 21) == 7 << 8 &&
          lock(RT_FX_TRACK, 4, 41) == 6 << 8);
    rows_max = 71;
}

/* origins: none keeps the locks stored at the position (lanes excepted); out of range is refused */
static void test_origins(void)
{
    int i, ok;
    fixture();
    save();
    CHECK(rt_seed_read(6, lanes_smp_frq, &a) == SEED_OK);
    for (i = 0; i < 16; i++) {
        seed_step_clear(&a.s[i]);
        if (i % 4 == 0)
            a.s[i].flags |= SEED_TRIG;
    }
    CHECK(rt_seed_write(6, lanes_smp_frq, &a) == SEED_OK);
    for (ok = 1, i = 0; i < 16; i++) {
        int k;
        ok &= !!(flags(6, i) & 1) == (i % 4 == 0);
        ok &= (flags(6, i) & SEED_SWING) == (rec0[6][2 * i + 1] & SEED_SWING);
        for (k = 0; k < RT_SOUND_LOCKS; k++)
            ok &= lock(6, i, k) == (int16_t)(lks0[6][RT_STEP_LOCKS * i + 2 * k] << 8 | lks0[6][RT_STEP_LOCKS * i + 2 * k + 1]);
        ok &= rec[6][RT_SLOCK + i] == rec0[6][RT_SLOCK + i];
    }
    CHECK(ok);
    fixture();
    save();
    CHECK(rt_seed_read(6, lanes_smp_frq, &a) == SEED_OK);
    a.s[3].origin = 16;                                    /* past the length */
    CHECK(rt_seed_write(6, lanes_smp_frq, &a) == SEED_E_ARG && same());
    a.s[3].origin = -2;
    CHECK(rt_seed_room(6, lanes_smp_frq, &a) == SEED_E_ARG && same());
}

/* A generated trig (no origin) on a step without one gets the SYN / SMP / ENV bits stock's place
 * routine sets, as a trig from the TRIG keys; a trig that was there keeps its own */
static void test_new_trig_bits(void)
{
    int i, placed = 0, kept = 0, ok = 1;
    fixture();
    save();
    CHECK(rt_seed_read(6, lanes_smp_frq, &a) == SEED_OK);
    for (i = 0; i < 16; i++) {
        seed_step_clear(&a.s[i]);
        a.s[i].flags |= SEED_TRIG;
    }
    CHECK(rt_seed_write(6, lanes_smp_frq, &a) == SEED_OK);
    for (i = 0; i < 16; i++) {
        int was = rec0[6][2 * i + 1] & 1;
        uint16_t f0 = (uint16_t)(rec0[6][2 * i] << 8 | rec0[6][2 * i + 1]);
        ok &= (flags(6, i) & 1) != 0;
        if (!was) {
            placed++;
            ok &= (flags(6, i) & RT_STATE_BITS) == RT_STATE_BITS;
        } else {
            kept++;
            ok &= (flags(6, i) & RT_STATE_BITS) == (f0 & RT_STATE_BITS);
        }
    }
    CHECK(ok);
    CHECK(placed > 0 && kept > 0);              /* the fixture has both kinds */
}

/* I7: snapshot, takes written from it, restore: byte-identical; the snapshot is the source */
static void test_snapshot(void)
{
    int i, take;
    fixture();
    save();
    CHECK(rt_seed_snapshot(1u << 0 | 1u << 7) == SEED_OK);
    for (take = 1; take <= 3; take++) {
        CHECK(rt_seed_read(0, lanes_smp_frq, &a) == SEED_OK);
        /* each take from the original: rotate the original by `take`, with lane values */
        memcpy(&b, &a, sizeof b);
        for (i = 0; i < 16; i++) {
            b.s[i].origin = (int8_t)i;
        }
        rotate_origins(&b, take);
        for (i = 0; i < 16; i++)
            if (b.s[i].flags & SEED_TRIG)
                b.s[i].lane[0] = (int16_t)(take * 10 + i);
        CHECK(rt_seed_write(0, lanes_smp_frq, &b) == SEED_OK);
        CHECK(rt_seed_read(7, 0, &a) == SEED_OK);
        a.s[2].flags ^= SEED_TRIG;
        a.s[3].vel = 7;
        CHECK(rt_seed_write(7, 0, &a) == SEED_OK);
        CHECK(!same());
        CHECK(rt_seed_state()->snap[0].valid && rt_seed_state()->snap[7].valid);
        rt_seed_restore();
        CHECK(same());
    }
    /* origins come from the snapshot, not from the take on the track: write take 1 twice */
    CHECK(rt_seed_read(0, lanes_smp_frq, &a) == SEED_OK);
    rotate_origins(&a, 1);
    CHECK(rt_seed_write(0, lanes_smp_frq, &a) == SEED_OK);
    CHECK(rt_seed_write(0, lanes_smp_frq, &a) == SEED_OK && rt_seed_written() == 0);
    rt_seed_drop();
    CHECK(!rt_seed_state()->snap[0].valid && !rt_seed_state()->snap[7].valid);
    save();
    rt_seed_restore();                                     /* after a drop: nothing to restore */
    CHECK(same());
}

static void test_context(void)
{
    struct seed_ctx c;
    int t, i, ok = 1;
    fixture();
    len[3] = 40;
    memset(&c, 0xee, sizeof c);
    rt_seed_context(&c, 3);
    CHECK(c.caps == rt_seed_device()->caps && c.track == 3 && c.tracks == 13);
    for (t = 0; t < RT_TRACKS; t++) {
        CHECK(c.length[t] == len[t]);
        for (i = 0; i < SEED_STEPS; i++)
            ok &= seed_ctx_trig(&c, t, i) == (i < len[t] && (flags(t, i) & 1));
    }
    CHECK(ok);
    /* a track in Euclidean mode: the pattern that mode plays (its table), not its stored trigs */
    rec[4][RT_EUCLID] = 1;
    for (i = 0; i < 64; i++)
        rec[4][RT_EUCLID_TAB + i] = (uint8_t)(i % 3 == 0 ? i / 3 : 0xff);
    rt_seed_context(&c, 3);
    for (ok = 1, i = 0; i < SEED_STEPS; i++)
        ok &= seed_ctx_trig(&c, 4, i) == (i < len[4] && i % 3 == 0);
    CHECK(ok);
    rec[4][RT_EUCLID] = 0;
    rt_seed_context(&c, 3);
    for (ok = 1, i = 0; i < SEED_STEPS; i++)
        ok &= seed_ctx_trig(&c, 4, i) == (i < len[4] && (flags(4, i) & 1));
    CHECK(ok);
    cur = 5;
    CHECK(rt_seed_current() == 5 && rt_seed_device()->current() == 5);
    cur = 0;
    notices = 0;
    rt_seed_changed(2);
    rt_seed_changed(13);
    CHECK(notices == 1);
}


/* Fuzzing the layer (0.7): random patterns, lengths, lane parameters, row limits and takes, each
 * written from the snapshot and checked: a refused write changes nothing (I8); other tracks and
 * steps past the length are untouched (I6); a trig carries the take's fields and lanes, and its
 * origin's sound lock and other locks; the row table stays within the limit; the restore gives
 * the original back byte for byte (I7). DEV_FUZZ=n sets the number of cases. */

static void random_take(struct seed_track *t, int n)
{
    int i, l;
    for (i = 0; i < n; i++) {
        struct seed_step *s = &t->s[i];
        int o = rnd(5) == 0 ? SEED_NO_ORIGIN : rnd(n);
        *s = t->s[o >= 0 && o < n ? o : i];
        s->origin = (int8_t)o;
        s->flags = (uint16_t)(rnd(2) ? s->flags | SEED_TRIG : s->flags & ~SEED_TRIG);
        if (rnd(3) == 0)
            s->flags ^= SEED_ACCENT;
        if (s->flags & SEED_TRIG) {
            s->note = (uint8_t)(rnd(128));
            s->vel = (uint8_t)(1 + rnd(127));
            s->len = (uint8_t)rnd(128);
            s->utime = (int8_t)(rnd(47) - 23);
            s->cond = (int8_t)(rnd(3) ? -1 : rnd(65));
            s->rrate = (uint8_t)rnd(17);
            s->rvel = (int8_t)(rnd(256) - 128);
        }
        for (l = 0; l < SEED_LANES; l++)
            s->lane[l] = (int16_t)(rnd(4) == 0 ? SEED_NO_LOCK : rnd(128));
    }
    /* one take in 20 has a lane value out of range, one in 20 an origin past the length: refused */
    if (rnd(20) == 0)
        t->s[rnd(n)].lane[rnd(SEED_LANES)] = (int16_t)(rnd(2) ? 128 : -1);
    if (rnd(20) == 0)
        t->s[rnd(n)].origin = (int8_t)(n + rnd(4));
}

static void test_fuzz(int cases)
{
    int c;
    for (c = 0; c < cases; c++) {
        int t, n, i, k, l, w, e, ok = 1, took, kl[SEED_LANES];
        int16_t parm[SEED_LANES];
        fixture_n(1000u + (uint32_t)c);
        rng = 0x9e3779b9u * (uint32_t)(c + 1);
        t = rnd(RT_TRACKS);
        len[t] = n = 1 + rnd(64);
        parm[0] = (int16_t)rnd(RT_NPARAMS);
        do
            parm[1] = (int16_t)rnd(RT_NPARAMS);              /* the engine refuses two lanes on one */
        while (parm[1] == parm[0]);
        for (l = 0; l < SEED_LANES; l++)
            kl[l] = rt_seed_lane_k(t, parm, l);
        rows_max = rnd(3) ? 71 : rows_used() + rnd(3);
        save();
        CHECK(rt_seed_snapshot(1u << t) == SEED_OK);
        for (w = took = 0; w < 1 + rnd(3); w++) {
            CHECK(rt_seed_read(t, parm, &a) == SEED_OK);
            for (i = 0; i < SEED_STEPS; i++)                   /* a take from the original */
                a.s[i].origin = (int8_t)i;
            random_take(&a, n);
            memcpy(&b, &a, sizeof b);
            {
                static uint8_t r1[RT_TRACKS][RT_RECORD_SIZE], l1[RT_TRACKS][RT_LOCKS_SIZE];
                memcpy(r1, rec, sizeof rec);
                memcpy(l1, lks, sizeof lks);
                e = rt_seed_room(t, parm, &b);
                calls = 0;
                CHECK(rt_seed_write(t, parm, &b) == e);
                if (e != SEED_OK) {                            /* I8: refused, nothing written */
                    CHECK(calls == 0 && !memcmp(r1, rec, sizeof rec) && !memcmp(l1, lks, sizeof lks));
                    continue;
                }
            }
            took++;
            CHECK(rows_used() <= 71);
            for (i = 0; i < RT_TRACKS; i++)                    /* I6 */
                if (i != t)
                    ok &= same_track(i);
            for (i = n; i < SEED_STEPS; i++)
                ok &= step_is(t, i, i);
            for (i = 0; i < n; i++) {
                const struct seed_step *s = &b.s[i];
                int from = s->origin >= 0 ? s->origin : i;
                if (!(s->flags & SEED_TRIG)) {
                    ok &= !(flags(t, i) & 1);
                    continue;
                }
                ok &= (flags(t, i) & 1) && !!(flags(t, i) & SEED_ACCENT) == !!(s->flags & SEED_ACCENT);
                ok &= (flags(t, i) & SEED_SWING) == (flags0(t, i) & SEED_SWING);
                ok &= rec[t][RT_NOTE + i] == s->note && rec[t][RT_VEL + i] == s->vel && rec[t][RT_LEN + i] == s->len;
                ok &= (int8_t)rec[t][RT_UTIME + i] == s->utime && (int8_t)rec[t][RT_COND + i] == s->cond;
                ok &= rec[t][RT_RRATE + i] == s->rrate && (int8_t)rec[t][RT_RVEL + i] == s->rvel;
                ok &= rec[t][RT_SLOCK + i] == (t == RT_FX_TRACK ? 0xff : rec0[t][RT_SLOCK + from]);
                for (k = 0; k < RT_SOUND_LOCKS; k++) {
                    int lane = -1, v0 = (int16_t)(lks0[t][RT_STEP_LOCKS * from + 2 * k] << 8 |
                                                  lks0[t][RT_STEP_LOCKS * from + 2 * k + 1]);
                    for (l = 0; l < SEED_LANES; l++)
                        if (kl[l] == k)
                            lane = l;
                    if (lane < 0)
                        ok &= lock(t, i, k) == v0;             /* the origin's lock */
                    else if (s->lane[lane] == SEED_NO_LOCK)
                        ok &= lock(t, i, k) == -1;
                    else
                        ok &= lock(t, i, k) >> 8 == s->lane[lane];
                }
            }
            if (!ok) {
                printf("FAIL device fuzz case %d: track %d length %d parm %d %d write %d\n", c, t, n, parm[0], parm[1], w);
                break;
            }
        }
        CHECK(ok);
        rt_seed_restore();                                     /* I7 */
        CHECK(same());
        if (!same())
            printf("FAIL device fuzz case %d: restore (track %d, %d takes written)\n", c, t, took);
        rt_seed_drop();
        rows_max = 71;
    }
}

int main(void)
{
    test_caps();
    test_read();
    test_identity();
    test_move();
    test_three_locks();
    test_lanes();
    test_room();
    test_room_rows();
    test_origins();
    test_new_trig_bits();
    test_snapshot();
    test_context();
    test_fuzz(getenv("DEV_FUZZ") ? atoi(getenv("DEV_FUZZ")) : 3000);
    printf("test_rytm_ar2: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
