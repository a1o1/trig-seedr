/* SPDX-License-Identifier: GPL-2.0-or-later */
/* dyn_noise: DYNAMICS: SHAPE = NOISE (flag: seed_have_noise in noise.c).
 * Owner: task F7 (docs/plan-v1.md section 4).
 * The step's level is the take's value noise (noise.c) with N steps per cell, its corners from the
 * DYNAMICS slots SEED_P_SHAPE + corner: a slow contour of velocity between VEL's low and high end,
 * with AMT its share as for every shape. Freestanding. */
#include "noise.h"

int seed_shape_noise(const struct seed_recipe *r, const struct seed_take *k, int step)
{
    return seed_noise_take(k, SEED_SLOT(SEED_L_DYN, SEED_P_SHAPE, 0), step, r->n);
}
