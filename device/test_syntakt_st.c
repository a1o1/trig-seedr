/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for the Syntakt device layer (syntakt_st.h, st_seed_*; plan-syntakt.md S2), against a
 * model of the stock routines as docs/firmware-notes-st142.md gives them: the 0x499-byte records, the
 * lock matrix (72 int16 a step, word 71 the count, a byte per slot), the reset a placed trig and a
 * step left without a trig get, and the setters' length check. The model is what the notes say, not
 * the firmware: tools/st_s2.sh checks the real one in analog-emu. Covers the conditions' translation,
 * read, identity (I1), moves by origin with every lock and chord note (design section 4), lanes,
 * the settings check (I8), new trigs, the snapshot (I7), length and scope (I6), the context. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ST_HOST
#include "syntakt_st.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
/* the model's own argument checks: failures count, passes do not */
#define MODEL_OK(c) do { if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---- the model: 13 tracks; pattern 0x1000, its track objects 0x70 apart; lock model 0x9000 ---- */
#define PAT 0x1000u
#define MODEL 0x9000u
static uint8_t rec[ST_TRACKS][ST_RECORD_SIZE], lks[ST_TRACKS][ST_LOCKS_SIZE];
static int len[ST_TRACKS], cur = 0, calls, notices, grid;

static int T(unsigned obj) { return (int)((obj - PAT) / ST_PAT_STRIDE) - 1; }
uint8_t st_host_seq_running, st_host_seq_last[13];
unsigned st_host_app(void) { return 1; }
unsigned st_host_active_pattern(unsigned app) { (void)app; return PAT; }
unsigned st_host_active_track(unsigned app) { (void)app; return st_track_obj(PAT, cur); }
int st_host_track_length(unsigned obj) { return len[T(obj)]; }
uint8_t *st_record(unsigned obj) { return rec[T(obj)]; }
unsigned st_lock_model(unsigned obj) { (void)obj; return MODEL; }
uint8_t *st_locks(unsigned obj) { return lks[T(obj)]; }
int st_track_no(unsigned obj) { return T(obj); }
void st_notify_step(unsigned obj, int i) { (void)obj; (void)i; notices++; }
int st_grid_recording(void) { return grid; }
/* a parameter's slot, shaped as stock's table (every id its slot, some ids none) but not its values */
int st_host_slot(int id) { return id % 17 == 0 ? 255 : id % ST_SLOTS; }

