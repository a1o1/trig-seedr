/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for timing_drift.c (F8): DRFT. Its bound (DRFT % of the device's range, inside the
 * range), that it is correlated (neighbouring steps, round the loop, closer than RAND's), raising
 * DRFT scaling the same curve, its own numbers (RAND's unchanged, the same drift for a step whatever
 * the trigs, P1), VARY (P3, I10), the fields and steps it leaves, and I2, I3, I6 with DRFT on
 * (props.h). */
#include "tests.h"
#include "props.h"

static struct seed_track orig, out, ref;
static struct seed_recipe r;
static struct seed_ctx c;

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

/* every step a trig at micro timing 0 (RANDOM, DENS 100), so utime is the drift itself */
static void every_step(int len, int drft, uint32_t seed)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_RANDOM;
    r.dens = 100;
    r.drft = (int16_t)drft;
    r.seed = seed;
}

static int iabs(int v)
{
    return v < 0 ? -v : v;
}

static void test_bound(void)
{
    static const int lens[5] = { 3, 16, 17, 32, 64 };
    int L, i, ok = 1, lo = 99, hi = -99;
    uint32_t seed;
    struct seed_caps narrow = test_caps;
    for (L = 0; L < 5; L++) {
        int drft;
        for (drft = 10; drft <= 100; drft += 30) {
            int m = 23 * drft / 100, top = 0;
            for (seed = 0; seed < 100; seed++) {
                every_step(lens[L], drft, seed);
                ok &= rendered();
                for (i = 0; i < lens[L]; i++) {
                    ok &= iabs(out.s[i].utime) <= m;
                    if (iabs(out.s[i].utime) > top)
                        top = iabs(out.s[i].utime);
                }
            }
            ok &= top == m;               /* the bound is reached */
        }
    }
    CHECK(ok);
    /* the device's range */
    narrow.utime_lo = -6;
    narrow.utime_hi = 10;
    c.caps = &narrow;
    for (seed = 0; seed < 100; seed++) {
        every_step(32, 100, seed);
        CHECK(rendered());
        for (i = 0; i < 32; i++) {
            if (out.s[i].utime < lo) lo = out.s[i].utime;
            if (out.s[i].utime > hi) hi = out.s[i].utime;
        }
    }
    CHECK(lo == -6 && hi == 10);
    c.caps = &test_caps;
    /* SOURCE = TRACK: adds to the micro timing there, stays inside the range */
    random_track(&orig, 32);
    seed_recipe_defaults(&r, &test_caps);
    r.drft = 100;
    for (r.seed = 0, ok = 1; r.seed < 50; r.seed++) {
        ok &= rendered();
        for (i = 0; i < 32; i++) {
            int d = out.s[i].utime - orig.s[i].utime;
            ok &= orig.s[i].flags & SEED_TRIG ? iabs(d) <= 23 && iabs(out.s[i].utime) <= 23 : d == 0;
        }
    }
    CHECK(ok);
}

/* Correlation: over many takes, the mean |difference| between neighbouring steps (round the loop)
 * against the mean |drift|; for RAND (independent) the first is larger than the second. */
static void test_correlated(void)
{
    static const int lens[3] = { 16, 24, 64 };
    int L;
    for (L = 0; L < 3; L++) {
        int n = lens[L], i, rand;
        for (rand = 0; rand < 2; rand++) {
            long mag = 0, step = 0, wrap = 0;
            uint32_t seed;
            for (seed = 0; seed < 400; seed++) {
                every_step(n, rand ? 0 : 100, seed);
                r.rand = (int16_t)(rand ? 100 : 0);
                CHECK(rendered());
                for (i = 0; i < n; i++) {
                    mag += iabs(out.s[i].utime);
                    if (i + 1 < n)
                        step += iabs(out.s[i + 1].utime - out.s[i].utime);
                }
                wrap += iabs(out.s[0].utime - out.s[n - 1].utime);
            }
            mag /= n;
            step /= n - 1;
            printf("  %s, %d steps: mean |offset| %ld, mean |neighbour difference| %ld, at the loop %ld (means over 400 takes)\n",
                   rand ? "RAND 100" : "DRFT 100", n, mag / 400, step / 400, wrap / 400);
            if (rand)
                CHECK(step > mag);
            else {
                CHECK(step * 3 < mag * 2);    /* neighbours are close (RAND: more than the offset) */
                CHECK(wrap * 3 < mag * 2);    /* also round the loop: no jump */
                CHECK(wrap < step * 2);
            }
        }
    }
}

