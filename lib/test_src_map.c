/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for src_map.c: the levels against the original code's, as hashes; the arguments;
 * the DENS threshold and its order (I4); CHAOS as a slot per part per pass (P3) and its range;
 * X, Y and CHAOS from the pattern's shared settings (A1); what a generated step holds and the
 * steps past the length (I6); SHAPE = MAP; and props.h's I2, I6 and I3 with SOURCE = MAP.
 *
 * The hashes are FNV-1a over the levels, written by tools/tests/23-map.cc from the original code
 * (ReadDrumMap over the original resources.cc). That program, run by tools/tests/23-map.sh, also
 * compares every level and the source's trigs with the original directly. */
#include "tests.h"
#include "props.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(int len, int part, int dens, int x, int y, int chaos)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_MAP;
    r.map_part = (int16_t)part;
    r.map_dens = (int16_t)dens;
    c.shared.map_x = (int16_t)x;
    c.shared.map_y = (int16_t)y;
    c.shared.map_chaos = (int16_t)chaos;
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

static int trigs(const struct seed_track *t)
{
    int i, m = 0;
    for (i = 0; i < t->length; i++)
        m += (t->s[i].flags & SEED_TRIG) != 0;
    return m;
}

static uint32_t fnv(uint32_t h, int b)
{
    return (h ^ (uint8_t)b) * 16777619u;
}

/* tools/tests/23-map.cc's output: every level (X, Y 0..255, part, step 0..31), then per X of pts
 * the levels over every Y of pts */
static const uint8_t pts[15] = { 0, 1, 32, 63, 64, 65, 100, 127, 128, 160, 191, 192, 224, 254, 255 };
static const uint32_t hash_all = 0xfc767ca9u;
static const uint32_t hash_x[15] = {
    0x2f0e7675u, 0xc9f332f9u, 0x22b36523u, 0xc0715a40u, 0x19b70153u, 0x59ecb6cfu, 0xdd0ae674u, 0x99451592u,
    0x45992e22u, 0x45823c5cu, 0xf19bd024u, 0x17da8ee7u, 0xc43c401eu, 0x53997897u, 0x7a2bb4bbu
};

/* the levels: the original code's, and steps beyond 32 repeat */
static void test_levels(void)
{
    uint32_t h = 2166136261u;
    int x, y, p, s, ok = 1, rep = 1, range = 1;
    for (x = 0; x < 256; x++)
        for (y = 0; y < 256; y++)
            for (p = 0; p < 3; p++)
                for (s = 0; s < 32; s++) {
                    int v = seed_map_level(p, x, y, s);
                    range &= v >= 0 && v <= 255;
                    h = fnv(h, v);
                }
    CHECK(range);
    CHECK(h == hash_all);
    for (x = 0; x < 15; x++) {
        h = 2166136261u;
        for (y = 0; y < 15; y++)
            for (p = 0; p < 3; p++)
                for (s = 0; s < 32; s++)
                    h = fnv(h, seed_map_level(p, pts[x], pts[y], s));
        ok &= h == hash_x[x];
        if (h != hash_x[x])
            printf("  x %d: hash 0x%08x\n", pts[x], (unsigned)h);
    }
    CHECK(ok);
    for (x = 0; x < 256; x += 17)
        for (p = 0; p < 3; p++)
            for (s = 0; s < 32; s++)
                rep &= seed_map_level(p, x, 255 - x, s) == seed_map_level(p, x, 255 - x, s + 32);
    CHECK(rep);
}

