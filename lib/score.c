/* SPDX-License-Identifier: GPL-2.0-or-later */
/* score: TAKE: QUAL, candidate scoring.
 * Owner: task F11 (docs/plan-v1.md section 4; docs/status/F11.md).
 * seed_render.c's best() scores each candidate at the end of PLACE and keeps the highest (ties:
 * the lower candidate). The score reads only where the trigs are, so no DYNAMICS..TIME knob can
 * change the winner. Three terms, each deciding only when the ones before are equal:
 *   C  count kept: RANDOM only, how close the trigs are to the count DENS asked for (MASK and SKIP
 *      remove trigs; other sources: equal for every candidate);
 *   S  spread: the smallest largest gap between consecutive trigs, wrapping at the length, then the
 *      fewest adjacent trigs (a gap of at most one grid step). It does not change when the take is
 *      rotated, so at BIAS 0 it pulls trigs neither towards strong nor towards weak steps;
 *   F  fit to the meter: RANDOM with BIAS != 0 only, the mean rank of the trigs in BIAS's direction.
 * score = C << 24 | (64 - largest gap) << 17 | (64 - adjacent) << 10 | F >> 6, below 2^31.
 * Freestanding. */
#include "seed.h"

const uint8_t seed_have_score = 1;

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

int seed_score(const struct seed_track *t, const struct seed_recipe *r, const struct seed_ctx *c)
{
    int n = clamp(t->length, 1, SEED_STEPS), random = r->src == SEED_SRC_RANDOM, grid = 1, bias = 0;
    int i, trigs = 0, first = -1, last = -1, gap = 0, adj = 0, kept = 64;
    uint32_t sum = 0;
    (void)c;

    if (random) {
        grid = r->grid == 2 || r->grid == 4 || r->grid == 8 ? r->grid : 1;
        bias = clamp(r->bias, -100, 100);
    }
    for (i = 0; i < n; i++) {
        if (!(t->s[i].flags & SEED_TRIG))
            continue;
        if (last >= 0) {
            gap = i - last > gap ? i - last : gap;
            adj += i - last <= grid;
        } else {
            first = i;
        }
        if (bias) {
            int v = seed_rank(r->metr, r->beat, r->role, i, n);
            if (v < 0)
                return v;
            sum += (uint32_t)(bias < 0 ? SEED_RANK_MAX - v : v);
        }
        last = i;
        trigs++;
    }
    if (trigs) {
        int wrap = first + n - last;              /* from the last trig round to the first */
        gap = wrap > gap ? wrap : gap;
        adj += trigs > 1 && wrap <= grid;
    } else {
        gap = 64;                                 /* no trigs: the worst spread */
    }
    if (random) {
        int asked = seed_share(r->dens, (n + grid - 1) / grid), d = trigs - asked;
        kept = 64 - clamp(d < 0 ? -d : d, 0, 64);
    }
    return kept << 24 | (64 - gap) << 17 | (64 - adj) << 10 | (int)(trigs ? sum / (uint32_t)trigs >> 6 : 0);
}
