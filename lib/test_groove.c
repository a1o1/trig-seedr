/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for groove.c */
#include <stdio.h>
#include <string.h>
#include "groove.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void blank(struct gen_track *t, int len)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = len;
    t->default_note = 60;
    for (i = 0; i < GEN_STEPS; i++) {
        struct gen_step *s = &t->s[i];
        s->note = s->vel = s->len = GEN_DEFAULT;
        s->cond = GEN_NO_COND;
    }
}

static void all_trigs(struct gen_track *t, int vel)
{
    int i;
    for (i = 0; i < t->length; i++) {
        t->s[i].flags |= GEN_TRIG;
        t->s[i].vel = (uint8_t)vel;
    }
}

/* MPC-style swing on 16ths: every second step late by off, velocities 100 / 80 */
static void swing(struct gen_groove *g, int off)
{
    int i;
    memset(g, 0, sizeof *g);
    strcpy(g->name, "TEST");
    g->len = 2;
    g->vmax = 100;
    for (i = 0; i < GEN_STEPS; i++) {
        g->off[i] = (int8_t)(i & 1 ? off : 0);
        g->vel[i] = (uint8_t)(i & 1 ? 80 : 100);
    }
}

static struct gen_groove_params params(int timing, int velocity)
{
    struct gen_groove_params p;
    memset(&p, 0, sizeof p);
    p.timing = timing;
    p.velocity = velocity;
    p.seed = 1;
    return p;
}

static void test_timing(void)
{
    struct gen_track t;
    struct gen_groove g;
    struct gen_groove_params p = params(100, 0);
    swing(&g, 8);
    blank(&t, 16);
    all_trigs(&t, 100);
    CHECK(gen_groove_apply(&t, 0, 16, &g, &p, 100) == 8);
    CHECK(t.s[0].utime == 0 && t.s[1].utime == 8 && t.s[2].utime == 0 && t.s[15].utime == 8);
    CHECK(t.s[1].vel == 100);                          /* velocity 0 %: unchanged */
    /* half timing, rounded */
    blank(&t, 16); all_trigs(&t, 100);
    p.timing = 50; gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[1].utime == 4);
    p.timing = -100; blank(&t, 16); all_trigs(&t, 100); gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[1].utime == -8);
    /* added to the existing µtime, clamped to +-23 */
    blank(&t, 16); all_trigs(&t, 100); t.s[1].utime = 20; p.timing = 100;
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[1].utime == 23);
}

static void test_quant(void)
{
    struct gen_track t;
    struct gen_groove g;
    struct gen_groove_params p = params(0, 0);
    swing(&g, 8);
    blank(&t, 16); all_trigs(&t, 100);
    t.s[2].utime = -10; t.s[3].utime = 7;
    p.quant = 100;
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[2].utime == 0 && t.s[3].utime == 0);
    blank(&t, 16); all_trigs(&t, 100);
    t.s[2].utime = -10; p.quant = 50; p.timing = 100;
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[2].utime == -5);
    t.s[3].utime = 0;
}

static void test_velocity(void)
{
    struct gen_track t;
    struct gen_groove g;
    struct gen_groove_params p = params(0, 100);
    swing(&g, 8);
    blank(&t, 16); all_trigs(&t, 120);
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[0].vel == 120 && t.s[1].vel == 96);     /* 120 * 80/100 */
    p.velocity = 50; blank(&t, 16); all_trigs(&t, 120);
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[1].vel == 108);
    p.velocity = -100; blank(&t, 16); all_trigs(&t, 120);
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[1].vel == 127);                          /* 144, clamped */
    /* a trig at the default velocity takes the track default, written as a value */
    p.velocity = 100; blank(&t, 16); all_trigs(&t, GEN_DEFAULT);
    gen_groove_apply(&t, 0, 16, &g, &p, 110);
    CHECK(t.s[0].vel == 110 && t.s[1].vel == 88);
    /* groove step without a note: velocity unchanged */
    blank(&t, 16); all_trigs(&t, 120); g.vel[1] = 0;
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[1].vel == 120);
}

static void test_only_trigs_and_shift(void)
{
    struct gen_track t;
    struct gen_groove g;
    struct gen_groove_params p = params(100, 0);
    swing(&g, 8);
    blank(&t, 16);
    t.s[1].flags = GEN_SWING;                          /* no trig */
    t.s[3].flags = GEN_TRIG;
    CHECK(gen_groove_apply(&t, 0, 16, &g, &p, 100) == 1);
    CHECK(t.s[1].utime == 0 && t.s[3].utime == 8);
    p.shift = 1;
    blank(&t, 16); all_trigs(&t, 100);
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[0].utime == 8 && t.s[1].utime == 0);
    p.shift = -3;
    blank(&t, 16); all_trigs(&t, 100);
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[0].utime == 8 && t.s[1].utime == 0);
    /* a 3-step groove repeats */
    swing(&g, 0); g.len = 3; g.off[0] = 1; g.off[1] = 2; g.off[2] = 3; p.shift = 0;
    blank(&t, 16); all_trigs(&t, 100);
    gen_groove_apply(&t, 0, 16, &g, &p, 100);
    CHECK(t.s[3].utime == 1 && t.s[4].utime == 2 && t.s[14].utime == 3);
    /* region */
    blank(&t, 16); all_trigs(&t, 100);
    gen_groove_apply(&t, 4, 4, &g, &p, 100);
    CHECK(t.s[3].utime == 0 && t.s[4].utime == 2 && t.s[8].utime == 0);
}

static void test_random(void)
{
    struct gen_track a, b;
    struct gen_groove g;
    struct gen_groove_params p = params(0, 0);
    int i, nonzero = 0, ok = 1;
    swing(&g, 0);
    p.random = 100; p.seed = 7;
    blank(&a, 64); all_trigs(&a, 100);
    gen_groove_apply(&a, 0, 64, &g, &p, 100);
    for (i = 0; i < 64; i++) {
        nonzero += a.s[i].utime != 0;
        ok &= a.s[i].utime >= -GEN_GROOVE_RAND && a.s[i].utime <= GEN_GROOVE_RAND;
    }
    CHECK(ok && nonzero > 40);
    blank(&b, 64); all_trigs(&b, 100);
    gen_groove_apply(&b, 0, 64, &g, &p, 100);
    CHECK(memcmp(&a, &b, sizeof a) == 0);                /* same seed, same result */
    p.seed = 8; blank(&b, 64); all_trigs(&b, 100);
    gen_groove_apply(&b, 0, 64, &g, &p, 100);
    CHECK(memcmp(&a, &b, sizeof a) != 0);
}

static void test_bad(void)
{
    struct gen_track t;
    struct gen_groove g;
    struct gen_groove_params p = params(100, 100);
    swing(&g, 8);
    g.len = 0;
    blank(&t, 16); all_trigs(&t, 100);
    CHECK(gen_groove_apply(&t, 0, 16, &g, &p, 100) == 0);
    CHECK(gen_groove_apply(&t, 0, 16, 0, &p, 100) == 0);
    g.len = 2;
    CHECK(gen_groove_apply(&t, 60, 20, &g, &p, 100) >= 0);   /* region clipped to 64 */
}

int main(void)
{
    test_timing();
    test_quant();
    test_velocity();
    test_only_trigs_and_shift();
    test_random();
    test_bad();
    printf("groove: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
