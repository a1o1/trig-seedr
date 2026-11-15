/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for noise.c and dyn_noise.c (F7): integer value noise, the NOISE value source for
 * NOTE and both LOCK lanes, SHAPE = NOISE. The noise itself (corners, bounds, smoothness, ZOOM,
 * negative x), the mapping onto a range (bounds, either order, equal shares), VARY over the corners
 * (P3, I10), each stage against seed_value_noise / seed_shape_noise, a trig's value independent of
 * DENS and the lock % (P1, I4), and I1, I2, I3, I6 with NOISE on (props.h). */
#include "tests.h"
#include "props.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static int iabs(int v) { return v < 0 ? -v : v; }

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

/* the take seed_render makes for r on track 0, candidate 0 */
static struct seed_take take_of(const struct seed_recipe *q)
{
    struct seed_take k;
    seed_take_init(&k, q->seed, 0, q->vary, 0);
    return k;
}

/* a RANDOM source at DENS, everything else at its default */
static void random_base(int dens)
{
    random_track(&orig, 16);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_RANDOM;
    r.dens = (int16_t)dens;
}

static void test_noise_fn(void)
{
    int seed, x, z, ok_range = 1, ok_corner = 1, ok_between = 1, ok_step = 1, ok_one = 1, ok_curve = 1;
    for (seed = 0; seed < 200; seed++)
        for (z = 1; z <= 16; z++)
            for (x = -40; x < 80; x++) {
                int v = seed_noise((uint32_t)seed, x, z), f = ((x % z) + z) % z, c0 = (x - f) / z;
                int a = (int)(seed_hash((uint32_t)seed ^ seed_hash((uint32_t)c0)) >> 24);
                int b = (int)(seed_hash((uint32_t)seed ^ seed_hash((uint32_t)(c0 + 1))) >> 24);
                int lo = a < b ? a : b, hi = a < b ? b : a, d = iabs(seed_noise((uint32_t)seed, x + 1, z) - v);
                ok_range &= v >= 0 && v <= 255;
                if (f == 0)
                    ok_corner &= v == a;                        /* every z-th step is a corner */
                ok_between &= v >= lo && v <= hi;
                /* smoothstep's steepest part is 1.5 times the straight line's slope; a step is at
                 * most 256 / z + 1 of the 256 parts of a cell (noise.c floors t) */
                if (f + 1 < z)
                    ok_step &= d <= 3 * (hi - lo) * (256 / z + 1) / 512 + 2;
                if (z == 1)
                    ok_one &= v == a;
                /* smoothstep, not a straight line: a quarter of a cell in, 40 / 256 of the way */
                if (z == 4 && f == 1)
                    ok_curve &= v == (a * 216 + b * 40 + 128) >> 8;
            }
    CHECK(ok_range);
    CHECK(ok_corner);
    CHECK(ok_between);
    CHECK(ok_step);
    CHECK(ok_one);
    CHECK(ok_curve);
    /* ZOOM is clamped to 1..64; the same seed gives the same contour, another seed another */
    CHECK(seed_noise(7, 5, 0) == seed_noise(7, 5, 1));
    CHECK(seed_noise(7, 5, 500) == seed_noise(7, 5, 64));
    CHECK(seed_noise(7, 13, 4) == seed_noise(7, 13, 4));
    for (x = 0, z = 0; x < 64; x++)
        z += seed_noise(7, x, 4) != seed_noise(8, x, 4);
    CHECK(z > 48);
}

/* mean step-to-step change, x 100, over many seeds: higher ZOOM is smoother */
static int roughness(int zoom)
{
    long sum = 0;
    int seed, x;
    for (seed = 0; seed < 300; seed++)
        for (x = 0; x < 63; x++)
            sum += iabs(seed_noise((uint32_t)seed, x + 1, zoom) - seed_noise((uint32_t)seed, x, zoom));
    return (int)(sum * 100 / (300 * 63));
}

