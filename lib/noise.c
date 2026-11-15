/* SPDX-License-Identifier: GPL-2.0-or-later */
/* noise: integer value noise and the NOISE value source.
 * Owner: task F7 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_noise = 0;

int seed_noise(uint32_t seed, int x, int zoom)
{
    (void)seed; (void)x; (void)zoom;
    return SEED_E_NA;
}

int seed_value_noise(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int step)
{
    (void)v; (void)k; (void)slot; (void)step;
    return SEED_E_NA;
}
