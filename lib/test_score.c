/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for score.c (F11): QUAL. The score's terms in order (count kept, largest gap,
 * adjacent trigs, fit to the meter in BIAS's direction); no pull towards strong or weak steps at
 * BIAS 0 (a rotated take scores the same); ties to the lower candidate, so a source that draws no
 * rhythm gives QUAL 1's take; the winner's dressing is the full chain's (best() dresses the winner
 * only); I2, I3 and I6 at QUAL > 1; how QUAL changes the takes; and the share VARY keeps between
 * the takes shown at QUAL 2, 4 and 8 (design P3, I10). */
#include "tests.h"

static void clear(struct seed_track *t, int len)
{
    memset(t, 0, sizeof *t);
    t->length = (int16_t)len;
    t->default_note = 60;
    t->default_vel = 100;
}

/* a track with trigs where pat has 'x' */
static void pattern(struct seed_track *t, const char *pat)
{
    int i;
    clear(t, (int)strlen(pat));
    for (i = 0; pat[i]; i++)
        t->s[i].flags = pat[i] == 'x' ? SEED_TRIG : 0;
}

static int score_of(const char *pat, const struct seed_recipe *r)
{
    struct seed_track t;
    struct seed_ctx c = test_ctx(0);
    pattern(&t, pat);
    return seed_score(&t, r, &c);
}

#define TERM_C(s) ((s) >> 24)
#define TERM_G(s) ((s) >> 17 & 127)
#define TERM_A(s) ((s) >> 10 & 127)
#define TERM_F(s) ((s) & 1023)

static void test_terms(void)
{
    struct seed_recipe r;
    int s;
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_RANDOM;
    r.dens = 25;                                            /* asks for 4 of 16 */
    s = score_of("x...x...x...x...", &r);
    CHECK(TERM_C(s) == 64 && TERM_G(s) == 64 - 4 && TERM_A(s) == 64 && TERM_F(s) == 0);
    s = score_of("xx......x.......", &r);                   /* 3 trigs: one short of DENS */
    CHECK(TERM_C(s) == 63 && TERM_G(s) == 64 - 8 && TERM_A(s) == 63);
    s = score_of("x..............x", &r);                   /* adjacent across the wrap */
    CHECK(TERM_C(s) == 62 && TERM_G(s) == 64 - 15 && TERM_A(s) == 63);
    s = score_of("................", &r);
    CHECK(s == (60 << 24 | 0 << 17 | 64 << 10));
    s = score_of(".......x........", &r);                   /* one trig: a gap of the length, no neighbour */
    CHECK(TERM_G(s) == 64 - 16 && TERM_A(s) == 64);
    /* order: count first, then the largest gap, then adjacent trigs */
    CHECK(score_of("x...x...x...x...", &r) > score_of("x..x..x..x......", &r));    /* gap 4 vs 7 */
    CHECK(score_of("x..x..x..x......", &r) > score_of("x.x.x.x.........", &r));    /* gap 7 vs 10 */
    CHECK(score_of("xx......x.....x.", &r) > score_of("xx...x..x.......", &r));    /* gap 7 vs 8 */
    CHECK(score_of("x..x....x...x...", &r) > score_of("x..xx...x...x...", &r));    /* C: 4 vs 5 trigs */
    CHECK(score_of("x...x..xx.......", &r) > score_of("x..xx..xx.......", &r));    /* C: 4 vs 5 trigs */
    CHECK(score_of("x..xx...x..x....", &r) > score_of("x..xx..xx.......", &r));
    /* GRID 2: neighbours are a grid step apart */
    r.grid = 2;
    r.dens = 50;                                            /* 4 of 8 positions */
    s = score_of("x.x...x.x.......", &r);
    CHECK(TERM_C(s) == 64 && TERM_G(s) == 64 - 8 && TERM_A(s) == 64 - 2);
    /* fit: only with a BIAS, in its direction, after every other term */
    r.grid = 1;
    r.dens = 25;
    CHECK(TERM_F(score_of("x...x...x...x...", &r)) == 0);
    r.bias = 50;
    CHECK(TERM_F(score_of("x...x...x...x...", &r)) > TERM_F(score_of(".x...x...x...x..", &r)));
    CHECK(TERM_F(score_of("x...x...x...x...", &r)) > 800);                       /* 4/4's top four ranks */
    r.bias = -50;
    CHECK(TERM_F(score_of("x...x...x...x...", &r)) < TERM_F(score_of(".x...x...x...x..", &r)));
    CHECK(score_of("x...x...x...x...", &r) > score_of("x..x....x...x...", &r));    /* spread before fit */
    /* other sources: no count term, no fit, grid 1 */
    r.src = SEED_SRC_EUCLID;
    s = score_of("xx......", &r);
    CHECK(TERM_C(s) == 64 && TERM_G(s) == 64 - 7 && TERM_A(s) == 63 && TERM_F(s) == 0);
    /* only trigs before the length count */
    r.src = SEED_SRC_TRACK;
    {
        struct seed_track t;
        struct seed_ctx c = test_ctx(0);
        pattern(&t, "x...x...");
        t.s[9].flags = SEED_TRIG;
        t.s[0].vel = 3;                                     /* fields are not read */
        t.s[4].cond = 12;
        CHECK(seed_score(&t, &r, &c) == score_of("x...x...", &r));
    }
}

