/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for seed_rng.c: slots (P1), keys and shares (P2, I4), determinism (I2), VARY (P3, I10). */
#include <stdio.h>
#include <string.h>
#include "seed.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static uint32_t lcg = 12345;
static uint32_t rnd(void)
{
    lcg = lcg * 1103515245u + 12345u;
    return lcg >> 8;
}

/* I2: the same take and slot give the same number; distinct slots, takes, salts and candidates differ */
static void test_determinism(void)
{
    struct seed_take a, b;
    int i, same = 1, diff_slot = 0, diff_take = 0, diff_salt = 0, diff_cand = 0;
    uint32_t s = SEED_SLOT(SEED_L_DYN, SEED_P_VEL, 0);
    for (i = 0; i < 1000; i++) {
        seed_take_init(&a, (uint32_t)i, 3, i % 101, 0);
        seed_take_init(&b, (uint32_t)i, 3, i % 101, 0);
        same &= seed_rand(&a, s + (uint32_t)i) == seed_rand(&b, s + (uint32_t)i);
        seed_take_init(&a, (uint32_t)i, 3, 0, 0);
        diff_slot += seed_rand(&a, s) != seed_rand(&a, s + 1);
        seed_take_init(&b, (uint32_t)i + 1, 3, 0, 0);
        diff_take += seed_rand(&a, s) != seed_rand(&b, s);
        seed_take_init(&b, (uint32_t)i, 4, 0, 0);
        diff_salt += seed_rand(&a, s) != seed_rand(&b, s);
        seed_take_init(&b, (uint32_t)i, 3, 0, 1);
        diff_cand += seed_rand(&a, s) != seed_rand(&b, s);
    }
    CHECK(same);
    CHECK(diff_slot == 1000 && diff_take == 1000 && diff_salt == 1000 && diff_cand == 1000);
    /* layers draw apart (P1): the same purpose and index under another layer is another number */
    seed_take_init(&a, 7, 0, 0, 0);
    CHECK(seed_rand(&a, SEED_SLOT(SEED_L_DYN, SEED_P_ACC, 5)) != seed_rand(&a, SEED_SLOT(SEED_L_TRIG, SEED_P_ACC, 5)));
}

static void test_range(void)
{
    struct seed_take k;
    int i, lo = 1000, hi = -1000, hist[7] = { 0 }, even = 1;
    seed_take_init(&k, 42, 0, 0, 0);
    for (i = 0; i < 7000; i++) {
        int v = seed_rand_range(&k, (uint32_t)i, -3, 3);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
        if (v >= -3 && v <= 3)
            hist[v + 3]++;
    }
    CHECK(lo == -3 && hi == 3);
    for (i = 0; i < 7; i++)
        even &= hist[i] > 850 && hist[i] < 1150;
    CHECK(even);
    CHECK(seed_rand_range(&k, 1, 5, 5) == 5);
    i = seed_rand_range(&k, 2, 9, 2);
    CHECK(i >= 2 && i <= 9);
}

static void test_share(void)
{
    CHECK(seed_share(0, 16) == 0);
    CHECK(seed_share(50, 16) == 8);
    CHECK(seed_share(100, 16) == 16);
    CHECK(seed_share(33, 3) == 1);
    CHECK(seed_share(-5, 16) == 0 && seed_share(150, 16) == 16);
    CHECK(seed_share(50, 0) == 0);
}

/* keys: BIAS 100 is the rank order, -100 its reverse, ties to the earlier step; keys are distinct */
static void test_keys(void)
{
    struct seed_take k;
    uint32_t key[64];
    uint16_t rank[64];
    uint8_t order[64];
    int i, j, m, ok = 1, distinct = 1;
    for (i = 0; i < 64; i++)
        rank[i] = (uint16_t)((i * 37) % 64 * 1000);       /* distinct ranks */
    seed_take_init(&k, 9, 0, 0, 0);
    seed_keys(key, 64, &k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, 0), rank, 100);
    m = seed_order(order, key, NULL, 64);
    CHECK(m == 64);
    for (i = 1; i < 64; i++)
        ok &= rank[order[i - 1]] > rank[order[i]];
    CHECK(ok);
    seed_keys(key, 64, &k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, 0), rank, -100);
    seed_order(order, key, NULL, 64);
    for (ok = 1, i = 1; i < 64; i++)
        ok &= rank[order[i - 1]] < rank[order[i]];
    CHECK(ok);
    /* equal ranks at BIAS 100: earlier step first */
    for (i = 0; i < 64; i++)
        rank[i] = 500;
    seed_keys(key, 64, &k, 0, rank, 100);
    seed_order(order, key, NULL, 64);
    for (ok = 1, i = 0; i < 64; i++)
        ok &= order[i] == i;
    CHECK(ok);
    /* BIAS 0 ignores the rank: the same keys as no rank at all */
    {
        uint32_t k2[64];
        seed_keys(key, 64, &k, 77, rank, 0);
        seed_keys(k2, 64, &k, 77, NULL, 50);
        CHECK(memcmp(key, k2, sizeof key) == 0);
    }
    for (i = 0; i < 64; i++)
        for (j = i + 1; j < 64; j++)
            distinct &= key[i] != key[j];
    CHECK(distinct);
}

