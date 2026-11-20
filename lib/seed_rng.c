/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seed_rng: random numbers by slot, keys and shares, VARY (design section 3, P1-P3). Owner: the
 * lead. Counter based: a number is a hash of (take, slot), so no layer can disturb another's
 * numbers by drawing more or fewer (P1). Freestanding; ColdFire has mulu.l, divu.l and remu.l,
 * so 32-bit * / % need no libgcc. */
#include "seed.h"

uint32_t seed_hash(uint32_t x)
{
    /* lowbias32 (C. Wellons' hash-prospector): an invertible mix of 32 bits */
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

static uint32_t mix(uint32_t a, uint32_t b)
{
    return seed_hash(a ^ seed_hash(b + 0x9e3779b9u));
}

void seed_take_init(struct seed_take *k, uint32_t seed, uint32_t salt, int vary, int cand)
{
    k->seed = seed;
    k->salt = salt;
    k->vary = vary < 0 ? 0 : vary > 100 ? 100 : vary;
    k->cand = cand < 0 ? 0 : cand;
    k->length = SEED_STEPS;          /* seed_render sets the track's */
    k->tur = 0;                       /* seed_render sets it when the recipe uses TURING */
}

/* the slot's identity in this track and candidate, independent of the take number. QUAL's
 * candidates differ only in RHYTHM and PLACE (where the trigs are); every later layer draws
 * candidate 0's numbers, so the winner's dressing does not depend on which candidate won (F11). */
static uint32_t slot_base(const struct seed_take *k, uint32_t slot)
{
    uint32_t layer = slot >> 24;
    uint32_t cand = layer == SEED_L_RHYTHM || layer == SEED_L_PLACE ? (uint32_t)k->cand : 0u;
    return mix(mix(slot, k->salt), cand);
}

/* the slot's number when it re-rolled at take n */
static uint32_t rolled(uint32_t base, uint32_t n)
{
    return mix(base, n);
}

static int rerolls(uint32_t base, uint32_t n, int vary)
{
    uint32_t r = mix(base ^ 0x52e5011du, n);  /* a hash family apart from rolled()'s */
    return (int)(r % 100u) >= vary;
}

/* Reads only seed, salt, vary and cand of the take: values_list.c keeps its last list on exactly those
 * (M2.3), so a change here that reads another field must add it to that cache's key. */
uint32_t seed_rand(const struct seed_take *k, uint32_t slot)
{
    uint32_t base = slot_base(k, slot), n = k->seed;
    int i;
    if (k->vary <= 0)
        return rolled(base, n);
    for (i = 0; i < SEED_VARY_WALK; i++, n--)    /* take numbers wrap below 0, deterministically */
        if (rerolls(base, n, k->vary))
            return rolled(base, n);
    return seed_hash(base ^ 0xa0c1e5edu);         /* the anchor: no take re-rolled in the walk */
}

int seed_rand_range(const struct seed_take *k, uint32_t slot, int lo, int hi)
{
    uint32_t span;
    if (hi < lo) {
        int t = lo;
        lo = hi;
        hi = t;
    }
    span = (uint32_t)(hi - lo) + 1u;
    return lo + (int)(seed_rand(k, slot) % span);
}

void seed_keys(uint32_t *key, int n, const struct seed_take *k, uint32_t slot, const uint16_t *rank, int bias)
{
    int i, a;
    if (bias < -100)
        bias = -100;
    if (bias > 100)
        bias = 100;
    a = bias < 0 ? -bias : bias;
    if (!rank)
        a = 0;
    for (i = 0; i < n && i < 256; i++) {
        uint32_t rnd = seed_rand(k, slot + (uint32_t)i) >> 16, r = 0;
        if (rank)
            r = bias < 0 ? SEED_RANK_MAX - rank[i] : rank[i];
        /* at most 100 * 65535 = 6553500 < 2^23, so 8 bits are left for the tie break */
        key[i] = ((uint32_t)a * r + (uint32_t)(100 - a) * rnd) << 8 | (uint32_t)(255 - i);
    }
}

int seed_order(uint8_t *order, const uint32_t *key, const uint8_t *eligible, int n)
{
    int i, j, m = 0;
    for (i = 0; i < n; i++) {
        if (eligible && !eligible[i])
            continue;
        /* insertion by key, highest first; keys are distinct */
        for (j = m; j > 0 && key[order[j - 1]] < key[i]; j--)
            order[j] = order[j - 1];
        order[j] = (uint8_t)i;
        m++;
    }
    return m;
}

int seed_share(int pct, int n)
{
    if (pct <= 0 || n <= 0)
        return 0;
    if (pct >= 100)
        return n;
    return (pct * n + 50) / 100;
}
