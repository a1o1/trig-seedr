/* SPDX-License-Identifier: GPL-2.0-or-later */
/* place: PLACE: ROT, SKIP, the field rotation (FLD, FROT, FMOD) and ROTATE ALL's rotation.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_place = 0;

int seed_place_rot(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}

int seed_place_skip(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}

int seed_place_fields(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}

int seed_rotate(struct seed_track *t, int from, int n, int k, int what, int mode)
{
    (void)t; (void)from; (void)n; (void)k; (void)what; (void)mode;
    return SEED_E_NA;
}
