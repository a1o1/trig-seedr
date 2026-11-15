/* SPDX-License-Identifier: GPL-2.0-or-later */
/* vary: TAKE: PHRS over the pages, CVAR as conditions.
 * Owner: task F9 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_vary = 0;

int seed_phrase_take(int phrs, int page)
{
    (void)phrs; (void)page;
    return SEED_E_NA;
}

int seed_cvar_merge(struct seed_track *out, const struct seed_track *a, const struct seed_track *b,
                    const struct seed_recipe *r)
{
    (void)out; (void)a; (void)b; (void)r;
    return SEED_E_NA;
}
