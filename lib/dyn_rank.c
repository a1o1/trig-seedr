/* SPDX-License-Identifier: GPL-2.0-or-later */
/* dyn_rank: DYNAMICS: SHAPE = RANK, AMOD = TOP (flag: seed_have_rank in meter.c).
 * Owner: task F1 (docs/plan-v1.md section 4).
 * Both use the meter's Barlow ranks (ROLE = METER, whatever ROLE says: ROLE is a RHYTHM knob, and
 * DYNAMICS changes only its own fields, I3). SHAPE = RANK: a step's level is its indispensability
 * in the bar, spread evenly over 0..255 (4/4: 16 levels, the bar's first step loudest). AMOD = TOP:
 * the ACC % highest-ranked trigs get the accent flag, the others lose it; ties between bars go to
 * the bars' Barlow order. Freestanding. */
#include "seed.h"

int seed_shape_rank(const struct seed_recipe *r, int step, int n)
{
    int v = seed_rank(r->metr, r->beat, SEED_ROLE_METER, step, 1);   /* one bar: every bar alike */
    (void)n;
    if (v < 0)
        return v;
    return (v * 255 + SEED_RANK_MAX / 2) / SEED_RANK_MAX;
}

int seed_acc_top(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    uint16_t rank[SEED_STEPS];
    uint32_t key[SEED_STEPS];
    uint8_t elig[SEED_STEPS], order[SEED_STEPS], chosen[SEED_STEPS];
    int i, m, n = t->length;
    (void)c;
    if (n <= 0 || n > SEED_STEPS)
        return SEED_OK;
    for (i = 0; i < n; i++) {
        int v = seed_rank(r->metr, r->beat, SEED_ROLE_METER, i, n);
        if (v < 0)
            return v;
        rank[i] = (uint16_t)v;
        elig[i] = (t->s[i].flags & SEED_TRIG) != 0;
        chosen[i] = 0;
    }
    seed_keys(key, n, k, SEED_SLOT(SEED_L_DYN, SEED_P_ACC, 0), rank, 100);   /* the pure rank order */
    m = seed_share(r->acc, seed_order(order, key, elig, n));
    for (i = 0; i < m; i++)
        chosen[order[i]] = 1;
    for (i = 0; i < n; i++)
        if (t->s[i].flags & SEED_TRIG)
            t->s[i].flags = (uint16_t)(chosen[i] ? t->s[i].flags | SEED_ACCENT : t->s[i].flags & ~SEED_ACCENT);
    return SEED_OK;
}
