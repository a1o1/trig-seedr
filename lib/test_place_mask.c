/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for place_mask.c: AVOID and ONLY against another track, repetition between lengths,
 * a track the pattern lacks, the track in scope, that MASK only removes (I5) and leaves the trigs
 * it keeps as they were, that it holds on the final positions after ROT (P5), refusals, and I2,
 * I6 and I3 over random recipes with MASK on. The emulator check is tools/tests/24-copy-mask.sh. */
#include "tests.h"
#include "props.h"

static struct seed_track orig, out, off;
static struct seed_recipe r;
static struct seed_ctx c;

static void other(int t, const char *p)
{
    int i;
    c.length[t] = (uint8_t)strlen(p);
    memset(c.trig[t], 0, sizeof c.trig[t]);
    for (i = 0; p[i]; i++)
        if (p[i] == 'x')
            c.trig[t][i >> 3] |= (uint8_t)(1u << (i & 7));
}

/* an original with the trigs p and random fields; SOURCE = TRACK, MASK m against track mt */
static void base(const char *p, int m, int mt)
{
    int i, len = (int)strlen(p);
    random_track(&orig, len);
    for (i = 0; i < len; i++)
        orig.s[i].flags = (uint16_t)(p[i] == 'x' ? orig.s[i].flags | SEED_TRIG : orig.s[i].flags & ~SEED_TRIG);
    memset(&c, 0, sizeof c);
    c.caps = &test_caps;
    c.tracks = 12;
    c.track = 0;
    other(0, p);
    seed_recipe_defaults(&r, &test_caps);
    r.mask = (int16_t)m;
    r.mtrk = (int16_t)mt;
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

/* the same recipe with MASK OFF, into `off` */
static void unmasked(void)
{
    struct seed_recipe s = r;
    s.mask = SEED_MASK_OFF;
    CHECK(seed_render(&off, &orig, &s, &c, &work) == SEED_OK);
}

/* every step MASK kept is the unmasked step; every step it changed was a trig there and is now
 * SKIP's cleared step (I5: only removes) */
static int only_removes(void)
{
    int i, ok = 1;
    for (i = 0; i < out.length; i++) {
        if (!memcmp(&out.s[i], &off.s[i], sizeof out.s[i]))
            continue;
        struct seed_step cl = off.s[i];
        seed_step_clear(&cl);
        ok &= (off.s[i].flags & SEED_TRIG) && !memcmp(&out.s[i], &cl, sizeof cl);
    }
    return ok;
}

static void test_avoid_only(void)
{
    base("x.x.x.x.xxxx....", SEED_MASK_AVOID, 2);
    other(2, "x...x...x...x...");
    CHECK(rendered());
    CHECK(is("..x...x..xxx...."));
    unmasked();
    CHECK(only_removes());
    r.mask = SEED_MASK_ONLY;
    CHECK(rendered());
    CHECK(is("x...x...x......."));
    CHECK(only_removes());
}

/* AVOID and ONLY split the unmasked trigs between them */
static void test_split(void)
{
    struct seed_track a;
    int i, ok = 1;
    random_track(&orig, 64);
    base("x.xx..x.x..xxx.x.xx.x..x.xx.x.x..x..xx..x.x.xxx..x.x.x..x.xxx.x.", SEED_MASK_AVOID, 5);
    other(5, ".x.xx..xx.x.xx..x..xxx.x.x.x..xxxx");
    CHECK(rendered());
    a = out;
    r.mask = SEED_MASK_ONLY;
    CHECK(rendered());
    unmasked();
    for (i = 0; i < 64; i++) {
        int ta = (a.s[i].flags & SEED_TRIG) != 0, to = (out.s[i].flags & SEED_TRIG) != 0;
        ok &= !(ta && to) && (ta || to) == ((off.s[i].flags & SEED_TRIG) != 0);
    }
    CHECK(ok);
}

/* a shorter mask track repeats as it plays; a longer one gives its start */
static void test_lengths(void)
{
    base("xxxxxxxxxxxxxxxxxxxxxxxx", SEED_MASK_AVOID, 3);
    other(3, "x.x....x");
    CHECK(rendered());
    CHECK(is(".x.xxxx..x.xxxx..x.xxxx."));
    base("xxxxxx", SEED_MASK_ONLY, 3);
    other(3, "..x..x..x..xxxxx");
    CHECK(rendered());
    CHECK(is("..x..x"));
}

/* a track the pattern lacks: AVOID keeps every trig, ONLY none; MTRK outside the pattern and an
 * unknown MASK are refused */
static void test_missing(void)
{
    base("x.x.xx.x", SEED_MASK_AVOID, 8);
    c.length[8] = 0;
    c.trig[8][0] = 0xff;
    CHECK(rendered());
    CHECK(is("x.x.xx.x"));
    r.mask = SEED_MASK_ONLY;
    CHECK(rendered());
    CHECK(is("........"));
    r.mtrk = 12;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.mtrk = -1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.mtrk = 1;
    r.mask = SEED_MASK_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.mask = -1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

/* MTRK = the track in scope: with SOURCE = TRACK, ONLY keeps the track, AVOID clears it; with ROT
 * it keeps (ONLY) the rotated trigs that land on an original trig */
static void test_self(void)
{
    base("x..xx.x.x.......", SEED_MASK_ONLY, 0);
    CHECK(rendered());
    CHECK(memcmp(&out, &orig, sizeof out) == 0);
    r.mask = SEED_MASK_AVOID;
    CHECK(rendered());
    CHECK(is("................"));
    r.mask = SEED_MASK_ONLY;
    r.rot = 1;
    CHECK(rendered());
    CHECK(is("....x..........."));
}

/* P5: MASK holds on the final positions; ROT before it and SKIP after it */
static void test_final(void)
{
    int i, ok = 1;
    base("xxxx.xxx.xxxxx.xx.xxxxx.xxx.xxx.", SEED_MASK_AVOID, 4);
    other(4, "x..x.x.....xx..x");
    r.rot = 5;
    r.skip = 3;
    CHECK(rendered());
    for (i = 0; i < 32; i++)
        ok &= !((out.s[i].flags & SEED_TRIG) && seed_ctx_trig(&c, 4, i % 16));
    for (i = 0; i < 3; i++)
        ok &= !(out.s[i].flags & SEED_TRIG);
    CHECK(ok);
    unmasked();
    CHECK(only_removes());
}

/* with every built layer after it, MASK still only removes trigs, and the dressing layers see
 * the final positions: no trig of the result is on a masked step */
static void test_layers(void)
{
    int i, j, ok = 1, okc = 1;
    for (j = 0; j < 200; j++) {
        char p[SEED_STEPS + 1], q[17];
        int len = rnd(1, SEED_STEPS);
        for (i = 0; i < len; i++)
            p[i] = rnd(0, 1) ? 'x' : '.';
        p[len] = 0;
        for (i = 0; i < 16; i++)
            q[i] = rnd(0, 1) ? 'x' : '.';
        q[16] = 0;
        base(p, rnd(SEED_MASK_AVOID, SEED_MASK_ONLY), 6);
        other(6, q);
        random_recipe(&r);
        r.src = rnd(0, 1) ? SEED_SRC_TRACK : SEED_SRC_RANDOM;
        r.mask = (int16_t)rnd(SEED_MASK_AVOID, SEED_MASK_ONLY);
        r.mtrk = 6;
        r.fld = SEED_FLD_OFF;                               /* FLD moves values after MASK */
        if (!rendered())
            continue;
        for (i = 0; i < len; i++)
            ok &= !(out.s[i].flags & SEED_TRIG) || seed_ctx_trig(&c, 6, i % 16) == (r.mask == SEED_MASK_ONLY);
        {   /* the trig count never rises over the unmasked render's */
            struct seed_recipe s = r;
            int a = 0, b = 0;
            s.mask = SEED_MASK_OFF;
            okc &= seed_render(&off, &orig, &s, &c, &work) == SEED_OK;
            for (i = 0; i < len; i++) {
                a += (out.s[i].flags & SEED_TRIG) != 0;
                b += (off.s[i].flags & SEED_TRIG) != 0;
            }
            okc &= a <= b;
        }
    }
    CHECK(ok);
    CHECK(okc);
}

/* I2, I6 and I3 (props.h) over random recipes with MASK on against random other tracks */
static void test_props(void)
{
    struct prop_rng g = { 77 };
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
        pr.mask = (int16_t)prop_int(&g, SEED_MASK_AVOID, SEED_MASK_ONLY);
        pr.mtrk = (int16_t)prop_int(&g, 0, pc.tracks - 1);
        ok26 &= prop_i2_i6(&o, &pr, &pc);
        for (L = SEED_L_DYN; L < SEED_L_N; L++) {
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
    CHECK(seed_have_mask == 1);
    test_avoid_only();
    test_split();
    test_lengths();
    test_missing();
    test_self();
    test_final();
    test_layers();
    test_props();
    printf("test_place_mask: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