static void put16(uint8_t *p, int v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static int flags(int t, int i) { return rec[t][2 * i] << 8 | rec[t][2 * i + 1]; }
static void set_flags(int t, int i, int f) { put16(&rec[t][2 * i], f); }
static int lock(int t, int i, int k) { return st_lock_at(lks[t], i, k); }

/* the step's lock count (word 71) and the track's byte per slot, as 0x4001729e and 0x400175c2 keep them */
static void recount(int t, int i, int k)
{
    int kk, n = 0, any = 0, ii;
    for (kk = 0; kk < ST_SLOTS; kk++)
        n += lock(t, i, kk) >= 0;
    put16(&lks[t][2 * (ST_STEP_LOCKS * i + 71)], n);
    for (ii = 0; ii < SEED_STEPS; ii++)
        any |= lock(t, ii, k) >= 0;
    lks[t][ST_LOCK_IDS + k] = (uint8_t)any;
}

void st_host_lock_set(unsigned model, int track, int i, int k, int value)
{
    calls++;
    MODEL_OK(model == MODEL && k >= 0 && k < ST_SLOTS && value >= 0);
    put16(&lks[track][2 * (ST_STEP_LOCKS * i + k)], value);
    recount(track, i, k);
}

void st_host_lock_clear(unsigned model, int track, int i, int k)
{
    calls++;
    MODEL_OK(model == MODEL && k >= 0 && k < ST_SLOTS);
    put16(&lks[track][2 * (ST_STEP_LOCKS * i + k)], -1);
    recount(track, i, k);
}

/* 0x40029c74: a step reset: NOTE, NOT2..4, VEL, LEN default; micro timing 0; no condition; +0x240
 * none; retrig 14 / 12 / 0; the flag word's bits 10..15 cleared */
static void reset(int t, int i)
{
    rec[t][ST_NOTE + i] = rec[t][ST_NOT2 + i] = rec[t][ST_NOT3 + i] = rec[t][ST_NOT4 + i] = 0xff;
    rec[t][ST_VEL + i] = rec[t][ST_LEN + i] = 0xff;
    rec[t][ST_UTIME + i] = 0;
    rec[t][ST_COND + i] = 0xff;
    rec[t][ST_B240 + i] = 0xff;
    rec[t][ST_RLEN + i] = 14;
    rec[t][ST_RRATE + i] = 12;
    rec[t][ST_RVEL + i] = 0;
    set_flags(t, i, flags(t, i) & 0x03ff);
}

/* a step left without a trig: reset, and its locks cleared (assumed, as on the Rytm) */
static void cleared(int t, int i)
{
    int k;
    reset(t, i);
    for (k = 0; k < ST_SLOTS; k++)
        if (lock(t, i, k) >= 0)
            st_host_lock_clear(MODEL, t, i, k);
}

void st_host_place_trig(unsigned obj, int i, int on)
{
    int t = T(obj);
    calls++;
    if (i >= len[t] || !on)
        return;
    if (!(flags(t, i) & 1)) {
        reset(t, i);
        set_flags(t, i, (flags(t, i) & ~0x1800) | 0x0281);   /* the track's switches: a note trig */
    }
}

void st_host_set_flag(unsigned obj, int i, int mask, int on)
{
    int t = T(obj);
    calls++;
    if (i >= len[t])
        return;
    if (on && (mask & 1) && !(flags(t, i) & 1))
        reset(t, i);
    set_flags(t, i, on ? flags(t, i) | mask : flags(t, i) & ~mask);
    if (!(flags(t, i) & 1))
        cleared(t, i);
}

void st_host_set_field(unsigned obj, int i, int value, int offset)
{
    calls++;
    if (i < len[T(obj)])
        rec[T(obj)][offset + i] = (uint8_t)value;
}

/* ---- fixtures ---- */

static uint32_t rng = 1;
static int rnd(int n) { rng = rng * 1103515245u + 12345u; return (int)(rng >> 16) % n; }
static unsigned char mem[ST_SEED_STATE_SIZE + 16];

/* a pattern with trigs, fields, chord notes, swing and stock's bits, conditions of every kind, the
 * two unknown bytes, and locks on trig and trigless steps */
static void fixture_n(uint32_t seed)
{
    int t, i, k;
    memset(rec, 0, sizeof rec);
    memset(lks, 0xff, sizeof lks);
    rng = seed;
    for (t = 0; t < ST_TRACKS; t++) {
        len[t] = 16;
        memset(&lks[t][ST_LOCK_IDS], 0, ST_SLOTS);
        rec[t][ST_DEF_NOTE] = 60;
        rec[t][ST_DEF_VEL] = 100;
        for (i = 0; i < SEED_STEPS; i++) {
            int trig = rnd(3) == 0;
            set_flags(t, i, (trig ? 0x0281 | (rnd(4) ? 0 : 0x0800) : 0) | (i & 1 ? SEED_SWING : 0));
            rec[t][ST_NOTE + i] = trig ? (uint8_t)(48 + rnd(24)) : 0xff;
            rec[t][ST_NOT2 + i] = trig && rnd(3) == 0 ? (uint8_t)(64 + rnd(12)) : 0xff;
            rec[t][ST_NOT3 + i] = rec[t][ST_NOT4 + i] = 0xff;
            rec[t][ST_VEL + i] = trig ? (uint8_t)(1 + rnd(127)) : 0xff;
            rec[t][ST_LEN + i] = trig ? (uint8_t)rnd(128) : 0xff;
            rec[t][ST_UTIME + i] = trig ? (uint8_t)(rnd(47) - 23) : 0;
            rec[t][ST_COND + i] = trig && rnd(2) ? (uint8_t)rnd(100) : 0xff;
            rec[t][ST_RLEN + i] = 14;
            rec[t][ST_RRATE + i] = 12;
            rec[t][ST_B240 + i] = trig && rnd(5) == 0 ? (uint8_t)rnd(64) : 0xff;
            rec[t][ST_B280 + i] = (uint8_t)rnd(3);
            for (k = 0; k < ST_SLOTS; k++)
                if (rnd(30) == 0)
                    st_host_lock_set(MODEL, t, i, k, rnd(128) << 8 | rnd(256));
        }
    }
    calls = notices = 0;
}
static void fixture(void) { fixture_n(7); }

static uint8_t rec0[ST_TRACKS][ST_RECORD_SIZE], lks0[ST_TRACKS][ST_LOCKS_SIZE];
static void save(void) { memcpy(rec0, rec, sizeof rec); memcpy(lks0, lks, sizeof lks); }
static int same(void) { return !memcmp(rec0, rec, sizeof rec) && !memcmp(lks0, lks, sizeof lks); }
static int same_track(int t) { return !memcmp(rec0[t], rec[t], sizeof rec[t]) && !memcmp(lks0[t], lks[t], sizeof lks[t]); }

static const struct seed_dev *dev;
static struct seed_track a, b;
static int16_t lanes[SEED_LANES];

/* ---- tests ---- */

static void test_attach(void)
{
    CHECK(!st_seed_attach(mem, ST_SEED_STATE_SIZE - 1));
    CHECK(st_seed_attach(mem, sizeof mem));
    dev = st_seed_device();
    CHECK(dev->caps->tracks == ST_TRACKS && dev->caps->lanes == SEED_LANES);
    CHECK(dev->caps->utime_lo == -23 && dev->caps->utime_hi == 23);
    CHECK(!(dev->caps->fields & (SEED_F_RETRIG | SEED_F_ACCENT | SEED_F_SLIDE)));
}

static void test_detached(void)
{
    struct st_seed_state *s = st_seed_state();
    *st_seed_state_ptr() = 0;
    fixture();
    save();
    CHECK(dev->read(0, lanes, &a) == SEED_E_NA);
    CHECK(dev->write(0, lanes, &a) == SEED_E_NA && same() && calls == 0);
    CHECK(dev->snapshot(1) == SEED_E_NA);
    *st_seed_state_ptr() = s;
}

/* every Syntakt code goes to a seed code and back; every seed code the other way; known pairs */
static void test_cond(void)
{
    int c, ok = 1;
    for (c = -1; c <= 99; c++)
        ok &= st_cond_from_seed(st_cond_to_seed(c)) == c;
    CHECK(ok);
    for (c = 0, ok = 1; c <= 64; c++)
        ok &= st_cond_to_seed(st_cond_from_seed(c)) == c;
    CHECK(ok);
    CHECK(st_cond_from_seed(SEED_NO_COND) == -1 && st_cond_to_seed(-1) == SEED_NO_COND);
    CHECK(st_cond_from_seed(21) == 21 && st_cond_from_seed(28) == 28);     /* 100%, 1ST */
    CHECK(st_cond_from_seed(30) == 32 && st_cond_from_seed(31) == 33);     /* 1:2, 2:2 */
    CHECK(st_cond_from_seed(32) == 34 && st_cond_from_seed(33) == 36);     /* 1:3, 2:3 */
    CHECK(st_cond_from_seed(35) == 40 && st_cond_from_seed(64) == 98);     /* 1:4, 8:8 */
    CHECK(st_cond_to_seed(30) == ST_COND_OWN && st_cond_to_seed(35) == ST_COND_OWN + 2);   /* LST, !1:3 */
    CHECK(st_cond_to_seed(99) == ST_COND_OWN + 34);                        /* !8:8 */
}

/* the track's list: the machine's controls, then the shared pages; the FX track's own */
static void test_lists(void)
{
    const struct st_list *l;
    st_seed_set_machine(0, -1);
    l = st_seed_list(0);
    CHECK(l->n == ST_PAGE_FLTR_N + ST_PAGE_BASEW_N + ST_PAGE_AMP_N + ST_PAGE_LFO_N);
    CHECK(!strcmp(l->p[l->lane[1]].name, "FRQ"));
    st_seed_set_machine(0, 0);                     /* BD MODERN: its controls first */
    l = st_seed_list(0);
    CHECK(l->n == 7 + ST_PAGE_FLTR_N + ST_PAGE_BASEW_N + ST_PAGE_AMP_N + ST_PAGE_LFO_N);
    CHECK(!strcmp(l->p[0].name, "TUNE") && l->lane[0] == 0 && !strcmp(l->p[l->lane[1]].name, "FRQ"));
    dev->current();
    CHECK(dev->caps->nparams == l->n && dev->caps->params == l->p);
    l = st_seed_list(ST_FX_TRACK);
    CHECK(!strcmp(l->p[0].name, "DRV") && !strcmp(l->p[l->lane[1]].name, "FRQ"));
    st_seed_set_machine(0, -1);
}

static void test_read(void)
{
    int i, k;
    fixture();
    rec[2][ST_COND + 3] = 35;                      /* !1:3: the Syntakt's own */
    rec[2][ST_COND + 4] = 34;                      /* 1:3 */
    lanes[0] = 0;
    lanes[1] = 4;
    CHECK(dev->read(2, lanes, &a) == SEED_OK);
    CHECK(a.length == 16 && a.fields == ST_VOICE_FIELDS && a.default_note == 60 && a.default_vel == 100);
    CHECK(a.s[3].cond == ST_COND_OWN + 2 && a.s[4].cond == 32);
    k = st_seed_lane_k(2, lanes, 1);
    CHECK(k >= 0);
    for (i = 0; i < SEED_STEPS; i++) {
        CHECK(a.s[i].flags == flags(2, i) && a.s[i].note == rec[2][ST_NOTE + i] && a.s[i].origin == i);
        CHECK(a.s[i].lane[1] == (lock(2, i, k) < 0 ? SEED_NO_LOCK : lock(2, i, k) >> 8));
    }
    CHECK(dev->read(ST_FX_TRACK, lanes, &a) == SEED_OK && a.fields == ST_FX_FIELDS);
}

/* I1: what was read, written back, calls nothing and changes nothing, on every track and length */
static void test_identity(void)
{
    int t, n;
    for (n = 1; n <= 64; n += 21)
        for (t = 0; t < ST_TRACKS; t++) {
            fixture();
            len[t] = n;
            save();
            CHECK(dev->read(t, lanes, &a) == SEED_OK);
            calls = 0;
            CHECK(dev->write(t, lanes, &a) == SEED_OK && calls == 0 && same() && st_seed_written() == 0);
        }
}

/* the whole step i of t (but its swing) is what step f of the original was */
static int step_is(int t, int i, int f)
{
    int k;
    if ((flags(t, i) & ~SEED_SWING) != ((rec0[t][2 * f] << 8 | rec0[t][2 * f + 1]) & ~SEED_SWING))
        return 0;
    if ((flags(t, i) & SEED_SWING) != ((rec0[t][2 * i] << 8 | rec0[t][2 * i + 1]) & SEED_SWING))
        return 0;
    for (k = 0x080; k < 0x3c0; k += 0x40)
        if (rec[t][k + i] != rec0[t][k + f])
            return 0;
    for (k = 0; k < ST_SLOTS; k++)
        if (lock(t, i, k) != st_lock_at(lks0[t], f, k))
            return 0;
    return 1;
}

/* rotations by origin: every step is its origin's, all of it; other tracks untouched */
static void test_move(void)
{
    int t = 1, r, i, ok;
    for (r = 1; r < 16; r += 5) {
        fixture();
        save();
        CHECK(dev->read(t, lanes, &a) == SEED_OK);
        b = a;
        for (i = 0; i < 16; i++)
            b.s[i] = a.s[(i - r + 16) % 16];
        CHECK(dev->write(t, lanes, &b) == SEED_OK);
        for (i = 0, ok = 1; i < 16; i++)
            ok &= step_is(t, i, (i - r + 16) % 16);
        CHECK(ok);
        CHECK(same_track(0) && same_track(2) && same_track(ST_FX_TRACK));
        CHECK(!memcmp(&rec[t][2 * 16], &rec0[t][2 * 16], 2 * 48));       /* past the length */
    }
}

/* lanes: a value is a lock of its whole part; a value equal to the old whole part keeps the fraction;
 * NO_LOCK clears; a step with no origin keeps the position's other locks */
static void test_lanes(void)
{
    int k, i;
    fixture();
    lanes[0] = 4;                                  /* FRQ (no machine: the filter page first) */
    lanes[1] = 1;
    k = st_seed_lane_k(3, lanes, 0);
    CHECK(k >= 0);
    st_host_lock_set(MODEL, 3, 2, k, 0x40de);
    save();
    CHECK(dev->read(3, lanes, &a) == SEED_OK && a.s[2].lane[0] == 0x40);
    b = a;
    b.s[0].lane[0] = 100;
    b.s[1].lane[0] = SEED_NO_LOCK;
    b.s[3].lane[0] = 5;
    b.s[3].origin = SEED_NO_ORIGIN;
    CHECK(dev->write(3, lanes, &b) == SEED_OK);
    CHECK(lock(3, 0, k) == 100 << 8 && lock(3, 1, k) < 0 && lock(3, 2, k) == 0x40de && lock(3, 3, k) == 5 << 8);
    for (i = 0; i < ST_SLOTS; i++)
        if (i != k && i != st_seed_lane_k(3, lanes, 1))
            CHECK(lock(3, 3, i) == st_lock_at(lks0[3], 3, i));
    CHECK(st_lock_at(lks[3], 0, 71) >= 1 && lks[3][ST_LOCK_IDS + k] == 1);
}

/* I8: a setting the device cannot take is refused before anything is written */
static void test_room(void)
{
    fixture();
    lanes[0] = 4;
    lanes[1] = 1;
    CHECK(dev->read(3, lanes, &a) == SEED_OK);
    save();
    b = a;
    b.s[5].lane[0] = 200;                          /* past FRQ's 0..127 */
    CHECK(dev->write(3, lanes, &b) == SEED_E_ARG && same());
    b = a;
    b.s[2].origin = 40;                            /* an origin past the length */
    CHECK(dev->room(3, lanes, &b) == SEED_E_ARG && dev->write(3, lanes, &b) == SEED_E_ARG && same());
    lanes[1] = 4;                                  /* two lanes on one parameter */
    CHECK(dev->write(3, lanes, &a) == SEED_E_ARG && same());
    lanes[1] = 99;                                 /* past the list */
    CHECK(dev->write(3, lanes, &a) == SEED_E_ARG && same());
    lanes[1] = 1;
    CHECK(dev->room(3, lanes, &a) == SEED_OK);
}

/* a generated trig on an empty step gets the switches stock's place sets; one moved keeps its own */
static void test_new_trig(void)
{
    int i;
    fixture();
    for (i = 0; i < 16 && (flags(4, i) & 1); i++)
        ;
    CHECK(i < 16);
    CHECK(dev->read(4, lanes, &a) == SEED_OK);
    b = a;
    b.s[i].flags = SEED_TRIG | (b.s[i].flags & SEED_SWING);
    b.s[i].origin = SEED_NO_ORIGIN;
    b.s[i].note = 62;
    CHECK(dev->write(4, lanes, &b) == SEED_OK);
    CHECK((flags(4, i) & 0xffef) == 0x0281 && rec[4][ST_NOTE + i] == 62);
}

/* I7: snapshot, takes written from it, restore: byte-identical; the snapshot is the source */
static void test_snapshot(void)
{
    int i, r;
    fixture();
    save();
    CHECK(dev->snapshot(1u << 5) == SEED_OK);
    CHECK(dev->read(5, lanes, &a) == SEED_OK);
    for (r = 1; r <= 3; r++) {
        b = a;
        for (i = 0; i < 16; i++) {
            b.s[i] = a.s[(i + r) % 16];
            b.s[i].lane[0] = (int16_t)rnd(128);
        }
        b.s[7].flags = 0;
        CHECK(dev->write(5, lanes, &b) == SEED_OK);
        CHECK(!same_track(5));
    }
    dev->restore();
    CHECK(same());
    dev->drop();
    dev->restore();
    CHECK(same());
}

static void test_context(void)
{
    struct seed_ctx c;
    int i;
    fixture();
    len[6] = 13;
    rec[6][ST_EUCLID] = 1;
    for (i = 0; i < SEED_STEPS; i++)
        rec[6][ST_EUCLID_TAB + i] = (uint8_t)(i % 3 == 0 ? i / 3 : 0xff);
    dev->context(&c, 6);
    CHECK(c.tracks == ST_TRACKS && c.track == 6 && c.length[6] == 13 && c.caps == dev->caps);
    for (i = 0; i < 13; i++)
        CHECK(seed_ctx_trig(&c, 6, i) == (i % 3 == 0));
    for (i = 0; i < 16; i++)
        CHECK(seed_ctx_trig(&c, 0, i) == (flags(0, i) & 1));
}

/* Fuzzing: random patterns, lengths, lanes and takes. A refused take changes nothing; a written one
 * leaves other tracks and the steps past the length alone, lands each step's fields and lanes, and a
 * restore after it is byte-identical. */
static void random_take(struct seed_track *t, int n)
{
    int i, j;
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        if (rnd(3) == 0)
            s->origin = rnd(8) ? (int8_t)rnd(n) : SEED_NO_ORIGIN;
        if (rnd(4) == 0)
            s->flags = (uint16_t)((s->flags & ~SEED_TRIG) | rnd(2));
        if (rnd(4) == 0)
            s->note = rnd(5) ? (uint8_t)(36 + rnd(48)) : SEED_DEFAULT;
        if (rnd(4) == 0)
            s->vel = (uint8_t)(1 + rnd(127));
        if (rnd(4) == 0)
            s->utime = (int8_t)(rnd(47) - 23);
        if (rnd(4) == 0)
            s->cond = rnd(4) ? (int8_t)rnd(65) : SEED_NO_COND;
        for (j = 0; j < SEED_LANES; j++)
            if (rnd(3) == 0)
                s->lane[j] = rnd(4) ? (int16_t)rnd(128) : SEED_NO_LOCK;
    }
    if (rnd(20) == 0)
        t->s[rnd(n)].lane[0] = 300;
}

