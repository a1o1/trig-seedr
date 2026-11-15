/* SPDX-License-Identifier: GPL-2.0-or-later */
/* values: value sources (RANGE) and the NOTE and LOCK stages.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_values = 0;

int seed_value(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int step, int index)
{
    (void)v; (void)k; (void)slot; (void)step; (void)index;
    return SEED_E_NA;
}

int seed_note(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}

int seed_lock(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}
