/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for meter.c and dyn_rank.c (task F1): Barlow's indispensability against published
 * values, every meter and BEAT, the roles, ranks over several bars, the five-level weight against
 * 0.3's, SHAPE = RANK, AMOD = TOP, refusals, and I2, I3, I4, I5, I6 over random recipes with
 * every meter and role. Prints BIAS's response curve (docs/status/F1.md). */
#include "tests.h"

#define ROUNDS 2000

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

/* psi of step x in a one-bar track of len steps, back from the rank */
static int psi_of(int metr, int beat, int role, int x, int len)
{
    int v = seed_rank(metr, beat, role, x, len);
    return len == 1 ? 0 : (int)(((long)v * (len - 1) + SEED_RANK_MAX / 2) / SEED_RANK_MAX);
}

static int bar_len(int metr, int beat)
{
    static const int units[SEED_METR_N] = { 4, 3, 6, 12, 5, 7 };
    return units[metr] * (beat < 1 ? 1 : beat > 16 ? 16 : beat);
}

static int same(int metr, int beat, const int *want, int len)
{
    int i, ok = 1;
    for (i = 0; i < len; i++)
        ok &= psi_of(metr, beat, SEED_ROLE_METER, i, len) == want[i];
    return ok;
}

/* the plan's checks and hmt's tables (Music.Theory.Meter.Barlow_1987) */
static void test_barlow(void)
{
    static const int b44[16] = { 15, 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7 };
    static const int b32[6] = { 5, 0, 3, 1, 4, 2 };           /* 3 x 2: 3/4 in eighths */
    static const int b23[6] = { 5, 0, 2, 4, 1, 3 };           /* 2 x 3: 6/8 */
    static const int b322[12] = { 11, 0, 6, 3, 9, 1, 7, 4, 10, 2, 8, 5 };
    static const int b223[12] = { 11, 0, 4, 8, 2, 6, 10, 1, 5, 9, 3, 7 };
    static const int b5[5] = { 4, 0, 3, 1, 2 }, b7[7] = { 6, 0, 4, 2, 5, 1, 3 };
    static const int b52[10] = { 9, 0, 5, 3, 8, 1, 6, 2, 7, 4 };  /* 5 x 2 */
    CHECK(same(SEED_METR_4_4, 4, b44, 16));
    CHECK(same(SEED_METR_3_4, 2, b32, 6));
    CHECK(same(SEED_METR_6_8, 1, b23, 6));
    CHECK(same(SEED_METR_3_4, 4, b322, 12));
    CHECK(same(SEED_METR_12_16, 1, b223, 12));
    CHECK(same(SEED_METR_5_8, 1, b5, 5));
    CHECK(same(SEED_METR_7_8, 1, b7, 7));
    CHECK(same(SEED_METR_5_8, 2, b52, 10));
    CHECK(seed_rank(SEED_METR_4_4, 1, SEED_ROLE_METER, 0, 1) == SEED_RANK_MAX);
}

/* every meter, BEAT and role: a bar's ranks are distinct; METER's follow the five-level weight
 * (a heavier class is always above a lighter one); 4/4 with an even BEAT is 0.3's weight */