static void test_fuzz(int cases)
{
    int c, t, i, j, n, ok;
    for (c = 0; c < cases; c++) {
        fixture_n(1000 + (uint32_t)c);
        t = rnd(ST_TRACKS);
        n = 1 + rnd(64);
        len[t] = n;
        lanes[0] = (int16_t)rnd(10);
        lanes[1] = (int16_t)(rnd(10) + 10);
        save();
        CHECK(dev->snapshot((uint16_t)(1u << t)) == SEED_OK);
        CHECK(dev->read(t, lanes, &a) == SEED_OK);
        b = a;
        random_take(&b, n);
        if (dev->write(t, lanes, &b) != SEED_OK) {
            CHECK(same());
            continue;
        }
        for (j = 0, ok = 1; j < ST_TRACKS; j++)
            if (j != t)
                ok &= same_track(j);
        CHECK(ok);
        ok = !memcmp(&rec[t][2 * n], &rec0[t][2 * n], 2 * (64 - n));
        for (i = n; i < SEED_STEPS; i++)
            for (j = 0; j < ST_SLOTS; j++)
                ok &= lock(t, i, j) == st_lock_at(lks0[t], i, j);
        CHECK(ok);
        CHECK(dev->read(t, lanes, &a) == SEED_OK);
        for (i = 0, ok = 1; i < n; i++) {
            const struct seed_step *w = &b.s[i], *g = &a.s[i];
            ok &= (g->flags & SEED_TRIG) == (w->flags & SEED_TRIG);
            ok &= g->note == w->note && g->vel == w->vel && g->utime == w->utime && g->cond == w->cond;
            if (st_seed_fields(t) & SEED_F_LOCKS)
                for (j = 0; j < SEED_LANES; j++)
                    ok &= g->lane[j] == w->lane[j];
        }
        CHECK(ok);
        dev->restore();
        CHECK(same());
        dev->drop();
    }
}

