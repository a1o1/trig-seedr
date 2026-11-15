/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for props.h, the property-test framework (task 0.5): its diff and its generators,
 * and I1, I2, I3 and I6 over random originals and recipes. The recipes use every layer that is
 * built (seed_have_*), so this covers more as workstreams land; layer owners prove I4 and I5 in
 * their own tests (test_props.c for 0.2). */
#include <stdio.h>
#include "props.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

#define CASES 400

static void test_diff(void)
{
    static struct seed_track a, b;
    struct prop_rng g = { 1 };
    prop_track(&a, &g, 16, 40);
    b = a;
    CHECK(prop_diff(&a, &b, 0) == 0);
    b.s[3].vel ^= 1;
    b.s[5].lane[1] = 7;
    b.s[20].flags ^= SEED_TRIG;                   /* past the length */
    CHECK(prop_diff(&a, &b, 16) == (PROP_VEL | PROP_LANE2));
    CHECK(prop_diff(&a, &b, 0) == (PROP_VEL | PROP_LANE2 | PROP_TRIG));
    b = a;
    b.s[0].flags ^= 0x0400u;                      /* a flag bit lib/ does not know */
    CHECK(prop_diff(&a, &b, 0) == PROP_XFLAG);
    b = a;
    b.s[1].flags ^= SEED_SWING;                   /* swing: no layer may change it */
    CHECK(prop_diff(&a, &b, 0) == PROP_OTHER);
}

/* the generators stay inside the ranges seed.h gives, and only turn on built layers */
static void test_generators(void)
{
    struct prop_rng g = { 7 };
    int k, i, bad = 0, active[SEED_L_N] = { 0 };
    for (k = 0; k < CASES; k++) {
        static struct seed_track t;
        struct seed_recipe r;
        prop_track(&t, &g, prop_int(&g, 1, SEED_STEPS), 50);
        for (i = 0; i < SEED_STEPS; i++)
            if (t.s[i].utime < -23 || t.s[i].utime > 23 || t.s[i].cond > 64 || t.s[i].origin != i)
                bad++;
        prop_recipe(&r, &g, 60);
        if (r.dens < 0 || r.dens > 100 || r.vary < 0 || r.vary > 100 || r.rot < -63 || r.rot > 63)
            bad++;
        for (i = 0; i < SEED_L_N; i++)
            active[i] += seed_layer_active(&r, (enum seed_layer)i);
    }
    CHECK(bad == 0);
    CHECK(!active[SEED_L_RHYTHM] == !seed_have_random);
    CHECK(!active[SEED_L_DYN] == !seed_have_dyn);
    CHECK(!active[SEED_L_TRIG] == !seed_have_trig);
    printf("props: layers active in %d random recipes:", CASES);
    for (i = 1; i < SEED_L_N; i++)
        printf(" %d", active[i]);
    printf(" (RHYTHM .. FIELDS)\n");
}

static void test_invariants(void)
{
    struct prop_rng g = { 2026 };
    int k, i1 = 0, i2 = 0, i3 = 0, L;
    unsigned seen[SEED_L_N] = { 0 };
    for (k = 0; k < CASES; k++) {
        static struct seed_track t;
        struct seed_recipe r;
        struct seed_ctx c;
        int track = prop_int(&g, 0, 12);
        prop_track(&t, &g, prop_int(&g, 1, SEED_STEPS), prop_int(&g, 0, 100));
        prop_ctx(&c, &t, track);
        i1 += !prop_i1(&t, &c, prop_next(&g), prop_int(&g, 0, 100));
        prop_recipe(&r, &g, 50);
        i2 += !prop_i2_i6(&t, &r, &c);
        for (L = SEED_L_PLACE; L < SEED_L_N; L++) {
            unsigned ch;
            i3 += !prop_i3(&t, &r, &c, &g, (enum seed_layer)L, &ch);
            seen[L] |= ch;
        }
    }
    CHECK(i1 == 0);
    CHECK(i2 == 0);
    CHECK(i3 == 0);
    printf("props: I1 %d, I2 + I6 %d, I3 %d failures in %d cases\n", i1, i2, i3, CASES);
}

int main(void)
{
    test_diff();
    test_generators();
    test_invariants();
    printf("test_props_frame: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