static void test_smooth(void)
{
    int r1 = roughness(1), r4 = roughness(4), r16 = roughness(16);
    printf("noise: mean change per step x100: ZOOM 1 %d, 4 %d, 16 %d\n", r1, r4, r16);
    CHECK(r4 * 3 < r1);
    CHECK(r16 * 3 < r4);
}

static void test_value_map(void)
{
    struct seed_values v, w;
    struct seed_take k;
    int seed, i, ok = 1, mirror = 1, same = 1, count[8] = { 0 };
    v.src = SEED_VSRC_NOISE;
    v.order = SEED_ORDR_UP;
    v.len = 4;
    for (seed = 0; seed < 500; seed++) {
        seed_take_init(&k, (uint32_t)seed, 3, 0, 0);
        for (i = 0; i < SEED_STEPS; i++) {
            int a, b;
            v.lo = 1; v.hi = 8; v.zoom = 1;
            a = seed_value_noise(&v, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i);
            ok &= a >= 1 && a <= 8;
            if (a >= 1 && a <= 8)
                count[a - 1]++;
            v.lo = -24; v.hi = 24; v.zoom = 5;
            w = v; w.lo = 24; w.hi = -24;
            a = seed_value_noise(&v, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i);
            b = seed_value_noise(&w, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i);
            ok &= a >= -24 && a <= 24;
            mirror &= a + b == 0;                               /* high to low: the contour turned over */
            v.lo = v.hi = 6;
            same &= seed_value_noise(&v, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i) == 6;
        }
    }
    CHECK(ok);
    CHECK(mirror);
    CHECK(same);
    /* ZOOM 1: each of 1..8 in an equal share, as RANGE (4000 each expected) */
    for (i = 0, ok = 1; i < 8; i++)
        ok &= count[i] > 3600 && count[i] < 4400;
    CHECK(ok);
}

/* corners kept from take n to n + 1, in % of 16 corners x 400 takes, at VARY */
static int kept(int vary)
{
    struct seed_values v = { 0, 255, SEED_VSRC_NOISE, SEED_ORDR_UP, 4, 4 };
    struct seed_take a, b;
    int n, i, same = 0;
    for (n = 100; n < 500; n++) {
        seed_take_init(&a, (uint32_t)n, 1, vary, 0);
        seed_take_init(&b, (uint32_t)n + 1u, 1, vary, 0);
        for (i = 0; i < SEED_STEPS; i += 4)                     /* the corners: level 0..255 as is */
            same += seed_value_noise(&v, &a, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL, 0), i)
                == seed_value_noise(&v, &b, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL, 0), i);
    }
    return same * 100 / (400 * 16);
}

static void test_vary(void)
{
    int k0 = kept(0), k50 = kept(50), k80 = kept(80), k100 = kept(100);
    printf("noise: corners kept from take n to n + 1: VARY 0 %d %%, 50 %d %%, 80 %d %%, 100 %d %%\n", k0, k50, k80, k100);
    CHECK(k0 <= 2);                                             /* equal by chance: 1 in 256 */
    CHECK(k50 >= 43 && k50 <= 57);
    CHECK(k80 >= 73 && k80 <= 87);
    CHECK(k100 == 100);
}

/* NOTE = NOISE: every trig's note is the default plus the source's value at its step */
static void test_note(void)
{
    struct seed_take k;
    int seed, i, ok = 1, trigs = 0;
    c = test_ctx(0);
    for (seed = 1; seed <= 50; seed++) {
        random_base(75);
        r.seed = (uint32_t)seed;
        r.note.lo = -5; r.note.hi = 7; r.note.src = SEED_VSRC_NOISE; r.note.zoom = 3;
        k = take_of(&r);
        if (!rendered()) {
            ok = 0;
            continue;
        }
        for (i = 0; i < 16; i++)
            if (out.s[i].flags & SEED_TRIG) {
                int off = seed_value_noise(&r.note, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i);
                ok &= off >= -5 && off <= 7 && out.s[i].note == orig.default_note + off;
                trigs++;
            }
    }
    CHECK(ok);
    CHECK(trigs > 400);
}

