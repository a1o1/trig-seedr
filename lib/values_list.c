/* SPDX-License-Identifier: GPL-2.0-or-later */
/* values_list: value lists with an order (LIST) and LMOD = LEGATO.
 * Owner: task F6 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_list = 0;

int seed_value_list(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int index)
{
    (void)v; (void)k; (void)slot; (void)index;
    return SEED_E_NA;
}

int seed_len_legato(const struct seed_track *t, int step)
{
    (void)t; (void)step;
    return SEED_E_NA;
}
