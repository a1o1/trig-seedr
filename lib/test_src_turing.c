/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for src_turing.c (0.9, docs/design-turing.md): each flavour's rule (§2), T2 (the locking
 * ends repeat the loop exactly), STRT = TRACK starting from the original, DJVU's three AMT zones,
 * CRPT's two stages (no bit flips up to AMT 50), SHFT at 0 doubling the loop by inversion, TURG
 * values, the live step function against the offline run, and refusals. */
#include <string.h>
#include "tests.h"
#include "props.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;
static struct seed_tur tur;

static void base(int flav, int loop, int amt, int len)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_TURING;
    r.tur_flav = (int16_t)flav;
    r.tur_loop = (int16_t)loop;
    r.tur_amt = (int16_t)amt;
}

static int run(uint32_t seed)
{
    struct seed_take k;
    seed_take_init(&k, seed, 0, 0, 0);
    return seed_turing_run(&tur, &orig, &r, &k);
}

/* steps i >= L that repeat step i - L (bit and value), out of len - L */
static int repeats(int L, int len, int *bits_same)
{
    int i, same = 0;
    *bits_same = 0;
    for (i = L; i < len; i++) {
        same += tur.bit[i] == tur.bit[i - L] && tur.val[i] == tur.val[i - L];
        *bits_same += tur.bit[i] == tur.bit[i - L];
    }
    return same;
}

/* T2: the locking ends give the loop again and again, in the run and in the rendered trigs */
static void test_locked(void)
{
    static const int lock[SEED_TURF_N] = { 50, 0, 100 };
    int f, L, s, i, bs;
    for (f = 0; f < SEED_TURF_N; f++)
        for (L = 1; L <= SEED_TUR_LOOP; L++)
            for (s = 1; s <= 5; s++) {
                int eff = f == SEED_TURF_SHFT && L < 2 ? 2 : L;
                base(f, L, lock[f], 64);
                CHECK(run((uint32_t)s) == SEED_OK);
                CHECK(repeats(eff, 64, &bs) == 64 - eff);
                CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
                for (i = eff; i < 64; i++)
                    CHECK((out.s[i].flags & SEED_TRIG) == (out.s[i - eff].flags & SEED_TRIG));
            }
}

/* STRT = TRACK: the first pass is the original's first LOOP steps */
static void test_strt_track(void)
{
    int L, i, s;
    for (s = 1; s <= 20; s++)
        for (L = 1; L <= SEED_TUR_LOOP; L++) {
            base(SEED_TURF_CRPT, L, 0, 64);
            r.tur_strt = SEED_TURS_TRACK;
            CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
            for (i = 0; i < 64; i++)
                CHECK(!(out.s[i].flags & SEED_TRIG) == !(orig.s[i % L].flags & SEED_TRIG));
        }
}

/* DJVU: below 50 a share (2 AMT - 1)^2 of the steps play a new cell; at 50 none; above 50 the
 * values are the first pass's, played in another order */
static void test_djvu(void)
{
    static const int amts[] = { 0, 25, 40 };
    int a, s, i, bs, total, fresh, L = 8;
    for (a = 0; a < 3; a++) {
        total = fresh = 0;
        for (s = 1; s <= 200; s++) {
            base(SEED_TURF_DJVU, L, amts[a], 64);
            r.tur_bias = 50;
            CHECK(run((uint32_t)s) == SEED_OK);
            fresh += 64 - L - repeats(L, 64, &bs);
            total += 64 - L;
        }
        {
            int x = 2 * amts[a] - 100, want = x * x / 100;        /* % */
            int got = fresh * 100 / total;
            /* a step differs when its own cell or the one a loop ago was replaced */
            int lo = want - 8, hi = 100 - (100 - want) * (100 - want) / 100 + 8;
            if (amts[a] == 0)
                lo = 95;
            CHECK(got >= lo && got <= hi);
        }
    }
    /* above 50: no new values, and jumps out of order */
    for (s = 1; s <= 50; s++) {
        struct seed_turing m;
        struct seed_take k;
        int jumped = 0;
        base(SEED_TURF_DJVU, L, 100, 64);
        CHECK(run((uint32_t)s) == SEED_OK);
        seed_take_init(&k, (uint32_t)s, 0, 0, 0);
        CHECK(seed_turing_init(&m, &r, &orig, &k) == SEED_OK);   /* the cells before any step */
        for (i = 0; i < 64; i++) {
            int j, in = 0;
            for (j = 0; j < L; j++)
                in |= tur.val[i] == m.val[j];
            CHECK(in);
        }
        for (i = L; i < 64; i++)
            jumped += tur.val[i] != tur.val[i - L];
        CHECK(jumped > 0);
    }
}

