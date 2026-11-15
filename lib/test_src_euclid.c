/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for src_euclid.c: the patterns against Bjorklund's algorithm (the same necklace, and
 * the stock Euclidean mode's start on step 1), counts, even spread, repetition over a longer track,
 * clamping, what a generated step holds, and independence from the seed. The emulator check that
 * these are the Rytm's own Euclidean patterns is tools/tests/22-euclid.sh. */
#include "tests.h"
#include "props.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(int len, int steps, int puls)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_EUCLID;
    r.eu_steps = (int16_t)steps;
    r.eu_puls = (int16_t)puls;
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

static void pattern(char *p)
{
    int i;
    for (i = 0; i < out.length; i++)
        p[i] = out.s[i].flags & SEED_TRIG ? 'x' : '.';
    p[i] = 0;
}

/* Bjorklund's algorithm (Toussaint, "The Euclidean algorithm generates traditional musical
 * rhythms"), written independently of src_euclid.c: k pulses in n steps as 'x' and '.' */
static void bjorklund(char *p, int k, int n)
{
    char seq[SEED_STEPS][SEED_STEPS + 1];
    int cnt = n, a, b, i;
    for (i = 0; i < n; i++) {
        seq[i][0] = i < k ? 'x' : '.';
        seq[i][1] = 0;
    }
    if (k == 0 || k == n) {
        for (i = 0; i < n; i++)
            p[i] = seq[i][0];
        p[n] = 0;
        return;
    }
    a = k;                                                  /* the front groups */
    b = n - k;                                              /* the remainder groups */
    while (b > 1) {
        int m = a < b ? a : b;
        for (i = 0; i < m; i++)                             /* append a remainder group to each of m front groups */
            strcat(seq[i], seq[cnt - 1 - i]);
        cnt -= m;
        if (a > b) {                                        /* the unpaired fronts become the remainder */
            b = a - m;
            a = m;
        } else {
            b = b - m;
            a = m;
        }
    }
    p[0] = 0;
    for (i = 0; i < cnt; i++)
        strcat(p, seq[i]);
}

/* q is a rotation of p */
static int rotation_of(const char *p, const char *q, int n)
{
    int s, i;
    for (s = 0; s < n; s++) {
        for (i = 0; i < n && p[i] == q[(i + s) % n]; i++)
            ;
        if (i == n)
            return 1;
    }
    return 0;
}

static void test_known(void)
{
    char p[SEED_STEPS + 1];
    static const struct { int n, k; const char *want; } t[] = {
        /* as the Rytm's Euclidean mode shows them (LEN, PL1), checked in the emulator */
        { 16, 5, "x...x..x..x..x.." },
        { 16, 7, "x..x.x.x..x.x.x." },
        { 16, 9, "x.x.x.x.xx.x.x.x" },
        { 16, 11, "x.xx.xx.xx.xx.xx" },
        { 16, 4, "x...x...x...x..." },
        { 8, 3, "x..x..x." },
        { 16, 0, "................" },
        { 4, 4, "xxxx" },
    };
    unsigned i;
    for (i = 0; i < sizeof t / sizeof t[0]; i++) {
        base(t[i].n, t[i].n, t[i].k);
        CHECK(rendered());
        pattern(p);
        CHECK(strcmp(p, t[i].want) == 0);
    }
}

/* every STEPS 1..64 and PULS 0..STEPS: Bjorklund's necklace, the exact count, a pulse on the first
 * step, gaps that differ by at most one */
static void test_all(void)
{
    char p[SEED_STEPS + 1], q[SEED_STEPS + 1];
    int n, k, i, ok_rot = 1, ok_count = 1, ok_first = 1, ok_even = 1;
    for (n = 1; n <= SEED_STEPS; n++)
        for (k = 0; k <= n; k++) {
            int gmin = SEED_STEPS, gmax = 0, last = -1, first = -1;
            base(n, n, k);
            CHECK(rendered());
            pattern(p);
            bjorklund(q, k, n);
            ok_rot &= rotation_of(p, q, n);
            ok_count &= ntrigs(&out) == k;
            ok_first &= k == 0 || p[0] == 'x';
            for (i = 0; i < n; i++)
                if (p[i] == 'x') {
                    if (last >= 0) {
                        gmin = i - last < gmin ? i - last : gmin;
                        gmax = i - last > gmax ? i - last : gmax;
                    } else
                        first = i;
                    last = i;
                }
            if (k > 1) {                                    /* the gap that wraps round */
                int g = n - last + first;
                gmin = g < gmin ? g : gmin;
                gmax = g > gmax ? g : gmax;
                ok_even &= gmax - gmin <= 1;
            }
        }
    CHECK(ok_rot);
    CHECK(ok_count);
    CHECK(ok_first);
    CHECK(ok_even);
}

