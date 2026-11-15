/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for src_random.c: GRID, DENS, BIAS on the five-level weights, what a generated step
 * holds, and the refusals while meter.c (F1) is not built. */
#include "tests.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(int len)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_RANDOM;
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

static void test_grid_dens(void)
{
    char p[SEED_STEPS + 1];
    base(16);
    r.grid = 4;
    r.dens = 100;
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "x...x...x...x...") == 0);
    r.dens = 0;
    CHECK(rendered() && ntrigs(&out) == 0);
    r.grid = 3;                                             /* not a grid: 1 */
    r.dens = 50;
    CHECK(rendered() && ntrigs(&out) == 8);
    base(13);
    r.grid = 2;
    r.dens = 50;                                            /* 7 positions: round(3.5) = 4 */
    CHECK(rendered() && ntrigs(&out) == 4);
    printf("dens 50, grid 1        ");
    base(16);
    CHECK(rendered());
    pattern(p);
    printf("%s\n", p);
}

static void test_bias(void)
{
    char p[SEED_STEPS + 1];
    int i, on_beats[3] = { 0 }, b;
    base(16);
    r.bias = 100;                                           /* the pure rank order: bar, half bar, beats */
    r.dens = 25;
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "x...x...x...x...") == 0);
    r.bias = -100;                                          /* the weakest steps (F1: Barlow's 1, 9, 5, 13) */
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, seed_have_rank ? ".x...x...x...x.." : ".x.x.x.x........") == 0);
    r.dens = 50;
    r.bias = 100;
    r.beat = 2;                                             /* a bar of 8 steps */
    CHECK(rendered());
    pattern(p);
    CHECK(strcmp(p, "x.x.x.x.x.x.x.x.") == 0);
    /* in between, a lean: trigs on beats out of 1200 */
    for (b = 0; b < 3; b++)
        for (i = 0; i < 300; i++) {
            int j;
            base(16);
            r.seed = (uint32_t)i;
            r.dens = 25;
            r.bias = (int16_t)(b == 0 ? 0 : b == 1 ? 60 : -60);
            CHECK(rendered());
            for (j = 0; j < 16; j += 4)
                on_beats[b] += (out.s[j].flags & SEED_TRIG) != 0;
        }
    printf("beats/1200 trigs       bias 0 %d, +60 %d, -60 %d\n", on_beats[0], on_beats[1], on_beats[2]);
    CHECK(on_beats[0] > 200 && on_beats[0] < 400);
    CHECK(on_beats[1] > on_beats[0] + 200);
    CHECK(on_beats[2] < on_beats[0] - 100);
}

static void test_step(void)
{
    int i, ok = 1;
    base(32);
    r.dens = 60;
    CHECK(rendered());
    for (i = 0; i < 32; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        ok &= (s->flags & ~SEED_TRIG) == (o->flags & ~SEED_KNOWN);     /* swing and unknown bits stay */
        ok &= s->vel == SEED_DEFAULT && s->note == SEED_DEFAULT && s->len == SEED_DEFAULT && s->utime == 0;
        ok &= s->cond == SEED_NO_COND && s->rlen == SEED_NEW_RLEN && s->rrate == SEED_NEW_RRATE && s->rvel == 0;
        ok &= s->origin == SEED_NO_ORIGIN && s->lane[0] == o->lane[0] && s->lane[1] == o->lane[1];
    }
    CHECK(ok);
}

static void test_seeds(void)
{
    struct seed_track a;
    int i, differ = 0, equal = 1;
    for (i = 0; i < 50; i++) {
        base(64);
        r.seed = (uint32_t)i;
        CHECK(rendered());
        a = out;
        r.seed = (uint32_t)i + 1;
        CHECK(rendered());
        differ += memcmp(&a, &out, sizeof a) != 0;
        r.vary = 100;                                       /* VARY 100: every take is equal */
        CHECK(rendered());
        a = out;
        r.seed = (uint32_t)i + 7;
        CHECK(rendered());
        equal &= memcmp(&a, &out, sizeof a) == 0;
    }
    CHECK(differ == 50);
    CHECK(equal);
    c.track = 1;                                            /* another track, another take */
    base(64);
    CHECK(rendered());
    a = out;
    c.track = 0;
    CHECK(rendered());
    CHECK(memcmp(&a, &out, sizeof a) != 0);
}

static void test_refusals(void)
{
    base(16);
    r.role = SEED_ROLE_KICK;                                /* the rank table comes with F1 */
    CHECK(rendered());                                      /* without BIAS no rank is needed */
    r.bias = 30;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_rank ? SEED_OK : SEED_E_NA));
    r.role = SEED_ROLE_METER;
    r.metr = SEED_METR_7_8;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == (seed_have_rank ? SEED_OK : SEED_E_NA));
    r.metr = SEED_METR_4_4;
    CHECK(rendered());
    r.role = SEED_ROLE_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_grid_dens();
    test_bias();
    test_step();
    test_seeds();
    test_refusals();
    printf("test_src_random: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
