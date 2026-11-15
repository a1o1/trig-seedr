/* SPDX-License-Identifier: GPL-2.0-or-later */
/* meter: rank tables, meters (METR, BEAT) and roles.
 * Owner: task F1 (docs/plan-v1.md section 4).
 * A stub from task 0.1: every function returns SEED_E_NA until the owner fills it. */
#include "seed.h"

const uint8_t seed_have_rank = 0;

int seed_rank(int metr, int beat, int role, int step, int n)
{
    (void)metr; (void)beat; (void)role; (void)step; (void)n;
    return SEED_E_NA;
}
