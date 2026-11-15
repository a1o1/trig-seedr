/* SPDX-License-Identifier: GPL-2.0-or-later */
/* src_euclid: the EUCLID source: STEPS, PULS.
 * Owner: task F2 (docs/plan-v1.md section 4).
 * PULS pulses spread as evenly as they go over a cycle of STEPS steps: step j of the cycle has a
 * trig when (j * PULS) mod STEPS < PULS. This is the pattern the Rytm's own Euclidean mode plays
 * for LEN = STEPS and PL1 = PULS (PL2 0, no rotation; tools/tests/22-euclid.sh compares the two in
 * the emulator): the first step always has a pulse, and the gaps between pulses differ by at most
 * one step. A track longer than STEPS repeats the cycle; a shorter one plays its start. PULS above
 * STEPS gives every step. No random numbers: every seed gives the same take. Freestanding. */
#include "seed.h"

const uint8_t seed_have_euclid = 1;

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

int seed_src_euclid(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int steps = clamp(r->eu_steps, 1, SEED_STEPS), puls = clamp(r->eu_puls, 0, steps), i;
    (void)k; (void)c;

    for (i = 0; i < t->length; i++)
        if (i % steps * puls % steps < puls)
            t->s[i].flags |= SEED_TRIG;
    return SEED_OK;
}