/* ROTATE ALL (st_seed_rotate): step i of the first len goes to (i + k) mod len with every field and its 72 lock
 * slots; the swing bit, the steps past the length, the track's defaults and the byte per slot stay; a track
 * in stock's Euclidean mode is skipped; other tracks do not change */
static void test_rotate(void)
{
    int c, t, i, j, k, steps, ok;
    for (c = 0; c < 400; c++) {
        fixture_n(500u + (uint32_t)c);
        t = rnd(ST_TRACKS);
        len[t] = 1 + rnd(64);
        steps = rnd(201) - 100;
        k = ((steps % len[t]) + len[t]) % len[t];
        save();
        CHECK(st_seed_rotate(t, steps) == 1);
        for (j = 0, ok = 1; j < ST_TRACKS; j++)
            ok &= j == t || same_track(j);
        CHECK(ok);
        for (i = 0, ok = 1; i < SEED_STEPS; i++) {
            int d = i < len[t] ? (i + k) % len[t] : i, a;
            ok &= (flags(t, d) & ~SEED_SWING) == ((rec0[t][2 * i] << 8 | rec0[t][2 * i + 1]) & ~SEED_SWING);
            ok &= (flags(t, i) & SEED_SWING) == (rec0[t][2 * i + 1] & SEED_SWING);
            for (a = ST_VEL; a <= ST_NOT4; a += SEED_STEPS)
                ok &= rec[t][a + d] == rec0[t][a + i];
            ok &= !memcmp(&lks[t][2 * ST_STEP_LOCKS * d], &lks0[t][2 * ST_STEP_LOCKS * i], 2 * ST_STEP_LOCKS);
        }
        CHECK(ok);
        CHECK(!memcmp(&rec[t][0x3c0], &rec0[t][0x3c0], ST_RECORD_SIZE - 0x3c0));
        CHECK(!memcmp(&lks[t][ST_LOCK_IDS], &lks0[t][ST_LOCK_IDS], ST_LOCKS_SIZE - ST_LOCK_IDS));
    }
    fixture();
    rec[3][ST_EUCLID] = 1;
    save();
    CHECK(st_seed_rotate(3, 5) == 0 && same());
}

