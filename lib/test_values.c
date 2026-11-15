/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for values.c: RANGE values, NOTE (LEN, NOTE) and LOCK (both lanes, 0.3's sample
 * locks as lane 1), and the refusals for LIST, NOISE and LEGATO while F6 and F7 are not built. */
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

static void test_range(void)
{
    struct seed_values v;
    struct seed_take k;
    int i, lo = 99, hi = -99;
    seed_take_init(&k, 5, 0, 0, 0);
    v.lo = 3;
    v.hi = -2;
    v.src = SEED_VSRC_RANGE;
    for (i = 0; i < 64; i++) {
        int x = seed_value(&v, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i, 0);
        if (x < lo) lo = x;
        if (x > hi) hi = x;
        CHECK(x == seed_value(&v, &k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i, 17));   /* by step */
    }
    CHECK(lo == -2 && hi == 3);
    v.src = 9;
    CHECK(seed_value(&v, &k, 0, 0, 0) == SEED_E_ARG);
}

static void test_note(void)
{
    int i, ok = 1, spread = 0;
    base();
    orig.default_note = 60;
    r.len.lo = r.len.hi = 14;
    r.note.lo = -2;
    r.note.hi = 2;
    CHECK(rendered());
    for (i = 0; i < 32; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        if (o->flags & SEED_TRIG) {
            ok &= s->len == 14 && s->note >= 58 && s->note <= 62;
            spread |= 1 << (s->note - 58);
        } else {
            ok &= s->len == o->len && s->note == o->note;
        }
    }
    CHECK(ok);
    CHECK(spread == 0x1f);
    /* the note stays inside 0..126 */
    base();
    orig.default_note = 120;
    r.note.lo = r.note.hi = 24;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].note == 126;
    CHECK(ok);
    /* LEN alone leaves the note */
    base();
    r.len.lo = 0;
    r.len.hi = 127;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        ok &= out.s[i].note == orig.s[i].note;
    CHECK(ok);
}

static void test_lock(void)
{
    int i, ok = 1, m = 0, n;
    base();
    n = ntrigs(&orig);
    r.lane[0].pct = 100;
    r.lane[0].val.lo = 3;
    r.lane[0].val.hi = 5;
    CHECK(rendered());
    for (i = 0; i < 32; i++) {
        const struct seed_step *s = &out.s[i];
        ok &= orig.s[i].flags & SEED_TRIG ? s->lane[0] >= 3 && s->lane[0] <= 5 : s->lane[0] == SEED_NO_LOCK;
        ok &= s->lane[1] == orig.s[i].lane[1];             /* lane 2 at 0 %: left */
    }
    CHECK(ok);
    /* an exact share; the others lose their locks */
    r.lane[0].pct = 50;
    CHECK(rendered());
    for (i = 0; i < 32; i++)
        m += out.s[i].lane[0] != SEED_NO_LOCK;
    CHECK(m == seed_share(50, n));
    /* the parameter's range: DEF takes all of it, values outside are clamped */
    r.lane[0].pct = 100;
    r.lane[0].val.lo = 200;
    r.lane[0].val.hi = 300;
    r.lane[1].pct = 100;
    r.lane[1].val.lo = r.lane[1].val.hi = SEED_DEF;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].lane[0] == 127 && out.s[i].lane[1] >= -64 && out.s[i].lane[1] <= 63;
    CHECK(ok);
    /* no capabilities: the lane's values as they come */
    c.caps = NULL;
    r.lane[1].pct = 0;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].lane[0] >= 200 && out.s[i].lane[0] <= 300;
    CHECK(ok);
    c.caps = &test_caps;
}

static void test_refusals(void)
{
    base();
    test_caps.lanes = 1;                                    /* a device with one lane */
    r.lane[1].pct = 10;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_NA);
    test_caps.lanes = 2;
    r.lane[1].parm = 2;                                     /* no such parameter */
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_NA);
    base();
    r.lane[0].pct = 10;
    r.lane[0].val.src = SEED_VSRC_NOISE;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_noise ? SEED_OK : SEED_E_NA));
    base();
    r.note.lo = 0;
    r.note.hi = 5;
    r.note.src = SEED_VSRC_LIST;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_list ? SEED_OK : SEED_E_NA));
    base();
    r.lmod = SEED_LMOD_LEGATO;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_list ? SEED_OK : SEED_E_NA));
    r.lmod = SEED_LMOD_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_range();
    test_note();
    test_lock();
    test_refusals();
    printf("test_values: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