/* raising DRFT scales the same curve: the sign stays, the size does not shrink */
static void test_scales(void)
{
    int i, ok = 1, drft;
    uint32_t seed;
    for (seed = 0; seed < 100; seed++) {
        every_step(32, 1, seed);
        ok &= rendered();
        ref = out;
        for (drft = 2; drft <= 100; drft++) {
            r.drft = (int16_t)drft;
            ok &= rendered();
            for (i = 0; i < 32; i++) {
                ok &= iabs(out.s[i].utime) >= iabs(ref.s[i].utime);
                ok &= !(out.s[i].utime < 0 && ref.s[i].utime > 0) && !(out.s[i].utime > 0 && ref.s[i].utime < 0);
            }
            ref = out;
        }
    }
    CHECK(ok);
}

/* P1: DRFT has its own numbers. With RAND as well, the result is RAND's plus DRFT's (when nothing
 * reaches the range's ends); RAND's offsets and the drift are unrelated (the same step, and RAND's
 * step j against the drift on step 2j, where the finest octave's cell j sits); a step's drift does
 * not depend on which steps have trigs. */
static void test_own_numbers(void)
{
    static struct seed_track d, a;
    int i, ok = 1, tried = 0, both = 0;
    long long sxy[2] = { 0, 0 }, sxx[2] = { 0, 0 }, syy[2] = { 0, 0 };
    uint32_t seed;
    for (seed = 0; seed < 200; seed++) {
        every_step(32, 40, seed);
        ok &= rendered();
        d = out;
        r.drft = 0;
        r.rand = 40;
        ok &= rendered();
        a = out;
        r.drft = 40;
        ok &= rendered();
        for (i = 0; i < 32; i++)
            if (iabs(a.s[i].utime + d.s[i].utime) < 23) {
                tried++;
                ok &= out.s[i].utime == a.s[i].utime + d.s[i].utime;
            }
        for (i = 0; i < 32; i++) {
            int x = a.s[i].utime, y = d.s[i].utime, y2 = d.s[(2 * i) % 32].utime;
            sxy[0] += x * y; sxx[0] += x * x; syy[0] += y * y;
            sxy[1] += x * y2; sxx[1] += x * x; syy[1] += y2 * y2;
        }
        /* DENS 50: the trigs it keeps have DENS 100's drift */
        r.rand = 0;
        r.dens = 50;
        ok &= rendered();
        for (i = 0; i < 32; i++)
            if (out.s[i].flags & SEED_TRIG) {
                both++;
                ok &= out.s[i].utime == d.s[i].utime;
            }
    }
    CHECK(ok && tried > 5000 && both > 2000);
    for (i = 0; i < 2; i++)                    /* |correlation| < 0.1 */
        CHECK(sxy[i] * sxy[i] * 100 < sxx[i] * syy[i]);
}

/* P3 and I10: VARY 100 gives every take the same drift, VARY 0 unrelated takes, VARY 50 between.
 * A step's drift comes from up to 10 cells, so VARY 50 re-rolls some cell of nearly every step:
 * the measure is the size of the change, not the count of steps changed. */