/* parts outside 0..2 and negative steps refuse; X and Y are clamped to 0..255 */
static void test_args(void)
{
    CHECK(seed_map_level(-1, 0, 0, 0) == SEED_E_ARG);
    CHECK(seed_map_level(3, 0, 0, 0) == SEED_E_ARG);
    CHECK(seed_map_level(0, 0, 0, -1) == SEED_E_ARG);
    CHECK(seed_map_level(1, -40, 300, 5) == seed_map_level(1, 0, 255, 5));
    CHECK(seed_map_level(2, 1000, -1, 7) == seed_map_level(2, 255, 0, 7));
    base(16, 3, 128, 0, 0, 0);
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.map_part = -1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

/* DENS: a step fires when its level exceeds 255 - DENS (CHAOS 0); DENS 0 gives none; raising
 * DENS keeps every trig (I4); DENS outside 0..255 is clamped */
static void test_dens(void)
{
    struct seed_track prev;
    int d, i, p, ok = 1, nest = 1, xy;
    static const int16_t xys[4][2] = { { 0, 0 }, { 128, 128 }, { 37, 211 }, { 255, 255 } };
    for (xy = 0; xy < 4; xy++)
        for (p = 0; p < 3; p++) {
            base(64, p, 0, xys[xy][0], xys[xy][1], 0);
            CHECK(rendered());
            CHECK(trigs(&out) == 0);
            prev = out;
            for (d = 0; d <= 255; d++) {
                r.map_dens = (int16_t)d;
                CHECK(rendered());
                for (i = 0; i < 64; i++) {
                    int on = (out.s[i].flags & SEED_TRIG) != 0;
                    ok &= on == (seed_map_level(p, xys[xy][0], xys[xy][1], i) > 255 - d);
                    nest &= !(prev.s[i].flags & SEED_TRIG) || on;
                }
                prev = out;
            }
        }
    CHECK(ok);
    CHECK(nest);
    base(32, 0, 255, 0, 0, 0);
    CHECK(rendered());
    prev = out;
    r.map_dens = 999;
    CHECK(rendered());
    CHECK(memcmp(&prev, &out, sizeof out) == 0);
    r.map_dens = -5;
    CHECK(rendered());
    CHECK(trigs(&out) == 0);
}

/* the key a step of pass `pass` gets with CHAOS chaos at take k: the original's rule */
static int key_of(const struct seed_take *k, int part, int x, int y, int step, int chaos)
{
    int lev = seed_map_level(part, x, y, step), byte, pert;
    byte = (int)(seed_rand(k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_CHAOS, part * 256 + step / 32)) >> 24);
    pert = byte * (chaos >> 2) >> 8;
    return lev < 255 - pert ? lev + pert : 255;
}

/* CHAOS: one offset per part per pass from its slot; up to (CHAOS / 4) * 255 / 256; CHAOS 0 and
 * 1..3 use no random numbers; with CHAOS the seed changes the take, VARY 100 holds it (P3) */
static void test_chaos(void)
{
    struct seed_take k;
    struct seed_track a;
    int i, ok = 1, n, differ = 0, seed;
    base(64, 1, 90, 100, 60, 200);
    for (seed = 1; seed <= 30; seed++) {
        r.seed = (uint32_t)seed;
        CHECK(rendered());
        seed_take_init(&k, (uint32_t)seed, (uint32_t)c.track, 0, 0);
        for (i = 0; i < 64; i++)
            ok &= ((out.s[i].flags & SEED_TRIG) != 0) == (key_of(&k, 1, 100, 60, i, 200) > 255 - 90);
    }
    CHECK(ok);
    /* CHAOS 255 lifts a level by at most 62 (255 * 63 / 256) */
    base(32, 0, 0, 0, 0, 255);
    for (seed = 1, ok = 1; seed <= 200; seed++) {
        r.seed = (uint32_t)seed;
        r.map_dens = 62;                          /* threshold 193: only a level above 130 can reach it */
        CHECK(rendered());
        for (i = 0; i < 32; i++)
            if (out.s[i].flags & SEED_TRIG)
                ok &= seed_map_level(0, 0, 0, i) + 62 > 193;
    }
    CHECK(ok);
    /* CHAOS below 4: no offset, so every seed gives the same rhythm */
    for (n = 0; n < 4; n++) {
        base(64, 2, 140, 77, 177, n);
        CHECK(rendered());
        a = out;
        for (seed = 2, ok = 1; seed < 20; seed++) {
            r.seed = (uint32_t)seed;
            CHECK(rendered());
            ok &= memcmp(&a, &out, sizeof a) == 0;
        }
        CHECK(ok);
    }
    /* CHAOS: seeds differ; VARY 100: every seed the same take */
    base(64, 0, 100, 128, 128, 255);
    CHECK(rendered());
    a = out;
    for (seed = 2; seed < 20; seed++) {
        r.seed = (uint32_t)seed;
        CHECK(rendered());
        differ += memcmp(&a, &out, sizeof a) != 0;
    }
    CHECK(differ > 0);
    r.vary = 100;
    r.seed = 1;
    CHECK(rendered());
    a = out;
    for (seed = 2, ok = 1; seed < 20; seed++) {
        r.seed = (uint32_t)seed;
        CHECK(rendered());
        ok &= memcmp(&a, &out, sizeof a) == 0;
    }
    CHECK(ok);
}