static void test_every_meter(void)
{
    int metr, beat, role, i, j, distinct = 1, ordered = 1, w44 = 1, odd44 = 0;
    for (metr = 0; metr < SEED_METR_N; metr++)
        for (beat = 1; beat <= 16; beat++) {
            int len = bar_len(metr, beat), rk[192], w[192];
            for (role = 0; role < SEED_ROLE_N; role++) {
                for (i = 0; i < len; i++) {
                    rk[i] = psi_of(metr, beat, role, i, len);
                    w[i] = seed_meter_weight(metr, beat, i);
                }
                for (i = 0; i < len; i++)
                    for (j = 0; j < len; j++) {
                        distinct &= i == j || rk[i] != rk[j];
                        if (role == SEED_ROLE_METER)
                            ordered &= !(w[i] > w[j]) || rk[i] > rk[j];
                    }
            }
            if (metr == SEED_METR_4_4)
                for (i = 0; i < 64; i++) {
                    int a = seed_meter_weight(metr, beat, i), b = seed_weight(i, beat);
                    if (beat % 2 == 0 || beat == 1)
                        w44 &= a == b;
                    else
                        odd44 += a != b;
                }
        }
    CHECK(distinct);
    CHECK(ordered);
    CHECK(w44);
    printf("4/4, odd BEAT 3..15: %d steps of 448 weigh 1 (the beat's first division), 0 in 0.3\n", odd44);
    CHECK(seed_meter_weight(SEED_METR_6_8, 2, 6) == 2 && seed_meter_weight(SEED_METR_6_8, 2, 2) == 1
          && seed_meter_weight(SEED_METR_6_8, 2, 1) == 0 && seed_meter_weight(SEED_METR_12_16, 1, 6) == 3
          && seed_meter_weight(SEED_METR_12_16, 1, 3) == 2 && seed_meter_weight(SEED_METR_3_4, 4, 4) == 2);
}

/* the steps of a 16-step 4/4 bar in a role's order, strongest first */
static void role_order(int role, int *top)
{
    int i, j, rk[16];
    for (i = 0; i < 16; i++)
        rk[i] = seed_rank(SEED_METR_4_4, 4, role, i, 16);
    for (i = 0; i < 16; i++) {
        int best = -1;
        for (j = 0; j < 16; j++)
            if (rk[j] >= 0 && (best < 0 || rk[j] > rk[best]))
                best = j;
        top[i] = best;
        rk[best] = -1;
    }
}

static void test_roles(void)
{
    static const int want[SEED_ROLE_N][16] = {
        { 0, 8, 12, 4, 14, 6, 10, 2, 15, 7, 11, 3, 13, 5, 9, 1 },     /* METER: Barlow */
        { 0, 8, 14, 6, 10, 2, 15, 7, 11, 3, 13, 5, 9, 1, 12, 4 },     /* KICK: D G O F B */
        { 12, 4, 15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 8, 0 },     /* BACK: B F O G D */
        { 14, 6, 10, 2, 0, 8, 12, 4, 15, 7, 11, 3, 13, 5, 9, 1 },     /* HAT: O D G B F */
        { 15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0 },     /* PERC: F O B G D */
    };
    int role, i, top[16];
    for (role = 0; role < SEED_ROLE_N; role++) {
        int ok = 1;
        role_order(role, top);
        for (i = 0; i < 16; i++)
            ok &= top[i] == want[role][i];
        CHECK(ok);
    }
    /* 3/4 with BEAT 1: no class G; BACK puts beats 2 and 3 first */
    CHECK(seed_rank(SEED_METR_3_4, 1, SEED_ROLE_BACK, 2, 3) > seed_rank(SEED_METR_3_4, 1, SEED_ROLE_BACK, 1, 3)
          && seed_rank(SEED_METR_3_4, 1, SEED_ROLE_BACK, 1, 3) > seed_rank(SEED_METR_3_4, 1, SEED_ROLE_BACK, 0, 3));
}