static void test_vary(void)
{
    static struct seed_track prev;
    int vary, i;
    long changed[3] = { 0, 0, 0 };
    for (vary = 0; vary <= 2; vary++) {
        uint32_t seed;
        for (seed = 0; seed < 200; seed++) {
            every_step(32, 100, seed);
            r.vary = (int16_t)(vary * 50);
            CHECK(rendered());
            if (seed)
                for (i = 0; i < 32; i++)
                    changed[vary] += iabs(out.s[i].utime - prev.s[i].utime);
            prev = out;
        }
    }
    printf("  VARY 0 / 50 / 100: mean |change| between consecutive takes %ld / %ld / %ld (x100)\n",
           changed[0] * 100 / (199 * 32), changed[1] * 100 / (199 * 32), changed[2] * 100 / (199 * 32));
    CHECK(changed[2] == 0);
    CHECK(changed[0] > 199 * 32 * 5);
    CHECK(changed[1] > changed[0] / 4 && changed[1] < changed[0] * 4 / 5);
}

/* what DRFT leaves: other fields, steps without a trig, steps past the length, a track without
 * micro timing, tracks of 1 and 2 steps; DRFT 0 does not run */
static void test_leaves(void)
{
    int i, ok = 1, len;
    uint32_t seed;
    for (seed = 0; seed < 50; seed++) {
        random_track(&orig, 24);
        seed_recipe_defaults(&r, &test_caps);
        r.drft = 70;
        r.seed = seed;
        ok &= rendered();
        ok &= (prop_diff(&out, &orig, 0) & ~PROP_UTIME) == 0;
        for (i = 0; i < SEED_STEPS; i++)
            if (i >= 24 || !(orig.s[i].flags & SEED_TRIG))
                ok &= out.s[i].utime == orig.s[i].utime;
        orig.fields &= (uint16_t)~SEED_F_UTIME;
        ok &= rendered() && !memcmp(&out, &orig, sizeof out);
    }
    CHECK(ok);
    for (len = 1, ok = 1; len <= 2; len++) {
        random_track(&orig, len);
        seed_recipe_defaults(&r, &test_caps);
        r.drft = 100;
        ok &= rendered() && !memcmp(&out, &orig, sizeof out);
    }
    CHECK(ok);
    CHECK(seed_layer_active(&r, SEED_L_TIME));
    r.drft = 0;
    CHECK(!seed_layer_active(&r, SEED_L_TIME));
}

/* I2, I6 and I3 with DRFT on; changing DRFT alone changes only micro timing */
static void test_props(void)
{
    static struct seed_track a;
    struct prop_rng g = { 0xd81f7u };
    struct seed_recipe s;
    int round, ok2 = 1, ok3 = 1, okd = 1, L;
    unsigned changed;
    for (round = 0; round < 1000; round++) {
        int len = prop_int(&g, 1, 64);
        prop_track(&orig, &g, len, prop_int(&g, 0, 100));
        prop_ctx(&c, &orig, prop_int(&g, 0, 12));
        prop_recipe(&r, &g, 50);
        r.drft = (int16_t)prop_int(&g, 1, 100);
        ok2 &= prop_i2_i6(&orig, &r, &c);
        for (L = SEED_L_PLACE; L < SEED_L_N; L++)
            ok3 &= prop_i3(&orig, &r, &c, &g, (enum seed_layer)L, &changed);
        s = r;
        s.drft = (int16_t)prop_int(&g, 0, 100);
        if (seed_render(&a, &orig, &r, &c, &prop_work) == SEED_OK && seed_render(&out, &orig, &s, &c, &prop_work) == SEED_OK)
            okd &= (prop_diff(&a, &out, orig.length) & ~PROP_UTIME) == 0;
    }
    CHECK(ok2);
    CHECK(ok3);
    CHECK(okd);
    c = test_ctx(0);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    CHECK(seed_have_drift);
    test_bound();
    test_correlated();
    test_scales();
    test_own_numbers();
    test_vary();
    test_leaves();
    test_props();
    printf("test_timing_drift: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
