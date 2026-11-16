/* SPDX-License-Identifier: GPL-2.0-or-later */
/* values_list: value lists with an order (LIST) and LMOD = LEGATO.
 * Owner: task F6 (docs/plan-v1.md section 4).
 * A list is `len` items, item i drawn from the range from slot + 0x100 + i (a P3 slot each, so VARY
 * keeps or re-rolls items one by one) and sorted low to high. The track's trigs walk it in their
 * order: the index-th trig gets the item ORDR picks for index. RAND picks item by item from
 * slot + 0x140 + index. LEGATO gives a trig the shortest length code that reaches the next trig.
 * Neither function can report an error; a length or order out of range is clamped (UP). Freestanding. */
#include "seed.h"

const uint8_t seed_have_list = 1;

#define LIST_ITEM 0x100u                /* slot + LIST_ITEM + i: item i */
#define LIST_PICK 0x140u                /* slot + LIST_PICK + index: RAND's pick for the index-th trig */

/* the place in the sorted list (0 = lowest) that order o gives the index-th trig, n >= 1 */
static int place(int o, int n, int index, const struct seed_take *k, uint32_t slot)
{
    int q;
    switch (o) {
    case SEED_ORDR_DOWN:
        return n - 1 - index % n;
    case SEED_ORDR_PING:                /* 0 1 2 3 2 1, the ends once */
        if (n == 1)
            return 0;
        q = index % (2 * n - 2);
        return q < n ? q : 2 * n - 2 - q;
    case SEED_ORDR_PNG2:                /* 0 1 2 3 3 2 1 0, the ends twice */
        q = index % (2 * n);
        return q < n ? q : 2 * n - 1 - q;
    case SEED_ORDR_ODD:                 /* the 1st, 3rd, 5th ... then the 2nd, 4th ... */
        q = index % n;
        return q < (n + 1) / 2 ? 2 * q : 2 * (q - (n + 1) / 2) + 1;
    case SEED_ORDR_RAND:
        return seed_rand_range(k, slot + LIST_PICK + (uint32_t)index, 0, n - 1);
    default:                            /* UP */
        return index % n;
    }
}

/* The last list made (task M2.3). A stage asks for one item per trig, so the same list was made
 * again for every trig: up to 16 draws each, and each draw a walk of up to 64 takes at a high
 * VARY. The items depend only on the range, the length, the slot and the take's numbers (the
 * fields of struct seed_take that seed_rand reads), so a list made from the same ones is the same
 * list: it is kept and used again. */
static struct {
    int valid, lo, hi, n, vary, cand;
    uint32_t slot, seed, salt;
    int item[SEED_LIST_MAX];
} last;

int seed_value_list(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int index)
{
    int n = v->len < 1 ? 1 : v->len > SEED_LIST_MAX ? SEED_LIST_MAX : v->len;
    int i, j, p = place(v->order, n, index < 0 ? 0 : index, k, slot);

    if (!last.valid || last.lo != v->lo || last.hi != v->hi || last.n != n || last.slot != slot ||
        last.seed != k->seed || last.salt != k->salt || last.vary != k->vary || last.cand != k->cand) {
        /* the items, sorted as they are drawn (insertion) */
        for (i = 0; i < n; i++) {
            int x = seed_rand_range(k, slot + LIST_ITEM + (uint32_t)i, v->lo, v->hi);
            for (j = i; j > 0 && last.item[j - 1] > x; j--)
                last.item[j] = last.item[j - 1];
            last.item[j] = x;
        }
        last.valid = 1;
        last.lo = v->lo;
        last.hi = v->hi;
        last.n = n;
        last.slot = slot;
        last.seed = k->seed;
        last.salt = k->salt;
        last.vary = k->vary;
        last.cand = k->cand;
    }
    return last.item[p];
}

/* a length code's duration in 1/16 of a step (docs/firmware-notes-os174.md: LENGTH), codes 0..126 */
static int duration(int code)
{
    int seg, base = 32, unit = 2;
    if (code <= 30)                     /* 2 .. 32, by 1 */
        return code + 2;
    for (seg = 30; seg + 16 < code; seg += 16) {
        base *= 2;
        unit *= 2;
    }
    return base + unit * (code - seg);
}

int seed_len_legato(const struct seed_track *t, int step)
{
    int n = t->length < 1 ? 1 : t->length > SEED_STEPS ? SEED_STEPS : t->length;
    int gap, code;

    /* steps to the next trig, wrapping at the length; a lone trig reaches itself a loop later */
    for (gap = 1; gap < n; gap++)
        if (t->s[(step + gap) % n].flags & SEED_TRIG)
            break;
    for (code = 0; code < 126 && duration(code) < 16 * gap; code++)
        ;
    return code;
}