/* at BIAS 0 the score does not change when the take is rotated: no pull towards strong or weak
 * steps. With a BIAS the fit term follows it. */
static void test_rotation(void)
{
    struct seed_track t, u;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int n, i, k, same = 1;
    for (n = 0; n < 300; n++) {
        int len = rnd(1, 64);
        seed_recipe_defaults(&r, &test_caps);
        random_rhythm(&r);
        r.bias = 0;
        r.metr = (int16_t)rnd(0, SEED_METR_N - 1);
        r.role = (int16_t)rnd(0, SEED_ROLE_N - 1);
        clear(&t, len);
        for (i = 0; i < len; i++)
            t.s[i].flags = rnd(0, 2) == 0 ? SEED_TRIG : 0;
        k = rnd(1, 63);
        clear(&u, len);
        for (i = 0; i < len; i++)
            u.s[(i + k) % len] = t.s[i];
        same &= seed_score(&t, &r, &c) == seed_score(&u, &r, &c);
    }
    CHECK(same);
}

static int same_trigs(const struct seed_track *a, const struct seed_track *b)
{
    int i;
    for (i = 0; i < SEED_STEPS; i++)
        if ((a->s[i].flags ^ b->s[i].flags) & SEED_TRIG)
            return 0;
    return a->length == b->length;
}

/* QUAL > 1 with a source that draws no rhythm: every candidate ties, candidate 0 wins, the take is
 * QUAL 1's; and I2 */
static void test_ties(void)
{
    struct seed_track orig, a, b;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int n, same = 1, det = 1;
    for (n = 0; n < 200; n++) {
        int len = rnd(1, 64);
        random_track(&orig, len);
        c.length[0] = (uint8_t)len;
        random_recipe(&r);
        if (n % 2 == 0)
            r.src = SEED_SRC_TRACK;
        else if (n % 4 == 1) {
            r.src = SEED_SRC_EUCLID;
            r.eu_steps = (int16_t)rnd(1, 64);
            r.eu_puls = (int16_t)rnd(0, r.eu_steps);
        } else {
            r.src = SEED_SRC_RANDOM;
            r.bias = n % 8 == 3 ? 100 : -100;               /* the pure rank order */
        }
        r.qual = 1;
        CHECK(seed_render(&a, &orig, &r, &c, &work) == SEED_OK);
        r.qual = (int16_t)rnd(2, 8);
        CHECK(seed_render(&b, &orig, &r, &c, &work) == SEED_OK);
        same &= memcmp(&a, &b, sizeof a) == 0;
        CHECK(seed_render(&a, &orig, &r, &c, &work) == SEED_OK);
        det &= memcmp(&a, &b, sizeof a) == 0;
    }
    CHECK(same);
    CHECK(det);
}

/* best() scores candidates at the end of PLACE and dresses only the winner. The result must be the
 * full chain on the winner's trigs: the winner's positions (every dressing layer off) dressed by
 * SOURCE = TRACK with PLACE off give the same fields. Also I3 for QUAL: dressing knobs do not
 * change where the trigs are. */