/* P1: a trig in both takes keeps its note when DENS changes (the value is the step's, not the trig's
 * place in the order); LEN by NOISE too (V9: every value field takes the source) */
static void test_dens_independent(void)
{
    static struct seed_track a;
    int seed, i, ok = 1, both = 0;
    c = test_ctx(0);
    for (seed = 1; seed <= 50; seed++) {
        random_base(40);
        r.seed = (uint32_t)seed;
        r.note.lo = -12; r.note.hi = 12; r.note.src = SEED_VSRC_NOISE; r.note.zoom = 4;
        r.len.lo = 0; r.len.hi = 60; r.len.src = SEED_VSRC_NOISE; r.len.zoom = 2;
        CHECK(rendered());
        a = out;
        r.dens = 80;
        CHECK(rendered());
        for (i = 0; i < 16; i++)
            if (a.s[i].flags & out.s[i].flags & SEED_TRIG) {
                ok &= a.s[i].note == out.s[i].note && a.s[i].len == out.s[i].len;
                both++;
            }
    }
    CHECK(ok);
    CHECK(both > 200);
}

/* LOCK = NOISE on both lanes: values from the source, clamped to the parameter; raising % keeps
 * every lock with its value (I4) */
static void test_lock(void)
{
    static struct seed_track a;
    struct seed_take k;
    int seed, i, l, ok = 1, nest = 1, locks = 0;
    c = test_ctx(0);
    for (seed = 1; seed <= 50; seed++) {
        random_base(100);
        r.seed = (uint32_t)seed;
        for (l = 0; l < SEED_LANES; l++) {
            r.lane[l].pct = 30;
            r.lane[l].val.src = SEED_VSRC_NOISE;
            r.lane[l].val.zoom = (int16_t)(l ? 8 : 2);
        }
        r.lane[0].val.lo = 10; r.lane[0].val.hi = 20;
        r.lane[1].val.lo = -80; r.lane[1].val.hi = 80;           /* TUNE is -64..63 */
        k = take_of(&r);
        CHECK(rendered());
        a = out;
        for (i = 0; i < 16; i++)
            for (l = 0; l < SEED_LANES; l++)
                if (a.s[i].lane[l] != SEED_NO_LOCK) {
                    int v = seed_value_noise(&r.lane[l].val, &k, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL + l, 0), i);
                    int lo = test_params[l].lo, hi = test_params[l].hi;
                    ok &= a.s[i].lane[l] == (v < lo ? lo : v > hi ? hi : v);
                    locks++;
                }
        r.lane[0].pct = r.lane[1].pct = 70;
        CHECK(rendered());
        for (i = 0; i < 16; i++)
            for (l = 0; l < SEED_LANES; l++)
                if (a.s[i].lane[l] != SEED_NO_LOCK)
                    nest &= out.s[i].lane[l] == a.s[i].lane[l];
    }
    CHECK(ok);
    CHECK(nest);
    CHECK(locks > 400);
}

/* SHAPE = NOISE: velocity at AMT 100 is VEL's low end + the level's share of the range */
static void test_shape(void)
{
    static struct seed_track a;
    struct seed_take k;
    int seed, i, ok = 1, differ = 0, same = 1;
    c = test_ctx(0);
    for (seed = 1; seed <= 50; seed++) {
        random_base(100);
        r.seed = (uint32_t)seed;
        r.vel.lo = 20; r.vel.hi = 120;
        r.vel.src = SEED_VSRC_RANGE;
        r.shape = SEED_SHAPE_NOISE;
        r.amt = 100;
        r.n = 8;
        k = take_of(&r);
        CHECK(rendered());
        for (i = 0; i < 16; i++) {
            int lev = seed_shape_noise(&r, &k, i);
            ok &= lev >= 0 && lev <= 255 && out.s[i].vel == 20 + (100 * lev + 127) / 255;
        }
        a = out;
        r.n = 1;                                                /* N is the zoom */
        CHECK(rendered());
        differ += memcmp(&a, &out, sizeof a) != 0;
        r.n = 8;
        r.vary = 100;                                           /* VARY 100: one contour for every seed */
        CHECK(rendered());
        a = out;
        r.seed++;
        CHECK(rendered());
        same &= memcmp(&a, &out, sizeof a) == 0;
    }
    CHECK(ok);
    CHECK(differ == 50);
    CHECK(same);
}

