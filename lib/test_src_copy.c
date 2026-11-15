/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for src_copy.c: the positions of another track from the context, repetition and
 * truncation between lengths, a track the pattern lacks, the track in scope, what a generated
 * step holds, independence from the seed and from the other track's fields, refusals, and I2, I6
 * and I3 over random recipes with SOURCE = COPY. The emulator check is tools/tests/24-copy-mask.sh. */
#include "tests.h"
#include "props.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

/* track t of the context gets the pattern p ('x' a trig), its length strlen(p) */
static void other(int t, const char *p)
{
    int i;
    c.length[t] = (uint8_t)strlen(p);
    memset(c.trig[t], 0, sizeof c.trig[t]);
    for (i = 0; p[i]; i++)
        if (p[i] == 'x')
            c.trig[t][i >> 3] |= (uint8_t)(1u << (i & 7));
}

/* the track in scope is the original, in the context too, as the device and seedref give it */
static void base(int len, int from)
{
    int i;
    random_track(&orig, len);
    memset(&c, 0, sizeof c);
    c.caps = &test_caps;
    c.tracks = 12;
    c.track = 0;
    c.length[0] = (uint8_t)len;
    for (i = 0; i < len; i++)
        if (orig.s[i].flags & SEED_TRIG)
            c.trig[0][i >> 3] |= (uint8_t)(1u << (i & 7));
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_COPY;
    r.copy_trk = (int16_t)from;
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

static int is(const char *p)
{
    int i;
    for (i = 0; i < out.length; i++)
        if ((p[i] == 'x') != ((out.s[i].flags & SEED_TRIG) != 0))
            return 0;
    return p[i] == 0;
}

static void test_same_length(void)
{
    base(16, 3);
    other(3, "x..x..x...x.x...");
    CHECK(rendered());
    CHECK(is("x..x..x...x.x..."));
    other(3, "................");
    CHECK(rendered());
    CHECK(is("................"));
    other(3, "xxxxxxxxxxxxxxxx");
    CHECK(rendered());
    CHECK(is("xxxxxxxxxxxxxxxx"));
}

/* a shorter track repeats as it plays; a longer one gives its start */
static void test_lengths(void)
{
    base(32, 5);
    other(5, "x.x....x..x.");
    CHECK(rendered());
    CHECK(is("x.x....x..x.x.x....x..x.x.x....x"));
    base(7, 5);
    other(5, "x.x....x..x.x...xxxxxxxx");
    CHECK(rendered());
    CHECK(is("x.x...."));
    base(64, 1);
    other(1, "x");
    CHECK(rendered());
    CHECK(is("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
    /* trigs stored past the other track's length are not part of it */
    base(16, 2);
    other(2, "x.......");
    c.trig[2][1] = 0xff;
    CHECK(rendered());
    CHECK(is("x.......x......."));
}

/* a track the pattern lacks (length 0) gives no trigs; TRK outside the pattern is refused */
static void test_missing(void)
{
    base(16, 9);
    c.length[9] = 0;
    c.trig[9][0] = 0xff;
    CHECK(rendered());
    CHECK(is("................"));
    r.copy_trk = 12;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.copy_trk = -1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.copy_trk = 11;
    CHECK(rendered());
}

/* TRK = the track in scope: its original rhythm, as new trigs */
static void test_self(void)
{
    int i, ok = 1;
    base(48, 0);
    CHECK(rendered());
    for (i = 0; i < 48; i++)
        ok &= (out.s[i].flags & SEED_TRIG) == (orig.s[i].flags & SEED_TRIG);
    CHECK(ok);
}

/* every step below the length is a generated step; past it, untouched (I6) */
static void test_step(void)
{
    int i, ok = 1;
    base(40, 4);
    other(4, "x.xx..x.");
    CHECK(rendered());
    for (i = 0; i < 40; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        ok &= (s->flags & ~SEED_TRIG) == (o->flags & ~SEED_KNOWN);     /* swing and unknown bits stay */
        ok &= s->vel == SEED_DEFAULT && s->note == SEED_DEFAULT && s->len == SEED_DEFAULT && s->utime == 0;
        ok &= s->cond == SEED_NO_COND && s->rlen == SEED_NEW_RLEN && s->rrate == SEED_NEW_RRATE && s->rvel == 0;
        ok &= s->origin == SEED_NO_ORIGIN && s->lane[0] == o->lane[0] && s->lane[1] == o->lane[1];
    }
    for (i = 40; i < SEED_STEPS; i++)
        ok &= memcmp(&out.s[i], &orig.s[i], sizeof out.s[i]) == 0;
    CHECK(ok);
    CHECK(out.length == 40);
}

/* no random numbers: seeds, VARY and the track in scope give the same rhythm (I2); only the
 * context's trigs count, not the original's fields */
static void test_seeds(void)
{
    struct seed_track a;
    int i, equal = 1;
    base(64, 7);
    other(7, "x...x.x.xx..x...x..x");
    CHECK(rendered());
    a = out;
    for (i = 0; i < 20; i++) {
        r.seed = (uint32_t)i * 91u;
        r.vary = (int16_t)(i * 5);
        c.track = i % 7;                                    /* never 7: the copied track stays */
        CHECK(rendered());
        equal &= memcmp(&a, &out, sizeof a) == 0;
    }
    CHECK(equal);
}

/* PLACE on top: ROT rotates the copy */
static void test_rot(void)
{
    base(16, 3);
    other(3, "x..x..x...x.x...");
    r.rot = 1;
    CHECK(rendered());
    CHECK(is(".x..x..x...x.x.."));
}

/* I2, I6 and I3 (props.h) over random recipes with SOURCE = COPY, random other tracks, and
 * every other built layer */
static void test_props(void)
{
    struct prop_rng g = { 424 };
    struct seed_track o, t;
    struct seed_recipe pr;
    struct seed_ctx pc;
    int i, j, L, ok26 = 1, ok3 = 1;
    for (i = 0; i < 400; i++) {
        int len = prop_int(&g, 1, SEED_STEPS);
        prop_track(&o, &g, len, prop_int(&g, 0, 100));
        prop_ctx(&pc, &o, prop_int(&g, 0, 12));
        for (j = 0; j < pc.tracks; j++) {
            if (j == pc.track)
                continue;
            prop_track(&t, &g, prop_int(&g, 1, SEED_STEPS), prop_int(&g, 0, 100));
            pc.length[j] = (uint8_t)t.length;
            memset(pc.trig[j], 0, sizeof pc.trig[j]);
            for (L = 0; L < t.length; L++)
                if (t.s[L].flags & SEED_TRIG)
                    pc.trig[j][L >> 3] |= (uint8_t)(1u << (L & 7));
        }
        prop_recipe(&pr, &g, 50);
        pr.src = SEED_SRC_COPY;
        pr.copy_trk = (int16_t)prop_int(&g, 0, pc.tracks - 1);
        if (seed_have_mask && prop_chance(&g, 30)) {
            pr.mask = (int16_t)prop_int(&g, SEED_MASK_AVOID, SEED_MASK_ONLY);
            pr.mtrk = (int16_t)prop_int(&g, 0, pc.tracks - 1);
        }
        ok26 &= prop_i2_i6(&o, &pr, &pc);
        for (L = SEED_L_PLACE; L < SEED_L_N; L++) {
            unsigned changed;
            ok3 &= prop_i3(&o, &pr, &pc, &g, (enum seed_layer)L, &changed);
        }
    }
    CHECK(ok26);
    CHECK(ok3);
}

int main(void)
{
    test_setup();
    CHECK(seed_have_copy == 1);
    test_same_length();
    test_lengths();
    test_missing();
    test_self();
    test_step();
    test_seeds();
    test_rot();
    test_props();
    printf("test_src_copy: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