/* the playhead (task T0b): -1 while stopped or for a track out of range; the step played last while running */
static void test_playhead(void)
{
    int t;
    st_host_seq_running = 0;
    for (t = 0; t < ST_TRACKS; t++)
        st_host_seq_last[t] = (uint8_t)(t * 3);
    CHECK(st_playhead(2) == -1);
    st_host_seq_running = 1;
    for (t = 0, checks++; t < ST_TRACKS; t++)
        if (st_playhead(t) != t * 3) { fails++; printf("FAIL playhead %d\n", t); break; }
    CHECK(st_playhead(-1) == -1 && st_playhead(ST_TRACKS) == -1);
    st_host_seq_last[4] = 64;
    CHECK(st_playhead(4) == -1);
    st_host_seq_running = 0;
}

/* the steps passed between two reads, as a mask */
static void test_steps_passed(void)
{
    uint8_t m[SEED_STEPS / 8];
    int len, from, to, i, n, ok;
    CHECK(st_steps_passed(3, 3, 16, m) == 0);
    CHECK(st_steps_passed(-1, 5, 16, m) == 1 && m[0] == 0x20 && m[1] == 0);
    CHECK(st_steps_passed(14, 1, 16, m) == 3 && m[0] == 0x03 && m[1] == 0x80);
    CHECK(st_steps_passed(2, 20, 16, m) == 0);
    for (len = 1, ok = 1; len <= SEED_STEPS; len++)
        for (from = -1; from < len; from++)
            for (to = 0; to < len; to++) {
                int want = from < 0 ? 1 : from == to ? 0 : (to - from + len) % len;
                n = st_steps_passed(from, to, len, m);
                ok &= n == want;
                for (i = 0; i < SEED_STEPS; i++) {
                    int in = from < 0 ? i == to : from != to && (i - from + len) % len >= 1 &&
                             (i - from + len) % len <= (to - from + len) % len && i < len;
                    ok &= ((m[i >> 3] >> (i & 7)) & 1) == in;
                }
            }
    CHECK(ok);
}