/* several bars: the same order in each, ties between bars to bars 1, 3, 4, 2 */
static void test_bars(void)
{
    int i, j, ok = 1, distinct = 1;
    for (i = 0; i < 64; i++)
        for (j = 0; j < 64; j++) {
            int a = seed_rank(SEED_METR_4_4, 4, SEED_ROLE_METER, i, 64), b = seed_rank(SEED_METR_4_4, 4, SEED_ROLE_METER, j, 64);
            int pa = psi_of(SEED_METR_4_4, 4, SEED_ROLE_METER, i % 16, 16), pb = psi_of(SEED_METR_4_4, 4, SEED_ROLE_METER, j % 16, 16);
            distinct &= i == j || a != b;
            ok &= !(pa > pb) || a > b;
        }
    CHECK(distinct && ok);
    CHECK(seed_rank(0, 4, 0, 0, 64) > seed_rank(0, 4, 0, 32, 64) && seed_rank(0, 4, 0, 32, 64) > seed_rank(0, 4, 0, 48, 64)
          && seed_rank(0, 4, 0, 48, 64) > seed_rank(0, 4, 0, 16, 64) && seed_rank(0, 4, 0, 16, 64) > seed_rank(0, 4, 0, 8, 64));
    /* a track of 3 bars of 3/4 (BEAT 4): 36 steps, bars rounded up to 4 */
    for (i = 0, distinct = 1; i < 36; i++)
        for (j = 0; j < i; j++)
            distinct &= seed_rank(SEED_METR_3_4, 4, SEED_ROLE_HAT, i, 36) != seed_rank(SEED_METR_3_4, 4, SEED_ROLE_HAT, j, 36);
    CHECK(distinct);
}

