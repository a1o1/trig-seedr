/* SPDX-License-Identifier: GPL-2.0-or-later */
/* src_copy: the COPY source.
 * Owner: task F4 (docs/plan-v1.md section 4).
 * SRC = COPY: step i has a trig when track TRK has one at step i, read from the context (the
 * pattern as stored; the track in scope as its original). A shorter track repeats over a longer
 * one, as it plays; a longer one gives its start. Only the positions are copied: every step is a
 * generated step (seed_step_clear), so later layers dress it as any source's. TRK may be the
 * track in scope: its original rhythm, with the fields at their defaults. No random numbers.
 * Freestanding. */
#include "seed.h"

const uint8_t seed_have_copy = 1;

int seed_src_copy(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int i, from = r->copy_trk, n;
    (void)k;
    if (from < 0 || from >= c->tracks)
        return SEED_E_ARG;
    n = c->length[from];
    if (n < 1)
        return SEED_OK;                   /* no such track in the pattern: no trigs */
    for (i = 0; i < t->length; i++)
        if (seed_ctx_trig(c, from, i % n))
            t->s[i].flags |= SEED_TRIG;
    return SEED_OK;
}
