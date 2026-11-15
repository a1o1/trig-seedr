/* SPDX-License-Identifier: GPL-2.0-or-later */
/* score: TAKE: QUAL, candidate scoring.
 * Owner: task F11 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_score = 0;

int seed_score(const struct seed_track *t, const struct seed_recipe *r, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)c;
    return SEED_E_NA;
}
