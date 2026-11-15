/* SPDX-License-Identifier: GPL-2.0-or-later */
/* place_sync: PLACE: SYNC, anticipation (after G. Sioros and C. Guedes' syncopation transformations).
 * Owner: task F5 (docs/plan-v1.md section 4).
 *
 * A trig is anticipated: moved one grid step earlier, onto a weaker step, leaving its own step
 * empty. The grid step is RANDOM's GRID (1 for every other source); weaker is a lower Barlow rank
 * (meter.c, ROLE = METER whatever ROLE says, as SHAPE = RANK: ROLE is a RHYTHM knob).
 *
 *   candidate  a trig at or past SKIP whose step one grid step earlier (round the loop) is empty,
 *              at or past SKIP, and weaker, with no trig between the two
 *   run        two or more consecutive trigs (step order, round the loop, from SKIP) all moved;
 *              it must resolve onto a strong step: the trig after it stays, and is stronger than
 *              every step the run's trigs left. So not every trig can move (that would only
 *              rotate the track), and a lone anticipation is always allowed.
 *
 * The candidates are tried in the order of the take's random keys (P1: slot SEED_P_SYNC); a move
 * is kept when every run still resolves. SYNC % makes the first share of the kept moves (P2:
 * each share is a set the rule allows, and raising SYNC adds to it).
 *
 * A move swaps the trig's whole step (origin, lanes, every field) with the empty one, so the
 * device moves it with all its parameter locks; swing stays with the position. Nothing is created
 * or removed: the count of trigs is unchanged (I5), and no trig enters or leaves the steps SKIP
 * clears. Freestanding. */
#include "seed.h"

const uint8_t seed_have_sync = 1;

static int mod(int k, int n)
{
    k %= n;
    return k < 0 ? k + n : k;
}

/* 1 when every run of moved trigs resolves: trig[0..m) in step order, moved[] by index */
static int resolves(const uint8_t *trig, const uint8_t *moved, int m, const uint16_t *rank)
{
    int start, j, len = 0, top = 0;
    for (start = 0; start < m && moved[start]; start++)
        ;
    if (start == m)
        return 0;                             /* every trig moved: nothing to resolve onto */
    for (j = 1; j <= m; j++) {
        int x = mod(start + j, m), v = rank[trig[x]];
        if (moved[x]) {
            len++;
            if (v > top)
                top = v;
            continue;
        }
        if (len >= 2 && v <= top)
            return 0;
        len = top = 0;
    }
    return 1;
}

int seed_place_sync(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    uint16_t rank[SEED_STEPS];
    uint32_t key[SEED_STEPS];
    uint8_t trig[SEED_STEPS], index[SEED_STEPS], cand[SEED_STEPS], moved[SEED_STEPS], order[SEED_STEPS];
    int n = t->length, skip = r->skip, g = 1, i, j, m = 0, nc, kept = 0;
    (void)c;

    if (r->metr < 0 || r->metr >= SEED_METR_N)
        return SEED_E_ARG;
    if (n <= 1 || n > SEED_STEPS)
        return SEED_OK;
    if (skip < 0)
        skip = 0;
    if (r->src == SEED_SRC_RANDOM && (r->grid == 2 || r->grid == 4 || r->grid == 8))
        g = r->grid;
    for (i = 0; i < n; i++) {
        int v = seed_rank(r->metr, r->beat, SEED_ROLE_METER, i, n);
        if (v < 0)
            return v;
        rank[i] = (uint16_t)v;
        cand[i] = 0;
    }
    /* the trigs from SKIP on, in step order, and which are candidates: the steps from p up to the
     * trig are empty (a trig is never moved past another), p is weaker */
    for (i = skip; i < n; i++)
        if (t->s[i].flags & SEED_TRIG) {
            int p = mod(i - g, n), d, free = p != i && p >= skip;
            for (d = 1; d <= g && free; d++) {
                int q = mod(i - d, n);
                free = q < skip || !(t->s[q].flags & SEED_TRIG);
            }
            cand[i] = (uint8_t)(free && rank[p] < rank[i]);
            index[i] = (uint8_t)m;
            moved[m] = 0;
            trig[m++] = (uint8_t)i;
        }
    if (m < 2)
        return SEED_OK;                       /* a lone trig would only rotate */
    seed_keys(key, n, k, SEED_SLOT(SEED_L_PLACE, SEED_P_SYNC, 0), 0, 0);
    nc = seed_order(order, key, cand, n);
    /* try each candidate in key order; the kept ones go to the front of order[] */
    for (j = 0; j < nc; j++) {
        int x = index[order[j]];
        moved[x] = 1;
        if (resolves(trig, moved, m, rank))
            order[kept++] = order[j];
        else
            moved[x] = 0;
    }
    kept = seed_share(r->sync, kept);
    for (j = 0; j < kept; j++) {
        /* Swing is set in the copies, then each step is stored whole. Storing the steps and then
         * their flags is miscompiled by m68k-elf-gcc 16.2 at -O2 (store merging puts one word 6
         * bytes early; docs/status/F5.md). */
        int s = order[j], p = mod(s - g, n);
        struct seed_step a = t->s[s], b = t->s[p];
        uint16_t sw_a = a.flags & SEED_SWING;
        a.flags = (uint16_t)((a.flags & ~SEED_SWING) | (b.flags & SEED_SWING));
        b.flags = (uint16_t)((b.flags & ~SEED_SWING) | sw_a);
        t->s[p] = a;
        t->s[s] = b;
    }
    return SEED_OK;
}
