/* SPDX-License-Identifier: GPL-2.0-or-later */
/* dyn_noise: DYNAMICS: SHAPE = NOISE (flag: seed_have_noise in noise.c).
 * Owner: task F7 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

int seed_shape_noise(const struct seed_recipe *r, const struct seed_take *k, int step)
{
    (void)r; (void)k; (void)step;
    return SEED_E_NA;
}
