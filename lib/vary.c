/* SPDX-License-Identifier: GPL-2.0-or-later */
/* vary: TAKE: PHRS over the pages, CVAR as conditions.
 * Owner: task F9 (docs/plan-v1.md section 4).
 * PHRS gives each 16-step page a letter, a take offset: page p plays take n + offset(p) as a
 * 16-step phrase (seed_render.c renders it), so pages with the same letter repeat. CVAR merges
 * takes n and n + 1 into one track: a trig in both plays plain, the others get complementary
 * conditions, so the track plays take n and take n + 1 in turn (1:2) or take n three times and
 * take n + 1 once (AAAB). The Rytm has no "not 4:4", so AAAB gives take n + 1's trigs 4:4 and
 * take n's !PRE, which is true when the last condition evaluated on the track (PRE and !PRE not
 * counted) was false; a trig of take n before the first 4:4 trig has nothing to follow and plays
 * plain, in all four loops. A trig in both keeps take n's step, its own condition included.
 * Freestanding. */
#include "seed.h"

const uint8_t seed_have_vary = 1;

/* condition codes beyond A:B (docs/firmware-notes-os174.md) */
#define COND_NOT_PRE 25

/* the letters of each PHRS, A = 0 */
static const uint8_t letters[SEED_PHRS_N][SEED_STEPS / SEED_PAGE] = {
    { 0, 0, 0, 0 },                   /* OFF: one take */
    { 0, 0, 0, 0 },                   /* AAAA */
    { 0, 0, 0, 1 },                   /* AAAB */
    { 0, 1, 0, 1 },                   /* ABAB */
    { 0, 1, 0, 2 },                   /* ABAC */
    { 0, 1, 2, 3 },                   /* ABCD */
};

int seed_phrase_take(int phrs, int page)
{
    if (phrs < 0 || phrs >= SEED_PHRS_N || page < 0 || page >= SEED_STEPS / SEED_PAGE)
        return SEED_E_ARG;
    return letters[phrs][page];
}

/* out may be a or b: each step is read before it is written */
int seed_cvar_merge(struct seed_track *out, const struct seed_track *a, const struct seed_track *b,
                    const struct seed_recipe *r)
{
    int i, n = a->length, only_a, only_b, seen_b = 0;
    if (r->cvar == SEED_CVAR_1_2) {
        only_a = seed_cond_ab(1, 2);
        only_b = seed_cond_ab(2, 2);
    } else if (r->cvar == SEED_CVAR_AAAB) {
        only_a = COND_NOT_PRE;
        only_b = seed_cond_ab(4, 4);
    } else {
        return SEED_E_ARG;
    }
    if (!(a->fields & SEED_F_COND))
        return SEED_E_NA;
    out->length = a->length;
    out->fields = a->fields;
    out->default_note = a->default_note;
    out->default_vel = a->default_vel;
    for (i = 0; i < SEED_STEPS; i++) {
        int ta = i < n && (a->s[i].flags & SEED_TRIG), tb = i < n && (b->s[i].flags & SEED_TRIG);
        if (tb && !ta) {
            out->s[i] = b->s[i];
            out->s[i].cond = (int8_t)only_b;
            seen_b = 1;
        } else {
            out->s[i] = a->s[i];
            /* AAAB: !PRE needs a 4:4 trig before it in the loop; one before the first plays plain */
            if (ta && !tb && (r->cvar == SEED_CVAR_1_2 || seen_b))
                out->s[i].cond = (int8_t)only_a;
        }
    }
    return SEED_OK;
}