/* A1: X, Y and CHAOS are the pattern's (the context), PART and DENS the track's (the recipe); with
 * CHAOS 0 two tracks with the same PART and DENS get the same rhythm */
static void test_shared(void)
{
    struct seed_track a;
    int ok = 1, t;
    base(32, 0, 160, 64, 192, 0);
    CHECK(rendered());
    a = out;
    for (t = 1; t < 12; t++) {
        c.track = t;
        CHECK(rendered());
        ok &= memcmp(&a, &out, sizeof a) == 0;
    }
    c.track = 0;
    CHECK(ok);
    c.shared.map_x = 200;
    CHECK(rendered());
    CHECK(memcmp(&a, &out, sizeof a) != 0);
}

/* a generated step holds the defaults (seed_step_clear); steps past the length stay (I6) */
static void test_step(void)
{
    int i, ok = 1;
    base(40, 0, 200, 30, 30, 120);
    CHECK(rendered());
    CHECK(trigs(&out) > 0);
    for (i = 0; i < 40; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        ok &= (s->flags & ~SEED_TRIG) == (o->flags & ~SEED_KNOWN);
        ok &= s->vel == SEED_DEFAULT && s->note == SEED_DEFAULT && s->len == SEED_DEFAULT && s->utime == 0;
        ok &= s->cond == SEED_NO_COND && s->rlen == SEED_NEW_RLEN && s->rrate == SEED_NEW_RRATE && s->rvel == 0;
        ok &= s->origin == SEED_NO_ORIGIN && s->lane[0] == o->lane[0] && s->lane[1] == o->lane[1];
    }
    for (i = 40; i < SEED_STEPS; i++)
        ok &= memcmp(&out.s[i], &orig.s[i], sizeof out.s[i]) == 0;
    CHECK(ok);
}

/* SHAPE = MAP: the level is the shape (VEL 1..127, AMT 100: 1 + 126 * level / 255, rounded), on
 * any source; X and Y from the context, the part from the recipe */
static void test_shape(void)
{
    int i, ok = 1, m = 0;
    base(32, 2, 255, 90, 10, 0);
    r.vel.lo = 1;
    r.vel.hi = 127;
    r.shape = SEED_SHAPE_MAP;
    r.amt = 100;
    CHECK(rendered());
    for (i = 0; i < 32; i++)
        if (out.s[i].flags & SEED_TRIG) {
            int lev = seed_map_level(2, 90, 10, i);
            ok &= out.s[i].vel == 1 + (126 * lev + 127) / 255;
            m++;
        }
    CHECK(m > 0);
    CHECK(ok);
    /* on SOURCE = TRACK */
    random_track(&orig, 16);
    for (i = 0; i < 16; i++)
        orig.s[i].flags |= SEED_TRIG;
    r.src = SEED_SRC_TRACK;
    CHECK(rendered());
    for (i = 0, ok = 1; i < 16; i++)
        ok &= out.s[i].vel == 1 + (126 * seed_map_level(2, 90, 10, i) + 127) / 255;
    CHECK(ok);
}

/* I2, I6 and I3 (props.h) over random recipes with SOURCE = MAP and every other built layer */
static void test_props(void)
{
    struct prop_rng g = { 2027 };
    struct seed_track o;
    struct seed_recipe pr;
    struct seed_ctx pc;
    int i, L, ok26 = 1, ok3 = 1;
    for (i = 0; i < 400; i++) {
        int len = prop_int(&g, 1, SEED_STEPS);
        prop_track(&o, &g, len, prop_int(&g, 0, 100));
        prop_ctx(&pc, &o, prop_int(&g, 0, 12));
        prop_recipe(&pr, &g, 50);
        pr.src = SEED_SRC_MAP;
        pr.map_part = (int16_t)prop_int(&g, 0, 2);
        pr.map_dens = (int16_t)prop_int(&g, 0, 255);
        pc.shared.map_x = (int16_t)prop_int(&g, 0, 255);
        pc.shared.map_y = (int16_t)prop_int(&g, 0, 255);
        pc.shared.map_chaos = (int16_t)(prop_chance(&g, 50) ? 0 : prop_int(&g, 0, 255));
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
    c = test_ctx(0);
    test_levels();
    test_args();
    test_dens();
    test_chaos();
    test_shared();
    test_step();
    test_shape();
    test_props();
    printf("test_src_map: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