/* P1: NOTE, each lane and SHAPE have their own corners */
static void test_slots(void)
{
    struct seed_values v = { 0, 255, SEED_VSRC_NOISE, SEED_ORDR_UP, 4, 1 };
    struct seed_take k;
    struct seed_recipe q;
    int i, d01 = 0, dn = 0, ds = 0;
    seed_take_init(&k, 9, 0, 0, 0);
    seed_recipe_defaults(&q, &test_caps);
    q.n = 1;
    for (i = 0; i < SEED_STEPS; i++) {
        int n0 = seed_value_noise(&v, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i);
        int l0 = seed_value_noise(&v, &k, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL, 0), i);
        int l1 = seed_value_noise(&v, &k, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL + 1, 0), i);
        d01 += l0 != l1;
        dn += n0 != l0;
        ds += seed_shape_noise(&q, &k, i) != n0;
    }
    CHECK(d01 > 56 && dn > 56 && ds > 56);
}

/* I1, I2, I6 and I3 for every layer, with NOISE on in NOTE, the lanes or SHAPE */
static void test_props_noise(void)
{
    static struct seed_track t;
    struct prop_rng g = { 77 };
    struct seed_ctx cx;
    struct seed_recipe q;
    int n, L, i1 = 0, i2 = 0, i3 = 0, refused = 0;
    for (n = 0; n < 1500; n++) {
        unsigned ch;
        prop_track(&t, &g, prop_int(&g, 1, SEED_STEPS), prop_int(&g, 0, 100));
        prop_ctx(&cx, &t, prop_int(&g, 0, 12));
        i1 += !prop_i1(&t, &cx, (uint32_t)prop_int(&g, 0, 9999), prop_int(&g, 0, 100));
        prop_recipe(&q, &g, 50);
        if (prop_chance(&g, 60)) {
            prop_values(&q.note, &g, -24, 24);
            q.note.src = SEED_VSRC_NOISE;
            q.note.zoom = (int16_t)prop_int(&g, 1, 64);
        }
        if (prop_chance(&g, 60)) {
            q.lane[0].pct = (int16_t)prop_int(&g, 1, 100);
            prop_values(&q.lane[0].val, &g, 0, 127);
            q.lane[0].val.src = SEED_VSRC_NOISE;
            q.lane[0].val.zoom = (int16_t)prop_int(&g, 1, 64);
        }
        if (prop_chance(&g, 60)) {
            q.shape = SEED_SHAPE_NOISE;
            q.amt = (int16_t)prop_int(&g, 1, 100);
            q.n = (int16_t)prop_int(&g, 1, 64);
        }
        refused += seed_render(&out, &t, &q, &cx, &prop_work) != SEED_OK;
        i2 += !prop_i2_i6(&t, &q, &cx);
        for (L = 0; L < SEED_L_N; L++)
            i3 += !prop_i3(&t, &q, &cx, &g, (enum seed_layer)L, &ch);
    }
    printf("noise props: I1 %d, I2 + I6 %d, I3 %d failures in 1500 cases (%d refused)\n", i1, i2, i3, refused);
    CHECK(i1 == 0 && i2 == 0 && i3 == 0);
    CHECK(refused == 0);
}

int main(void)
{
    test_setup();
    CHECK(seed_have_noise == 1);
    test_noise_fn();
    test_smooth();
    test_value_map();
    test_vary();
    test_note();
    test_dens_independent();
    test_lock();
    test_shape();
    test_slots();
    test_props_noise();
    printf("test_noise: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
