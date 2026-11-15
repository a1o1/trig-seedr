/* SPDX-License-Identifier: GPL-2.0-or-later */
/* src_random: the RANDOM source: GRID, DENS, BIAS, ROLE.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * Each step gets a key from rank and chance (P2, seed_keys); the grid positions with the top
 * DENS % of keys get a trig, exactly round(DENS * positions / 100) of them (I5). Raising DENS
 * adds to the trigs there were (I4). The rank is meter.c's (F1). Freestanding. */
#include "seed.h"

const uint8_t seed_have_random = 1;

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* the step's rank, 0..SEED_RANK_MAX (meter.c, F1), or < 0 */
static int rank_of(const struct seed_recipe *r, int step, int n)
{
    return seed_rank(r->metr, r->beat, r->role, step, n);
}

int seed_src_random(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    uint16_t rank[SEED_STEPS];
    uint32_t key[SEED_STEPS];
    uint8_t elig[SEED_STEPS], order[SEED_STEPS];
    int n = t->length, bias = clamp(r->bias, -100, 100), grid, i, m;
    (void)c;

    if (r->role < 0 || r->role >= SEED_ROLE_N || r->metr < 0 || r->metr >= SEED_METR_N)
        return SEED_E_ARG;
    grid = r->grid == 2 || r->grid == 4 || r->grid == 8 ? r->grid : 1;
    if (bias)
        for (i = 0; i < n; i++) {
            int v = rank_of(r, i, n);
            if (v < 0)
                return v;
            rank[i] = (uint16_t)v;
        }
    for (i = 0; i < n; i++)
        elig[i] = i % grid == 0;
    seed_keys(key, n, k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_DENS, 0), bias ? rank : 0, bias);
    m = seed_share(r->dens, seed_order(order, key, elig, n));
    for (i = 0; i < m; i++)
        t->s[order[i]].flags |= SEED_TRIG;
    return SEED_OK;
}