/* a masked write changes only the steps it names, and each as the whole write would; restore while it runs is
 * byte-identical (TURING's live page writes behind the playhead) */
static void test_write_steps(int cases)
{
    uint8_t m[SEED_STEPS / 8], recw[ST_TRACKS][ST_RECORD_SIZE], lksw[ST_TRACKS][ST_LOCKS_SIZE];
    int c, t, n, i, k, ok;
    for (c = 0; c < cases; c++) {
        fixture_n(5000 + (uint32_t)c);
        t = rnd(ST_TRACKS);
        n = 1 + rnd(64);
        len[t] = n;
        lanes[0] = (int16_t)rnd(10);
        lanes[1] = (int16_t)(rnd(10) + 10);
        save();
        CHECK(dev->snapshot((uint16_t)(1u << t)) == SEED_OK);
        CHECK(dev->read(t, lanes, &a) == SEED_OK);
        b = a;
        random_take(&b, n);
        /* the whole write, kept as the reference, then back to the original */
        if (dev->write(t, lanes, &b) != SEED_OK) {
            dev->drop();
            continue;
        }
        memcpy(recw, rec, sizeof rec);
        memcpy(lksw, lks, sizeof lks);
        dev->restore();
        CHECK(same());
        /* the masked writes, a few steps at a time in playhead order, then the whole track is the reference */
        for (i = 0; i < SEED_STEPS / 8; i++)
            m[i] = 0;
        for (i = 0, ok = 1; i < n; i += k) {
            k = 1 + rnd(3);
            if (n > 1 && k >= n)
                k = n - 1;   /* a whole loop between two reads cannot be told from no move */
            /* from the step before, in loop order (step n - 1 comes before step 0; one step alone: -1) */
            st_steps_passed(i > 0 ? i - 1 : n > 1 ? n - 1 : -1, i + k - 1 < n ? i + k - 1 : n - 1, n, m);
            if (st_seed_write_steps(t, lanes, &b, m) != SEED_OK) { ok = 0; break; }
        }
        CHECK(ok);
        CHECK(!memcmp(recw[t], rec[t], sizeof rec[t]) && !memcmp(lksw[t], lks[t], sizeof lks[t]));
        for (i = 0, ok = 1; i < ST_TRACKS; i++)
            if (i != t)
                ok &= same_track(i);
        CHECK(ok);
        dev->restore();
        CHECK(same());
        dev->drop();
    }
}

int main(void)
{
    int cases = getenv("DEV_FUZZ") ? atoi(getenv("DEV_FUZZ")) : 2000;
    test_attach();
    test_detached();
    test_cond();
    test_lists();
    test_read();
    test_identity();
    test_move();
    test_lanes();
    test_room();
    test_new_trig();
    test_snapshot();
    test_context();
    test_rotate();
    test_playhead();
    test_steps_passed();
    test_write_steps(500);
    test_fuzz(cases);
    printf("  test_syntakt_st: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
