/* SPDX-License-Identifier: GPL-2.0-or-later */
/* dyn_rank: DYNAMICS: SHAPE = RANK, AMOD = TOP (flag: seed_have_rank in meter.c).
 * Owner: task F1 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

int seed_shape_rank(const struct seed_recipe *r, int step, int n)
{
    (void)r; (void)step; (void)n;
    return SEED_E_NA;
}

int seed_acc_top(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}
