/* SPDX-License-Identifier: GPL-2.0-or-later */
/* src_map: the MAP source and SHAPE = MAP's levels (map data in map_data.c, GPL-3.0-or-later).
 * Owner: task F3 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_map = 0;

int seed_src_map(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)t; (void)r; (void)k; (void)c;
    return SEED_E_NA;
}

int seed_map_level(int part, int x, int y, int step)
{
    (void)part; (void)x; (void)y; (void)step;
    return SEED_E_NA;
}
