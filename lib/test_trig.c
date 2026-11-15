/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for trig.c: A:B codes, COND with each CTYP, exact shares, RTRG. */
#include "tests.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(void)
{
    random_track(&orig, 48);
    seed_recipe_defaults(&r, &test_caps);
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

/* 1 when every trig's condition passes ok(cond) and every other step's is unchanged */
static int conds(int (*ok)(int))
{
    int i, all = 1;
    for (i = 0; i < 48; i++)
        all &= orig.s[i].flags & SEED_TRIG ? ok(out.s[i].cond) : out.s[i].cond == orig.s[i].cond;
    return all;
}
static int is_prob(int v) { return v >= 10 && v <= 21; }
static int is_ab4(int v) { return v >= 35 && v <= 38; }          /* 1:4 .. 4:4 */
static int is_1st(int v) { return v == 28; }
static int is_fill(int v) { return v == 22; }
static int is_mix(int v) { return is_prob(v) || (v >= 57 && v <= 64) || v == 28 || v == 22; }

static void test_cond(void)
{
    int i, m = 0, types[4] = { 0 };
    CHECK(seed_cond_ab(1, 2) == 30 && seed_cond_ab(1, 4) == 35 && seed_cond_ab(1, 8) == 57 && seed_cond_ab(8, 8) == 64);
    base();
    r.cond = 100;
    CHECK(rendered() && conds(is_prob));
    r.ctyp = SEED_CTYP_AB;
    r.cval.lo = 4;
    CHECK(rendered() && conds(is_ab4));
    r.ctyp = SEED_CTYP_1ST;
    CHECK(rendered() && conds(is_1st));
    r.ctyp = SEED_CTYP_FILL;
    CHECK(rendered() && conds(is_fill));
    r.ctyp = SEED_CTYP_MIX;
    r.cval.lo = 10;                                         /* PROB 50%..100%; A:B on 8 */
    CHECK(rendered() && conds(is_mix));
    for (i = 0; i < 48; i++)
        if (orig.s[i].flags & SEED_TRIG) {
            int v = out.s[i].cond;
            types[is_prob(v) ? 0 : v == 28 ? 2 : v == 22 ? 3 : 1]++;
        }
    CHECK(types[0] && types[1] && types[2] && types[3]);
    /* an exact share; the other trigs lose their conditions */
    r.ctyp = SEED_CTYP_PROB;
    r.cond = 25;
    CHECK(rendered());
    for (i = 0; i < 48; i++)
        if (orig.s[i].flags & SEED_TRIG)
            m += out.s[i].cond != SEED_NO_COND;
    CHECK(m == seed_share(25, ntrigs(&orig)));
    r.ctyp = SEED_CTYP_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

static void test_rtrg(void)
{
    int i, ok = 1, m = 0;
    base();
    r.rtrg = 100;
    r.rate.lo = 3;
    r.rate.hi = 5;
    r.rcur = -40;
    CHECK(rendered());
    for (i = 0; i < 48; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        if (o->flags & SEED_TRIG)
            ok &= (s->flags & SEED_RETRIG) && s->rrate >= 3 && s->rrate <= 5 && s->rvel == -40 && s->rlen == o->rlen;
        else
            ok &= s->flags == o->flags && s->rrate == o->rrate && s->rvel == o->rvel;
        ok &= s->cond == o->cond;                           /* COND at 0: left */
    }
    CHECK(ok);
    r.rtrg = 40;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 48; i++)
        if (orig.s[i].flags & SEED_TRIG) {
            if (out.s[i].flags & SEED_RETRIG)
                m++;
            else
                ok &= out.s[i].rrate == orig.s[i].rrate && out.s[i].rvel == orig.s[i].rvel;
        }
    CHECK(ok);
    CHECK(m == seed_share(40, ntrigs(&orig)));
    /* RATE = DEF: a new trig's rate */
    r.rtrg = 100;
    r.rate.lo = r.rate.hi = SEED_DEF;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 48; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].rrate == SEED_NEW_RRATE;
    CHECK(ok);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_cond();
    test_rtrg();
    printf("test_trig: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
