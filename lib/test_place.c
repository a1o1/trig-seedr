/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for place.c: ROT with origins and swing, SKIP, the field rotation (lanes included) and
 * seed_rotate's argument checks. ROTATE itself is compared with 0.3's in test_port.c. */
#include "tests.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(int len)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

static void test_rot(void)
{
    int i, ok = 1;
    base(12);
    r.rot = -5;                                             /* = +7 in 12 steps */
    CHECK(rendered());
    for (i = 0; i < 12; i++) {
        const struct seed_step *s = &out.s[(i + 7) % 12], *o = &orig.s[i];
        ok &= s->origin == i && s->vel == o->vel && s->lane[0] == o->lane[0] && s->lane[1] == o->lane[1];
        ok &= (s->flags & ~SEED_SWING) == (o->flags & ~SEED_SWING);
        ok &= (out.s[i].flags & SEED_SWING) == (orig.s[i].flags & SEED_SWING);   /* swing stays */
    }
    CHECK(ok);
    r.rot = 12;                                             /* a whole turn */
    CHECK(rendered() && memcmp(&out, &orig, sizeof out) == 0);
}

static void test_skip(void)
{
    int i, ok = 1;
    base(16);
    r.skip = 5;
    CHECK(rendered());
    for (i = 0; i < 16; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        if (i < 5)
            ok &= !(s->flags & SEED_KNOWN) && (s->flags & ~SEED_KNOWN) == (o->flags & ~SEED_KNOWN)
                  && s->origin == SEED_NO_ORIGIN && s->vel == SEED_DEFAULT && s->lane[0] == o->lane[0];
        else
            ok &= memcmp(s, o, sizeof *s) == 0;
    }
    CHECK(ok);
    r.skip = 64;                                            /* past the length: the whole track */
    CHECK(rendered() && ntrigs(&out) == 0);
    CHECK(memcmp(&out.s[16], &orig.s[16], sizeof out.s[0] * 48) == 0);
}

static void test_fields(void)
{
    int i, j, ok = 1, idx[SEED_STEPS], n = 0;
    base(32);
    r.fld = SEED_FLD_LANE2;
    r.frot = 2;
    r.fmod = SEED_RMODE_TRIG;
    CHECK(rendered());
    for (i = 0; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            idx[n++] = i;
    for (j = 0; j < n; j++)                                 /* lane 2 two trigs later */
        ok &= out.s[idx[(j + 2) % n]].lane[1] == orig.s[idx[j]].lane[1];
    for (i = 0; i < 32; i++)
        ok &= out.s[i].lane[0] == orig.s[i].lane[0] && out.s[i].vel == orig.s[i].vel && out.s[i].origin == i;
    CHECK(n >= 2 && ok);
    /* the rotation runs last: it moves what DYNAMICS wrote */
    base(16);
    for (i = 0; i < 16; i++)
        orig.s[i].flags |= SEED_TRIG;
    r.vel.lo = 80;
    r.vel.hi = 120;
    r.shape = SEED_SHAPE_STRONG;
    r.amt = 100;
    r.fld = SEED_FLD_VEL;
    r.frot = 1;
    CHECK(rendered() && out.s[1].vel == 120 && out.s[0].vel == 80);
    r.fld = SEED_FLD_ALL;                                   /* not a field rotation */
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.fld = SEED_FLD_VEL;
    r.fmod = SEED_RMODE_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

static void test_args(void)
{
    struct seed_track t;
    random_track(&t, 64);
    CHECK(seed_rotate(&t, -1, 4, 1, SEED_FLD_VEL, 0) == SEED_E_ARG);
    CHECK(seed_rotate(&t, 60, 5, 1, SEED_FLD_VEL, 0) == SEED_E_ARG);
    CHECK(seed_rotate(&t, 0, 0, 1, SEED_FLD_VEL, 0) == SEED_E_ARG);
    CHECK(seed_rotate(&t, 0, 4, 1, SEED_FLD_OFF, 0) == SEED_E_ARG);
    CHECK(seed_rotate(&t, 0, 4, 1, SEED_FLD_N, 0) == SEED_E_ARG);
    CHECK(seed_rotate(&t, 0, 4, 1, SEED_FLD_VEL, 2) == SEED_E_ARG);
    CHECK(seed_rotate(&t, 0, 64, 64, SEED_FLD_ALL, 0) == SEED_OK);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_rot();
    test_skip();
    test_fields();
    test_args();
    printf("test_place: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
