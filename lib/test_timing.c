/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for timing.c: RAND within the device's range, QUANT, a groove's timing and velocity
 * (more in test_port.c), and the refusals. */
#include "tests.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(void)
{
    random_track(&orig, 32);
    seed_recipe_defaults(&r, &test_caps);
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

static void test_rand(void)
{
    int i, ok = 1, lo = 99, hi = -99;
    struct seed_caps narrow = test_caps;
    base();
    r.src = SEED_SRC_RANDOM;
    r.dens = 100;
    r.rand = 100;                                           /* a generated step: +-23 */
    for (r.seed = 0; r.seed < 40; r.seed++) {
        CHECK(rendered());
        for (i = 0; i < 32; i++) {
            if (out.s[i].utime < lo) lo = out.s[i].utime;
            if (out.s[i].utime > hi) hi = out.s[i].utime;
        }
    }
    CHECK(lo == -23 && hi == 23);
    r.rand = 50;                                            /* +-11 */
    for (r.seed = 0, lo = 99, hi = -99; r.seed < 40; r.seed++) {
        CHECK(rendered());
        for (i = 0; i < 32; i++) {
            if (out.s[i].utime < lo) lo = out.s[i].utime;
            if (out.s[i].utime > hi) hi = out.s[i].utime;
        }
    }
    CHECK(lo == -11 && hi == 11);
    /* the device's range */
    narrow.utime_lo = -6;
    narrow.utime_hi = 10;
    c.caps = &narrow;
    r.rand = 100;
    for (r.seed = 0, lo = 99, hi = -99; r.seed < 40; r.seed++) {
        CHECK(rendered());
        for (i = 0; i < 32; i++) {
            if (out.s[i].utime < lo) lo = out.s[i].utime;
            if (out.s[i].utime > hi) hi = out.s[i].utime;
        }
    }
    CHECK(lo == -6 && hi == 10);
    c.caps = &test_caps;
    /* on SOURCE = TRACK it adds to the trig's micro timing; steps without a trig stay */
    base();
    r.rand = 20;
    CHECK(rendered());
    for (i = 0; i < 32; i++) {
        int d = out.s[i].utime - orig.s[i].utime;
        ok &= orig.s[i].flags & SEED_TRIG ? (d >= -4 && d <= 4) || out.s[i].utime == 23 || out.s[i].utime == -23 : d == 0;
    }
    CHECK(ok);
}

static void test_quant_groove(void)
{
    int i, ok = 1;
    base();
    r.quant = 100;
    CHECK(rendered());
    for (i = 0; i < 32; i++)
        ok &= out.s[i].utime == (orig.s[i].flags & SEED_TRIG ? 0 : orig.s[i].utime);
    CHECK(ok);
    /* groove 1: every second step 5 late; QUANT 100 first, TIME 60 => 3 */
    r.grv = 1;
    r.timing = 60;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].utime == (i & 1 ? 3 : 0);
    CHECK(ok);
    r.shft = 1;                                             /* one step later */
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].utime == (i & 1 ? 0 : 3);
    CHECK(ok);
    /* GVEL with no velocity data (groove 3) leaves velocity; GVEL at 0 does too */
    r.grv = 3;
    r.gvel = 100;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        ok &= out.s[i].vel == orig.s[i].vel;
    CHECK(ok);
    /* GVEL -100 .. 100 on a trig at the default velocity writes a value */
    r.grv = 2;
    r.gvel = 100;
    r.shft = 0;
    orig.default_vel = 100;
    for (i = 0; i < 32; i++)
        orig.s[i].vel = SEED_DEFAULT;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        if ((orig.s[i].flags & SEED_TRIG) && test_grooves[1].vel[i % 16])
            ok &= out.s[i].vel <= 100 && out.s[i].vel >= 1;
    CHECK(ok);
}

static void test_refusals(void)
{
    base();
    r.grv = 4;                                              /* 3 grooves */
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    c.grooves = NULL;
    r.grv = 1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    c.grooves = test_grooves;
    r.grv = 0;
    r.drft = 30;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_drift ? SEED_OK : SEED_E_NA));
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_rand();
    test_quant_groove();
    test_refusals();
    printf("test_timing: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