/* a track longer than STEPS repeats the cycle; a shorter one plays its start */
static void test_cycle(void)
{
    char p[SEED_STEPS + 1];
    base(64, 12, 5);
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "x..x.x..x.x.x..x.x..x.x.x..x.x..x.x.x..x.x..x.x.x..x.x..x.x.x..x") == 0);
    CHECK(ntrigs(&out) == 5 * 5 + 2);                       /* 5 cycles and steps 60..63 */
    base(10, 16, 5);
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "x...x..x..") == 0);
}

/* out of range: STEPS to 1..64, PULS to 0..STEPS */
static void test_clamp(void)
{
    char p[SEED_STEPS + 1];
    base(8, 8, 20);
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "xxxxxxxx") == 0);
    base(8, 8, -3);
    CHECK(rendered() && ntrigs(&out) == 0);
    base(8, 0, 1);                                          /* STEPS 1: every step */
    CHECK(rendered() && ntrigs(&out) == 8);
    base(64, 100, 3);                                       /* STEPS 64 */
    CHECK(rendered() && ntrigs(&out) == 3);
}

/* a generated step holds the defaults (as RANDOM's); steps past the length are untouched (I6) */
static void test_step(void)
{
    int i, ok = 1;
    base(32, 16, 7);
    CHECK(rendered());
    for (i = 0; i < 32; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        ok &= (s->flags & ~SEED_TRIG) == (o->flags & ~SEED_KNOWN);     /* swing and unknown bits stay */
        ok &= s->vel == SEED_DEFAULT && s->note == SEED_DEFAULT && s->len == SEED_DEFAULT && s->utime == 0;
        ok &= s->cond == SEED_NO_COND && s->rlen == SEED_NEW_RLEN && s->rrate == SEED_NEW_RRATE && s->rvel == 0;
        ok &= s->origin == SEED_NO_ORIGIN && s->lane[0] == o->lane[0] && s->lane[1] == o->lane[1];
    }
    for (i = 32; i < SEED_STEPS; i++)
        ok &= memcmp(&out.s[i], &orig.s[i], sizeof out.s[i]) == 0;
    CHECK(ok);
}

/* no random numbers: seeds, VARY and tracks give the same rhythm (I2) */
static void test_seeds(void)
{
    struct seed_track a;
    int i, equal = 1;
    base(64, 13, 6);
    CHECK(rendered());
    a = out;
    for (i = 0; i < 20; i++) {
        r.seed = (uint32_t)i * 77u;
        r.vary = (int16_t)(i * 5);
        c.track = i % 12;
        CHECK(rendered());
        equal &= memcmp(&a, &out, sizeof a) == 0;
    }
    c.track = 0;
    CHECK(equal);
}

/* with ROT, the rotation of the same pattern (PLACE is a plain rotation after the source) */
static void test_rot(void)
{
    char p[SEED_STEPS + 1];
    base(16, 16, 5);
    r.rot = 2;
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "..x...x..x..x..x") == 0);
}

/* I2, I6 and I3 (props.h) over random recipes with SOURCE = EUCLID and every other built layer */
static void test_props(void)
{
    struct prop_rng g = { 2026 };
    struct seed_track o;
    struct seed_recipe pr;
    struct seed_ctx pc;
    int i, L, ok26 = 1, ok3 = 1;
    for (i = 0; i < 400; i++) {
        int len = prop_int(&g, 1, SEED_STEPS);
        prop_track(&o, &g, len, prop_int(&g, 0, 100));
        prop_ctx(&pc, &o, prop_int(&g, 0, 12));
        prop_recipe(&pr, &g, 50);
        pr.src = SEED_SRC_EUCLID;
        pr.eu_steps = (int16_t)prop_int(&g, 1, SEED_STEPS);
        pr.eu_puls = (int16_t)prop_int(&g, 0, pr.eu_steps);
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
    test_known();
    test_all();
    test_cycle();
    test_clamp();
    test_step();
    test_seeds();
    test_rot();
    test_props();
    printf("test_src_euclid: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