/* len steps, every one a trig at the default velocity */
static void all_trigs(int len)
{
    int i;
    random_track(&orig, len);
    orig.default_vel = 100;
    for (i = 0; i < SEED_STEPS; i++) {
        orig.s[i].flags = (uint16_t)((orig.s[i].flags & ~SEED_ACCENT) | SEED_TRIG);
        orig.s[i].vel = SEED_DEFAULT;
    }
    seed_recipe_defaults(&r, &test_caps);
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

static void test_shape_rank(void)
{
    int i, ok = 1;
    all_trigs(32);
    r.vel.lo = 1;
    r.vel.hi = 121;
    r.shape = SEED_SHAPE_RANK;
    r.amt = 100;
    CHECK(rendered());
    for (i = 0; i < 32; i++)                                /* psi 0..15 => 1, 9, ..., 121 */
        ok &= out.s[i].vel == 1 + 8 * psi_of(SEED_METR_4_4, 4, SEED_ROLE_METER, i % 16, 16);
    CHECK(ok);
    r.role = SEED_ROLE_BACK;                                /* a RHYTHM knob: no effect here (I3) */
    {
        struct seed_track a = out;
        CHECK(rendered());
        CHECK(memcmp(&a, &out, sizeof a) == 0);
    }
    r.metr = SEED_METR_6_8;                                 /* 6/8, BEAT 2: 12 steps */
    r.beat = 2;
    CHECK(rendered());
    CHECK(out.s[0].vel == 121 && out.s[6].vel > out.s[2].vel && out.s[2].vel > out.s[1].vel && out.s[1].vel == 1);
    r.metr = SEED_METR_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

static void test_acc_top(void)
{
    int i, pct, m, nest = 1, exact = 1, seeds = 1;
    uint16_t prev[SEED_STEPS] = { 0 };
    all_trigs(16);
    r.amod = SEED_AMOD_TOP;
    r.acc = 25;
    CHECK(rendered());
    for (i = 0; i < 16; i++)
        CHECK(((out.s[i].flags & SEED_ACCENT) != 0) == (i == 0 || i == 4 || i == 8 || i == 12));
    for (pct = 0; pct <= 100; pct += 5) {                  /* I4 nesting, I5 exact count */
        r.acc = (int16_t)pct;
        if (pct == 0)
            continue;
        CHECK(rendered());
        for (i = m = 0; i < 16; i++) {
            m += (out.s[i].flags & SEED_ACCENT) != 0;
            nest &= !(prev[i] & SEED_ACCENT) || (out.s[i].flags & SEED_ACCENT);
            prev[i] = out.s[i].flags;
        }
        exact &= m == seed_share(pct, 16);
    }
    CHECK(nest && exact);
    for (i = 0; i < 20; i++) {                              /* the rank alone decides: no seed */
        struct seed_track a;
        r.acc = 40;
        r.seed = (uint32_t)i;
        CHECK(rendered());
        a = out;
        r.seed = (uint32_t)i + 100;
        CHECK(rendered());
        seeds &= memcmp(&a, &out, sizeof a) == 0;
    }
    CHECK(seeds);
    /* only trigs: a track with trigs on odd steps accents the strongest of those */
    all_trigs(16);
    for (i = 0; i < 16; i++)
        if (i % 2 == 0)
            orig.s[i].flags &= (uint16_t)~SEED_TRIG;
    r.amod = SEED_AMOD_TOP;
    r.acc = 25;                                             /* 2 of 8: steps 15 and 7 */
    CHECK(rendered());
    for (i = m = 0; i < 16; i++)
        m += (out.s[i].flags & SEED_ACCENT) != 0;
    CHECK(m == 2 && (out.s[15].flags & SEED_ACCENT) && (out.s[7].flags & SEED_ACCENT));
    r.metr = 99;                                            /* refused before velocity is written */
    r.vel.lo = r.vel.hi = 50;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

static void test_meters_render(void)
{
    char p[SEED_STEPS + 1];
    int i;
    all_trigs(12);                                          /* 12/16, BEAT 1: one bar */
    r.metr = SEED_METR_12_16;
    r.beat = 1;
    r.vel.lo = 40;
    r.vel.hi = 120;
    r.shape = SEED_SHAPE_STRONG;
    r.amt = 100;
    CHECK(rendered());
    for (i = 0; i < 12; i++)
        p[i] = (char)('0' + (out.s[i].vel - 40) / 20);
    p[12] = 0;
    CHECK(strcmp(p, "411211311211") == 0);            /* no finer steps: all but the beats weigh 1 */
    seed_recipe_defaults(&r, &test_caps);                    /* RANDOM, BIAS 100 in 7/8: 2 + 2 + 3 */
    r.src = SEED_SRC_RANDOM;
    r.metr = SEED_METR_7_8;
    r.beat = 2;
    r.dens = 22;
    r.bias = 100;
    orig.length = 14;
    CHECK(rendered());
    for (i = 0; i < 14; i++)
        p[i] = out.s[i].flags & SEED_TRIG ? 'x' : '.';
    p[14] = 0;
    CHECK(strcmp(p, "x...x...x.....") == 0);
    r.role = SEED_ROLE_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.role = SEED_ROLE_METER;
    r.metr = -1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    CHECK(seed_rank(0, 4, 0, -1, 16) == SEED_E_ARG && seed_meter_weight(SEED_METR_N, 4, 0) == SEED_E_ARG);
}

/* a random recipe over every meter, BEAT, role, shape and AMOD */
static void random_f1(struct seed_recipe *q)
{
    static const int16_t shapes[8] = { SEED_SHAPE_FLAT, SEED_SHAPE_STRONG, SEED_SHAPE_WEAK, SEED_SHAPE_OFF,
                                       SEED_SHAPE_NOT1, SEED_SHAPE_NOTBAR, SEED_SHAPE_RANK, SEED_SHAPE_EVRY };
    random_recipe(q);
    q->metr = (int16_t)rnd(0, SEED_METR_N - 1);
    q->beat = (int16_t)rnd(1, 16);
    q->role = (int16_t)rnd(0, SEED_ROLE_N - 1);
    q->bias = (int16_t)rnd(-100, 100);
    q->shape = shapes[rnd(0, 7)];
    q->amod = (int16_t)rnd(0, 1);
}

static void random_dyn_f1(struct seed_recipe *q)
{
    static const int16_t shapes[3] = { SEED_SHAPE_RANK, SEED_SHAPE_STRONG, SEED_SHAPE_OFF };
    random_values(&q->vel, 1, 127, 1);
    q->shape = shapes[rnd(0, 2)];
    q->amt = (int16_t)rnd(0, 100);
    q->acc = (int16_t)rnd(0, 100);
    q->amod = (int16_t)rnd(0, 1);
}

static void test_props_f1(void)
{
    static struct seed_track a, b;
    struct seed_recipe q, s;
    struct seed_ctx cx = test_ctx(0);
    int i, j, i2 = 1, i6 = 1, i3 = 1, i4 = 1, i5 = 1, ok = 0;
    for (i = 0; i < ROUNDS; i++) {
        int len = rnd(1, 64);
        random_track(&orig, len);
        cx.length[0] = (uint8_t)len;
        random_f1(&q);
        if (seed_render(&a, &orig, &q, &cx, &work) != SEED_OK)
            continue;
        ok++;
        CHECK(seed_render(&b, &orig, &q, &cx, &work) == SEED_OK);
        i2 &= memcmp(&a, &b, sizeof a) == 0;
        for (j = len; j < SEED_STEPS; j++)
            i6 &= memcmp(&a.s[j], &orig.s[j], sizeof a.s[j]) == 0;
        /* I3: other DYNAMICS settings change velocity and the accent flag only */
        s = q;
        random_dyn_f1(&s);
        if (seed_render(&b, &orig, &s, &cx, &work) == SEED_OK)
            for (j = 0; j < SEED_STEPS; j++) {
                struct seed_step x = a.s[j], y = b.s[j];
                x.vel = y.vel = 0;
                x.flags &= (uint16_t)~SEED_ACCENT;
                y.flags &= (uint16_t)~SEED_ACCENT;
                i3 &= memcmp(&x, &y, sizeof x) == 0;
            }
        /* I4, I5: RANDOM with this meter, role and BIAS; raising DENS keeps every trig */
        if (q.src == SEED_SRC_RANDOM) {
            struct seed_recipe d = q;
            int k, prev[SEED_STEPS] = { 0 };
            d.rot = d.skip = 0;
            for (k = 0; k <= 100; k += 10) {
                int m = 0, pos = 0, grid = d.grid == 2 || d.grid == 4 || d.grid == 8 ? d.grid : 1;
                d.dens = (int16_t)k;
                if (seed_render(&b, &orig, &d, &cx, &work) != SEED_OK)
                    break;
                for (j = 0; j < len; j++) {
                    int t = (b.s[j].flags & SEED_TRIG) != 0;
                    i4 &= !prev[j] || t;
                    prev[j] = t;
                    m += t;
                    pos += j % grid == 0;
                }
                i5 &= m == seed_share(k, pos);
            }
        }
    }
    CHECK(ok > ROUNDS / 2);
    CHECK(i2 && i6);
    CHECK(i3);
    CHECK(i4 && i5);
}

/* BIAS's response: trigs on beats (steps 0, 4, 8, 12) of 1,200 at DENS 25 on 16 steps, 4/4 */
static void test_bias_curve(void)
{
    int b, i, j, on[11], kick[11];
    printf("BIAS     -100  -80  -60  -40  -20    0   20   40   60   80  100\n");
    for (b = 0; b <= 10; b++) {
        on[b] = kick[b] = 0;
        for (i = 0; i < 300; i++) {
            all_trigs(16);
            r.src = SEED_SRC_RANDOM;
            r.seed = (uint32_t)i;
            r.dens = 25;
            r.bias = (int16_t)(b * 20 - 100);
            CHECK(rendered());
            for (j = 0; j < 16; j += 4)
                on[b] += (out.s[j].flags & SEED_TRIG) != 0;
            kick[b] += (out.s[0].flags & SEED_TRIG) != 0;
        }
    }
    printf("beats  ");
    for (b = 0; b <= 10; b++)
        printf(" %4d", on[b]);
    printf("\nstep 1 ");
    for (b = 0; b <= 10; b++)
        printf(" %4d", kick[b]);
    printf("  (of 300)\n");
    for (b = 1; b <= 10; b++)
        CHECK(on[b] >= on[b - 1]);
    CHECK(on[0] == 0 && on[10] == 1200 && on[5] > 200 && on[5] < 400);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_barlow();
    test_every_meter();
    test_roles();
    test_bars();
    test_shape_rank();
    test_acc_top();
    test_meters_render();
    test_props_f1();
    test_bias_curve();
    printf("test_meter: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