/* CRPT: up to 50 the trigs never change; at 100 about half the bits flip each pass */
static void test_crpt(void)
{
    int a, s, bs, flips = 0, total = 0, L = 8;
    for (a = 0; a <= 50; a += 5)
        for (s = 1; s <= 20; s++) {
            base(SEED_TURF_CRPT, L, a, 64);
            CHECK(run((uint32_t)s) == SEED_OK);
            repeats(L, 64, &bs);
            CHECK(bs == 64 - L);
        }
    for (s = 1; s <= 200; s++) {
        base(SEED_TURF_CRPT, L, 100, 64);
        CHECK(run((uint32_t)s) == SEED_OK);
        repeats(L, 64, &bs);
        flips += 64 - L - bs;
        total += 64 - L;
    }
    CHECK(flips * 100 / total >= 40 && flips * 100 / total <= 60);
    /* at 50 about a quarter of the on cells change value each pass, and none at 0 */
    {
        int on = 0, changed = 0, i;
        for (s = 1; s <= 200; s++) {
            base(SEED_TURF_CRPT, L, 50, 64);
            CHECK(run((uint32_t)s) == SEED_OK);
            for (i = L; i < 64; i++)
                if (tur.bit[i - L]) {
                    on++;
                    changed += tur.val[i] != tur.val[i - L];
                }
        }
        CHECK(changed * 100 / on >= 18 && changed * 100 / on <= 32);
    }
}

/* SHFT: AMT 0 inverts every bit as it comes round, so the loop is twice as long; the value is the
 * eight newest bits as a byte */
static void test_shft(void)
{
    int s, L, i;
    for (s = 1; s <= 20; s++)
        for (L = 2; L <= SEED_TUR_LOOP; L++) {
            base(SEED_TURF_SHFT, L, 0, 64);
            CHECK(run((uint32_t)s) == SEED_OK);
            for (i = L; i < 64; i++)
                CHECK(tur.bit[i] == !tur.bit[i - L]);
            for (i = 2 * L; i < 64; i++)
                CHECK(tur.bit[i] == tur.bit[i - 2 * L]);
            CHECK(tur.val[0] % 257 == 0);
        }
    /* LOOP 1 is played as 2 */
    base(SEED_TURF_SHFT, 1, 100, 64);
    CHECK(run(3) == SEED_OK);
    for (i = 2; i < 64; i++)
        CHECK(tur.bit[i] == tur.bit[i - 2]);
}

/* the live page's step function plays what the offline run plays */
static void test_step(void)
{
    struct seed_turing m;
    struct seed_take k;
    int f, s, i;
    for (f = 0; f < SEED_TURF_N; f++)
        for (s = 1; s <= 10; s++) {
            base(f, 1 + s % 16, (s * 37) % 101, 64);
            CHECK(run((uint32_t)s) == SEED_OK);
            seed_take_init(&k, (uint32_t)s, 0, 0, 0);
            CHECK(seed_turing_init(&m, &r, &orig, &k) == SEED_OK);
            for (i = 0; i < 64; i++) {
                uint16_t v;
                int b = seed_turing_step(&m, &k, i, &v);
                CHECK(b == tur.bit[i] && v == tur.val[i]);
            }
        }
}

/* TURG: with STRT = TRACK and a locked loop, NOTE -24..+24 from TURG gives back the original's notes */
static void test_turg(void)
{
    int s, i;
    for (s = 1; s <= 20; s++) {
        base(SEED_TURF_CRPT, 16, 0, 16);
        r.tur_strt = SEED_TURS_TRACK;
        r.note.lo = -24;
        r.note.hi = 24;
        r.note.src = SEED_VSRC_TURG;
        CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
        for (i = 0; i < 16; i++) {
            const struct seed_step *o = &orig.s[i];
            int off = (int)o->note - (int)orig.default_note;
            if (!(o->flags & SEED_TRIG) || o->note == SEED_DEFAULT || off < -24 || off > 24)
                continue;
            CHECK(out.s[i].note == o->note);
        }
    }
    /* a value source TURG with another SOURCE still has a machine to read */
    base(SEED_TURF_DJVU, 8, 30, 32);
    r.src = SEED_SRC_RANDOM;
    r.vel.lo = 1;
    r.vel.hi = 127;
    r.vel.src = SEED_VSRC_TURG;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
}

