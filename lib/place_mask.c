/* SPDX-License-Identifier: GPL-2.0-or-later */
/* place_mask: PLACE: MASK.
 * Owner: task F4 (docs/plan-v1.md section 4).
 * MASK = AVOID clears every trig on a step where track MTRK has a trig; ONLY clears every trig
 * on a step where it has none. MTRK is read from the context; a shorter track repeats over a
 * longer one, as it plays. A cleared step is SKIP's: no trig, fields at their defaults, origin
 * none. Steps without a trig are left. It runs after ROT and SYNC, so it holds on the final
 * positions (P5), and it only removes (I5). MTRK may be the track in scope (its original).
 * No random numbers. Freestanding. */
#include "seed.h"

const uint8_t seed_have_mask = 1;

int seed_place_mask(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int i, other = r->mtrk, n, only;
    (void)k;
    if (r->mask <= SEED_MASK_OFF || r->mask >= SEED_MASK_N || other < 0 || other >= c->tracks)
        return SEED_E_ARG;
    only = r->mask == SEED_MASK_ONLY;
    n = c->length[other];
    for (i = 0; i < t->length; i++) {
        int hit = n > 0 && seed_ctx_trig(c, other, i % n);
        if ((t->s[i].flags & SEED_TRIG) && hit != only)
            seed_step_clear(&t->s[i]);
    }
    return SEED_OK;
}
