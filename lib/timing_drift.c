/* SPDX-License-Identifier: GPL-2.0-or-later */
/* timing_drift: TIME: DRFT, correlated drift.
 * Owner: task F8 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_drift = 0;

int seed_time_drift(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}