static void test_dress(void)
{
    struct seed_track orig, full, pos, again;
    struct seed_recipe r, bare, dressed;
    struct seed_ctx c = test_ctx(0);
    int n, i, ok = 1, fields = 1;
    for (n = 0; n < 400; n++) {
        int len = rnd(1, 64);
        random_track(&orig, len);
        c.length[0] = (uint8_t)len;
        random_recipe(&r);
        r.src = SEED_SRC_RANDOM;
        r.dens = (int16_t)rnd(5, 95);
        r.qual = (int16_t)rnd(2, 8);
        CHECK(seed_render(&full, &orig, &r, &c, &work) == SEED_OK);
        /* the same RHYTHM, PLACE and TAKE, every dressing layer at 0 */
        seed_recipe_defaults(&bare, &test_caps);
        bare.seed = r.seed;
        bare.vary = r.vary;
        bare.qual = r.qual;
        bare.src = r.src;
        bare.grid = r.grid;
        bare.dens = r.dens;
        bare.bias = r.bias;
        bare.beat = r.beat;
        bare.rot = r.rot;
        bare.skip = r.skip;
        CHECK(seed_render(&pos, &orig, &bare, &c, &work) == SEED_OK);
        ok &= same_trigs(&full, &pos);
        /* dress the winner's positions as SOURCE = TRACK with candidate 0's numbers */
        dressed = r;
        dressed.src = SEED_SRC_TRACK;
        dressed.rot = dressed.skip = 0;
        dressed.qual = 1;
        CHECK(seed_render(&again, &pos, &dressed, &c, &work) == SEED_OK);
        for (i = 0; i < SEED_STEPS; i++) {
            struct seed_step x = full.s[i], y = again.s[i];
            x.origin = y.origin = 0;
            fields &= memcmp(&x, &y, sizeof x) == 0;
        }
    }
    CHECK(ok);
    CHECK(fields);
}

/* I6 at QUAL > 1: steps at and past the length are the original's */
static void test_length(void)
{
    struct seed_track orig, out;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int n, i, ok = 1;
    for (n = 0; n < 200; n++) {
        int len = rnd(1, 63);
        random_track(&orig, len);
        c.length[0] = (uint8_t)len;
        random_recipe(&r);
        r.qual = (int16_t)rnd(2, 8);
        CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
        for (i = len; i < SEED_STEPS; i++)
            ok &= memcmp(&out.s[i], &orig.s[i], sizeof out.s[i]) == 0;
    }
    CHECK(ok);
}

/* What QUAL does, over 1,000 takes of RANDOM on 16 steps: the largest gap, adjacent trigs, trigs
 * on the beats. At BIAS 0 the trigs on the beats stay as they were (no pull); with BIAS 60 they
 * rise. Raising QUAL never lowers the take's score. */
static void test_effect(void)
{
    static const int quals[4] = { 1, 2, 4, 8 };
    struct seed_track orig, out;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int b, d, q, n, i, gap[4], adj[4], beats[4], score_up = 1;
    clear(&orig, 16);
    for (b = 0; b < 2; b++)
        for (d = 0; d < 2; d++) {
            for (q = 0; q < 4; q++) {
                gap[q] = adj[q] = beats[q] = 0;
                for (n = 0; n < 1000; n++) {
                    int s;
                    seed_recipe_defaults(&r, &test_caps);
                    r.src = SEED_SRC_RANDOM;
                    r.dens = d ? 50 : 25;
                    r.bias = b ? 60 : 0;
                    r.seed = (uint32_t)n;
                    r.qual = (int16_t)quals[q];
                    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
                    s = seed_score(&out, &r, &c);
                    gap[q] += 64 - TERM_G(s);
                    adj[q] += 64 - TERM_A(s);
                    for (i = 0; i < 16; i += 4)
                        beats[q] += (out.s[i].flags & SEED_TRIG) != 0;
                    if (q) {
                        int t;
                        r.qual = (int16_t)quals[q - 1];
                        CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
                        t = seed_score(&out, &r, &c);
                        score_up &= s >= t;
                    }
                }
            }
            printf("  DENS %d, BIAS %2d, QUAL 1 2 4 8: largest gap %.2f %.2f %.2f %.2f, adjacent %.2f %.2f %.2f %.2f,"
                   " on the beats %d %d %d %d (of %d trigs)\n", d ? 50 : 25, b ? 60 : 0,
                   gap[0] / 1000.0, gap[1] / 1000.0, gap[2] / 1000.0, gap[3] / 1000.0, adj[0] / 1000.0,
                   adj[1] / 1000.0, adj[2] / 1000.0, adj[3] / 1000.0, beats[0], beats[1], beats[2], beats[3],
                   (d ? 8 : 4) * 1000);
            CHECK(gap[3] < gap[2] && gap[2] < gap[1] && gap[1] < gap[0]);
            if (!b) {
                /* a quarter of the trigs on the beats, as with QUAL 1, within 15 % (an even take of
                 * 4 trigs in 16 has all or none of them on the beats, so the count varies widely) */
                for (q = 0; q < 4; q++)
                    CHECK(beats[q] * 4 > (d ? 8000 : 4000) * 85 / 100 && beats[q] * 4 < (d ? 8000 : 4000) * 115 / 100);
            }
        }
    CHECK(score_up);
}