/* I4: raising a share keeps every step it had chosen, for any keys, bias and eligibility */
static void test_monotonic(void)
{
    int trial, ok = 1, counts = 1;
    for (trial = 0; trial < 2000; trial++) {
        struct seed_take k;
        uint32_t key[64];
        uint16_t rank[64];
        uint8_t elig[64], order[64], prev[64], cur[64];
        int n = 1 + (int)(rnd() % 64), bias = (int)(rnd() % 201) - 100, i, m, pct;
        for (i = 0; i < n; i++) {
            rank[i] = (uint16_t)(rnd() % 5 * 16383);
            elig[i] = (uint8_t)(rnd() % 4 != 0);
        }
        seed_take_init(&k, rnd(), rnd() % 13, (int)(rnd() % 101), 0);
        seed_keys(key, n, &k, SEED_SLOT(SEED_L_TRIG, SEED_P_COND_KEY, 0), rank, bias);
        m = seed_order(order, key, elig, n);
        memset(prev, 0, sizeof prev);
        for (pct = 0; pct <= 100; pct++) {
            int c = seed_share(pct, m), got = 0;
            memset(cur, 0, sizeof cur);
            for (i = 0; i < c; i++)
                cur[order[i]] = 1;
            for (i = 0; i < n; i++) {
                ok &= !prev[i] || cur[i];       /* nothing chosen is dropped */
                ok &= !cur[i] || elig[i];       /* only eligible steps */
                got += cur[i];
            }
            counts &= got == c;                 /* the exact count */
            memcpy(prev, cur, sizeof prev);
        }
    }
    CHECK(ok);
    CHECK(counts);
}

/* the share of slots whose number take n + 1 keeps from take n, in % */
static int kept(int vary, uint32_t from, int takes)
{
    int s, t, same = 0, all = 0;
    for (t = 0; t < takes; t++) {
        struct seed_take a, b;
        seed_take_init(&a, from + (uint32_t)t, 1, vary, 0);
        seed_take_init(&b, from + (uint32_t)t + 1, 1, vary, 0);
        for (s = 0; s < 512; s++, all++)
            same += seed_rand(&a, SEED_SLOT(SEED_L_DYN, SEED_P_VEL, s)) == seed_rand(&b, SEED_SLOT(SEED_L_DYN, SEED_P_VEL, s));
    }
    return (int)((same * 1000L / all + 5) / 10);
}

/* I10: VARY 0 independent, 100 identical, between: the kept share matches VARY */
static void test_vary(void)
{
    static const int v[] = { 10, 25, 50, 75, 90 };
    int i;
    CHECK(kept(0, 100, 64) == 0);
    CHECK(kept(100, 100, 64) == 100);
    CHECK(kept(100, 0xfffffff0u, 64) == 100);       /* across the wrap of take numbers */
    for (i = 0; i < 5; i++) {
        int got = kept(v[i], 1000, 64);
        CHECK(got >= v[i] - 2 && got <= v[i] + 2);
        if (got < v[i] - 2 || got > v[i] + 2)
            printf("  VARY %d kept %d %%\n", v[i], got);
    }
    /* two takes apart a slot is kept when neither take re-rolled it: VARY^2, 25 % at 50 */
    {
        int s, same = 0;
        for (s = 0; s < 4096; s++) {
            struct seed_take a, b;
            seed_take_init(&a, 500, 1, 50, 0);
            seed_take_init(&b, 502, 1, 50, 0);
            same += seed_rand(&a, (uint32_t)s) == seed_rand(&b, (uint32_t)s);
        }
        CHECK(same * 100 / 4096 >= 22 && same * 100 / 4096 <= 28);
    }
    /* VARY's keys: at 50 % about half of a 16-step DENS 50 % pattern stays */
    {
        struct seed_take a, b;
        uint32_t ka[16], kb[16];
        uint8_t oa[16], ob[16], in[16];
        int t, i2, stay = 0, tot = 0;
        for (t = 0; t < 200; t++) {
            seed_take_init(&a, (uint32_t)t, 0, 100, 0);
            seed_take_init(&b, (uint32_t)t + 1, 0, 100, 0);
            seed_keys(ka, 16, &a, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, 0), NULL, 0);
            seed_keys(kb, 16, &b, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, 0), NULL, 0);
            seed_order(oa, ka, NULL, 16);
            seed_order(ob, kb, NULL, 16);
            memset(in, 0, sizeof in);
            for (i2 = 0; i2 < 8; i2++)
                in[oa[i2]] = 1;
            for (i2 = 0; i2 < 8; i2++, tot++)
                stay += in[ob[i2]];
        }
        CHECK(stay == tot);                     /* VARY 100: the same pattern */
    }
}

int main(void)
{
    test_determinism();
    test_range();
    test_share();
    test_keys();
    test_monotonic();
    test_vary();
    printf("test_seed_rng: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
