/* SPDX-License-Identifier: GPL-2.0-or-later */
/* dyn: DYNAMICS: VEL, the shapes, ACC, HUM; 0.3's metric weight.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_dyn = 0;

int seed_dyn(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}

int seed_weight(int step, int beat)
{
    (void)step; (void)beat;
    return SEED_E_NA;
}