/* P3 and VARY at QUAL > 1. A take is the best of its candidates, each a function of n. VARY keeps
 * a share of candidate c's keys from take n to n + 1; between the takes shown, the winners may be
 * different candidates. Measured: the share of the RANDOM source's keys the winner of take n + 1
 * shares with the winner of take n, and how often the same candidate wins. */
static int winner(uint32_t n, int vary, int q, struct seed_track *out)
{
    struct seed_track t, best;
    struct seed_recipe r;
    struct seed_take k;
    struct seed_ctx c = test_ctx(0);
    int i, w = 0, top = -1;
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_RANDOM;
    r.dens = 50;
    for (i = 0; i < q; i++) {
        int s;
        clear(&t, 16);
        seed_take_init(&k, n, 0, vary, i);
        CHECK(seed_src_random(&t, &r, &k, &c) == SEED_OK);
        s = seed_score(&t, &r, &c);
        if (s > top) {
            top = s;
            w = i;
            best = t;
        }
    }
    if (out) {
        /* the render agrees */
        struct seed_track orig, got;
        clear(&orig, 16);
        r.seed = n;
        r.vary = (int16_t)vary;
        r.qual = (int16_t)q;
        CHECK(seed_render(&got, &orig, &r, &c, &work) == SEED_OK);
        CHECK(same_trigs(&got, &best));
        *out = best;
    }
    return w;
}

static void test_vary(void)
{
    static const int quals[4] = { 1, 2, 4, 8 }, varies[6] = { 0, 25, 50, 75, 90, 100 };
    int v, q, n, i;
    for (v = 0; v < 6; v++) {
        int share[4], won[4];
        for (q = 0; q < 4; q++) {
            struct seed_track shown;
            int kept = 0, same = 0, w0 = winner(1000, varies[v], quals[q], NULL);
            for (n = 0; n < 500; n++) {
                struct seed_take a, b;
                int w1 = winner(1001u + (uint32_t)n, varies[v], quals[q], n < 20 ? &shown : NULL);
                seed_take_init(&a, 1000u + (uint32_t)n, 0, varies[v], w0);
                seed_take_init(&b, 1001u + (uint32_t)n, 0, varies[v], w1);
                for (i = 0; i < 16; i++)
                    kept += seed_rand(&a, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, i)) ==
                            seed_rand(&b, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, i));
                same += w0 == w1;
                w0 = w1;
            }
            share[q] = kept * 100 / (500 * 16);
            won[q] = same * 100 / 500;
        }
        printf("  VARY %3d: keys kept between takes shown, QUAL 1 2 4 8: %3d %3d %3d %3d %%;"
               " the same candidate wins %3d %3d %3d %3d %%\n", varies[v], share[0], share[1], share[2], share[3],
               won[0], won[1], won[2], won[3]);
        /* QUAL 1 is I10; above it the share is at most VARY's and at least P(same winner) * VARY */
        CHECK(share[0] >= varies[v] - 3 && share[0] <= varies[v] + 3);
        for (q = 1; q < 4; q++) {
            CHECK(share[q] <= varies[v] + 3);
            CHECK(share[q] + 3 >= won[q] * varies[v] / 100);
        }
        if (varies[v] == 100)
            for (q = 0; q < 4; q++)
                CHECK(share[q] == 100 && won[q] == 100);
    }
}

int main(void)
{
    test_setup();
    test_terms();
    test_rotation();
    test_ties();
    test_dress();
    test_length();
    test_effect();
    test_vary();
    printf("test_score: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