/* The live page (T3): its start is the offline run; rendering a given run is rendering the recipe;
 * a live step changes only its step; a locked loop changes nothing; replaying the same steps from
 * the same start gives the same run */
static void test_live(void)
{
    static const int lock[SEED_TURF_N] = { 50, 0, 100 };
    struct seed_turing m, m2;
    struct seed_take k;
    struct seed_tur live, live2;
    struct seed_track a, b;
    uint32_t count, count2;
    int f, s, i, j;
    for (f = 0; f < SEED_TURF_N; f++)
        for (s = 1; s <= 8; s++) {
            base(f, 2 + s % 15, (s * 29) % 101, 32);
            r.seed = (uint32_t)s;
            CHECK(run((uint32_t)s) == SEED_OK);
            seed_take_init(&k, (uint32_t)s, 0, 0, 0);
            k.length = 32;
            CHECK(seed_turing_live_start(&m, &live, &orig, &r, &k, &count) == SEED_OK);
            CHECK(count == 32);
            for (i = 0; i < SEED_STEPS; i++)
                CHECK(live.bit[i] == tur.bit[i] && live.val[i] == tur.val[i]);
            /* the render of the run is the render of the recipe */
            CHECK(seed_render(&a, &orig, &r, &c, &work) == SEED_OK);
            CHECK(seed_render_tur(&b, &orig, &r, &c, &work, &live) == SEED_OK);
            CHECK(memcmp(&a, &b, sizeof a) == 0);
            /* steps played in playhead order from step 5, three loops and more */
            live2 = live;
            m2 = m;
            count2 = count;
            for (j = 0; j < 100; j++) {
                struct seed_tur before = live;
                int st = (5 + j) % 32, x;
                seed_turing_live_step(&m, &live, &k, &count, st);
                for (x = 0; x < SEED_STEPS; x++)
                    if (x != st)
                        CHECK(live.bit[x] == before.bit[x] && live.val[x] == before.val[x]);
            }
            CHECK(count == 132);
            for (j = 0; j < 100; j++)
                seed_turing_live_step(&m2, &live2, &k, &count2, (5 + j) % 32);
            CHECK(memcmp(&live, &live2, sizeof live) == 0);
        }
    /* a locked loop: every live step gives what the step held a loop earlier */
    for (f = 0; f < SEED_TURF_N; f++) {
        base(f, 8, lock[f], 32);
        seed_take_init(&k, 3, 0, 0, 0);
        k.length = 32;
        CHECK(seed_turing_live_start(&m, &live, &orig, &r, &k, &count) == SEED_OK);
        live2 = live;
        for (j = 0; j < 64; j++)
            seed_turing_live_step(&m, &live, &k, &count, j % 32);
        for (i = 0; i < 32; i++)
            CHECK(live.bit[i] == live2.bit[i] && live.val[i] == live2.val[i]);
    }
}

/* the seed matters; the same seed gives the same run */
static void test_seeds(void)
{
    struct seed_tur a;
    int f, differ = 0;
    for (f = 0; f < SEED_TURF_N; f++) {
        base(f, 8, 30, 64);
        CHECK(run(1) == SEED_OK);
        a = tur;
        CHECK(run(1) == SEED_OK);
        CHECK(!memcmp(&a, &tur, sizeof a));
        CHECK(run(2) == SEED_OK);
        differ += memcmp(&a, &tur, sizeof a) != 0;
    }
    CHECK(differ == SEED_TURF_N);
}

static void test_refused(void)
{
    base(SEED_TURF_DJVU, 0, 25, 16);
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    base(SEED_TURF_DJVU, 17, 25, 16);
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    base(SEED_TURF_N, 8, 25, 16);
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    base(SEED_TURF_CRPT, 8, 101, 16);
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    /* knobs out of range are ignored while nothing uses TURING */
    base(SEED_TURF_CRPT, 99, 101, 16);
    r.src = SEED_SRC_TRACK;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_locked();
    test_strt_track();
    test_djvu();
    test_crpt();
    test_shft();
    test_step();
    test_live();
    test_turg();
    test_seeds();
    test_refused();
    printf("test_src_turing: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
