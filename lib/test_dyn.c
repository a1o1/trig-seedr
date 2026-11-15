/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for dyn.c: the metric weight, the velocity shapes' levels, AMT, VEL = DEF, HUM, ACC's
 * exact share, and the refusals for parts other tasks build. */
#include "tests.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

/* 16 steps, every one a trig at the default velocity */
static void base(void)
{
    int i;
    random_track(&orig, 16);
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

static void test_weight(void)
{
    static const int w4[16] = { 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };
    static const int w3[12] = { 4, 0, 0, 2, 0, 0, 3, 0, 0, 2, 0, 0 };
    int i, ok = 1;
    for (i = 0; i < 64; i++)
        ok &= seed_weight(i, 4) == w4[i % 16] && seed_weight(i, 3) == w3[i % 12];
    CHECK(ok);
    CHECK(seed_weight(-1, 4) == 0 && seed_weight(0, 0) == 4 && seed_weight(1, 1) == 2 && seed_weight(2, 1) == 3);
}

/* VEL 80..120 and AMT 100: each trig at its shape's level, 0..4 => 80, 90, 100, 110, 120 */
static void test_shapes(void)
{
    static const char *want[6] = {
        "40122102301221020",                                 /* unused: FLAT */
        "4010201030102010",                                  /* STRONG: the weights */
        "0434243414342434",                                  /* WEAK */
        "0242024202420242",                                  /* OFF */
        "0444044404440444",                                  /* NOT 1 */
        "0444444444444444" };                                /* NOTBAR */
    int shape, i;
    for (shape = SEED_SHAPE_STRONG; shape <= SEED_SHAPE_NOTBAR; shape++) {
        int ok = 1;
        base();
        r.vel.lo = 80;
        r.vel.hi = 120;
        r.shape = (int16_t)shape;
        r.amt = 100;
        CHECK(rendered());
        for (i = 0; i < 16; i++)
            ok &= out.s[i].vel == 80 + 10 * (want[shape][i] - '0');
        CHECK(ok);
    }
    /* the range either way round */
    base();
    r.vel.lo = 120;
    r.vel.hi = 80;
    r.shape = SEED_SHAPE_STRONG;
    r.amt = 100;
    CHECK(rendered() && out.s[0].vel == 120 && out.s[1].vel == 80);
    /* EVRY: every n-th trig loud */
    base();
    r.vel.lo = 80;
    r.vel.hi = 120;
    r.shape = SEED_SHAPE_EVRY;
    r.amt = 100;
    r.n = 3;
    orig.s[1].flags &= (uint16_t)~SEED_TRIG;                /* trigs 0, 2, 3, ...: the 4th trig is step 4 */
    CHECK(rendered());
    CHECK(out.s[0].vel == 120 && out.s[2].vel == 80 && out.s[3].vel == 80 && out.s[4].vel == 120);
    CHECK(out.s[1].vel == SEED_DEFAULT);                    /* no trig: untouched */
}

static void test_amt_def_hum(void)
{
    int i, ok = 1, lo = 127, hi = 0;
    /* AMT 50: half way from the drawn velocity to the target */
    base();
    r.vel.lo = 40;
    r.vel.hi = 120;
    r.shape = SEED_SHAPE_STRONG;
    r.amt = 50;
    CHECK(rendered());
    for (i = 0; i < 16; i++)
        ok &= out.s[i].vel >= (40 + (40 + 20 * seed_weight(i, 4))) / 2 && out.s[i].vel <= (120 + 40 + 20 * seed_weight(i, 4) + 1) / 2;
    CHECK(ok);
    /* VEL = DEF: the shape runs from half the trig's velocity to the velocity (100: 50..100; 60: 30..60) */
    base();
    r.shape = SEED_SHAPE_STRONG;
    r.amt = 100;
    orig.s[2].vel = 60;
    CHECK(rendered());
    CHECK(out.s[0].vel == 100 && out.s[1].vel == 50 && out.s[8].vel == 87 && out.s[2].vel == 37);
    /* VEL = DEF and only ACC: velocity untouched */
    base();
    r.acc = 50;
    CHECK(rendered());
    ok = 1;
    for (i = 0; i < 16; i++)
        ok &= out.s[i].vel == SEED_DEFAULT;
    CHECK(ok);
    /* HUM 100: up to +-32 */
    base();
    r.vel.lo = r.vel.hi = 64;
    r.hum = 100;
    for (r.seed = 0; r.seed < 50; r.seed++) {
        CHECK(rendered());
        for (i = 0; i < 16; i++) {
            if (out.s[i].vel < lo) lo = out.s[i].vel;
            if (out.s[i].vel > hi) hi = out.s[i].vel;
        }
    }
    CHECK(lo >= 32 && lo < 40 && hi <= 96 && hi > 88);
}

static void test_acc(void)
{
    int i, m = 0, kept = 1;
    base();
    for (i = 0; i < 16; i++)
        if (i % 3 == 0)
            orig.s[i].flags = (uint16_t)((orig.s[i].flags & ~SEED_TRIG) | SEED_ACCENT);    /* 6 empty, accented */
    r.acc = 50;                                             /* 10 trigs: 5 accents */
    CHECK(rendered());
    for (i = 0; i < 16; i++) {
        if (out.s[i].flags & SEED_TRIG)
            m += (out.s[i].flags & SEED_ACCENT) != 0;
        else
            kept &= (out.s[i].flags & SEED_ACCENT) != 0;
    }
    CHECK(m == 5);
    CHECK(kept);
    r.acc = 100;
    CHECK(rendered());
    for (i = m = 0; i < 16; i++)
        m += (out.s[i].flags & SEED_ACCENT) != 0;
    CHECK(m == 16);
}

static void test_refusals(void)
{
    static const int16_t later[3] = { SEED_SHAPE_RANK, SEED_SHAPE_NOISE, SEED_SHAPE_MAP };
    int i;
    base();
    r.acc = 20;
    r.amod = SEED_AMOD_TOP;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_rank ? SEED_OK : SEED_E_NA));
    r.amod = 7;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    for (i = 0; i < 3; i++) {
        base();
        r.shape = later[i];
        r.amt = 50;
        CHECK(seed_render(&out, &orig, &r, &c, &work) == (i == 0 && seed_have_rank ? SEED_OK : SEED_E_NA));
    }
    base();
    r.shape = SEED_SHAPE_STRONG;
    r.amt = 50;
    r.metr = SEED_METR_3_4;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_rank ? SEED_OK : SEED_E_NA));
    base();
    r.vel.lo = 1;
    r.vel.hi = 127;
    r.vel.src = SEED_VSRC_LIST;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_list ? SEED_OK : SEED_E_NA));
    base();
    r.shape = SEED_SHAPE_N;
    r.amt = 50;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_weight();
    test_shapes();
    test_amt_def_hum();
    test_acc();
    test_refusals();
    printf("test_dyn: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
