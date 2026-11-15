/* SPDX-License-Identifier: GPL-2.0-or-later */
/* meter: rank tables, meters (METR, BEAT) and roles.
 * Owner: task F1 (docs/plan-v1.md section 4).
 *
 * A bar is a prime stratification (C. Barlow, "Two Essays on Theory", Computer Music Journal
 * 11(1), 1987): the meter's groupings of its note value, top first, then BEAT's prime factors,
 * smallest first (BEAT 6 = 2 x 3: the half beat is the "&", as 0.3's weights).
 *   4/4  2 2      3/4  3      6/8  2 3      12/16  2 2 3      5/8  5      7/8  7
 * BEAT is steps per note value of the meter (a quarter in 4/4 and 3/4, an eighth in 6/8, 5/8 and
 * 7/8, a sixteenth in 12/16), so a bar is (6 / 8: 6) x BEAT steps.
 *
 * A step's indispensability psi (Barlow's formula; 0 .. bar - 1, higher = stronger) orders the
 * bar. Its depth is the first stratum whose grid it lies on, and the depth gives five classes:
 *   D the bar's first step; G a grouping above the counted beat (4/4's beat 3); B a counted beat
 *   and nothing more (4/4's beats 2 and 4, 6/8's second dotted quarter); O the counted beat's first
 *   division (the "&"); F finer steps.
 * psi orders the depths (every step of a depth is above every deeper step), so each class is a
 * run of psi. METER is psi; the other roles put the classes in another order and keep psi within
 * a class. The five-level weight (the velocity shapes) is the class: D 4, G 3, B 2, O 1, F 0,
 * which in 4/4 is 0.3's weight for every even BEAT.
 *
 * Over more than one bar, every bar has the same ranks; ties between bars go to the bars' own
 * Barlow order (bars as a 2 x 2 ... stratification, so bar 1, then 3, then 4, then 2).
 * Integer only, freestanding. */
#include "seed.h"

const uint8_t seed_have_rank = 1;

#define MAX_STRATA 8                  /* a meter's 3 and BEAT's 4 (16 = 2 2 2 2) */

/* Barlow's basic indispensability of each pulse of a prime meter, 1-indexed pulses in order */
static const uint8_t psi2[2] = { 1, 0 };
static const uint8_t psi3[3] = { 2, 0, 1 };
static const uint8_t psi5[5] = { 4, 0, 3, 1, 2 };
static const uint8_t psi7[7] = { 6, 0, 4, 2, 5, 1, 3 };
static const uint8_t psi11[11] = { 10, 0, 6, 4, 9, 1, 7, 3, 8, 2, 5 };
static const uint8_t psi13[13] = { 12, 0, 7, 4, 10, 1, 8, 5, 11, 2, 9, 3, 6 };

static const uint8_t *basic(int p)
{
    switch (p) {
    case 2: return psi2;
    case 3: return psi3;
    case 5: return psi5;
    case 7: return psi7;
    case 11: return psi11;
    default: return psi13;
    }
}

/* each meter's strata (top first) and how many of them lie above the counted beat */
static const struct {
    uint8_t q[3], z, k;
} meters[SEED_METR_N] = {
    { { 2, 2 }, 2, 2 },               /* 4/4: four quarters */
    { { 3 }, 1, 1 },                  /* 3/4: three quarters */
    { { 2, 3 }, 2, 1 },               /* 6/8: two dotted quarters of three eighths */
    { { 2, 2, 3 }, 3, 2 },            /* 12/16: four dotted eighths of three sixteenths */
    { { 5 }, 1, 1 },                  /* 5/8 */
    { { 7 }, 1, 1 },                  /* 7/8 */
};

enum { C_F, C_O, C_B, C_G, C_D, C_N };

/* where each class goes in a role's order, 0 = weakest (ROLE = METER uses psi itself) */
static const uint8_t role_prio[SEED_ROLE_N][C_N] = {
    /*            F  O  B  G  D */
    [SEED_ROLE_METER] = { 0, 1, 2, 3, 4 },
    [SEED_ROLE_KICK]  = { 1, 2, 0, 3, 4 },   /* D G O F B: the downbeat side, away from the backbeat */
    [SEED_ROLE_BACK]  = { 3, 2, 4, 1, 0 },   /* B F O G D: the backbeat, then ghost notes */
    [SEED_ROLE_HAT]   = { 0, 4, 1, 2, 3 },   /* O D G B F: off-beats, then the beats */
    [SEED_ROLE_PERC]  = { 4, 3, 2, 1, 0 },   /* F O B G D: the finest steps first, pickups leading */
};

struct bar {
    uint8_t q[MAX_STRATA];
    int z, k, len;                    /* strata, those above the counted beat, steps */
};

static int bar_of(struct bar *b, int metr, int beat)
{
    int i, p;
    if (metr < 0 || metr >= SEED_METR_N)
        return SEED_E_ARG;
    beat = beat < 1 ? 1 : beat > 16 ? 16 : beat;
    b->z = meters[metr].z;
    b->k = meters[metr].k;
    b->len = 1;
    for (i = 0; i < b->z; i++) {
        b->q[i] = meters[metr].q[i];
        b->len *= b->q[i];
    }
    for (p = 2; beat > 1; p++)
        while (beat % p == 0) {
            b->q[b->z++] = (uint8_t)p;
            b->len *= p;
            beat /= p;
        }
    return SEED_OK;
}

/* Barlow's indispensability of step x (0-indexed) of a stratification q of z strata, n steps
 * (hmt's Music.Theory.Meter.Barlow_1987 lower_psi; pulse x + 1 there) */
static int psi(const uint8_t *q, int z, int n, int x)
{
    int r, i, sum = 0, s2 = (x - 1 + n) % n;
    for (r = 0; r < z; r++) {
        int s3 = 1, s7 = 1, c = q[z - r - 1];
        for (i = 1; i <= r; i++)                /* q[z + 1 - i] for i = 0..r, 1-indexed; q[z + 1] = 1 */
            s3 *= q[z - i];
        for (i = 1; i < z - r; i++)             /* q[1] .. q[z - r - 1], 1-indexed */
            s7 *= q[i - 1];
        sum += s7 * basic(c)[(1 + s2 / s3) % c];
    }
    return sum;
}

/* steps of the bar at depth d or less: the product of the first d strata (d < 0: none) */
static int on_grid(const struct bar *b, int d)
{
    int i, c = 1;
    if (d < 0)
        return 0;
    for (i = 0; i < d && i < b->z; i++)
        c *= b->q[i];
    return c;
}

static int depth(const struct bar *b, int x)
{
    int d, g = b->len;
    for (d = 0; d < b->z; d++) {
        if (x % g == 0)
            return d;
        g /= b->q[d];
    }
    return b->z;
}

/* a class's depths, lo..hi */
static void class_depths(const struct bar *b, int cls, int *lo, int *hi)
{
    switch (cls) {
    case C_D: *lo = 0; *hi = 0; break;
    case C_G: *lo = 1; *hi = b->k - 1; break;
    case C_B: *lo = b->k; *hi = b->k; break;
    case C_O: *lo = b->k + 1; *hi = b->k + 1; break;
    default: *lo = b->k + 2; *hi = b->z; break;
    }
}

static int class_of(const struct bar *b, int d)
{
    return d == 0 ? C_D : d < b->k ? C_G : d == b->k ? C_B : d == b->k + 1 ? C_O : C_F;
}

/* how many steps of the bar are in the class, and the lowest psi among them */
static int class_size(const struct bar *b, int cls, int *lowest)
{
    int lo, hi;
    class_depths(b, cls, &lo, &hi);
    if (hi > b->z)
        hi = b->z;
    if (lo > hi) {
        *lowest = 0;
        return 0;
    }
    *lowest = b->len - on_grid(b, hi);        /* the deeper steps have psi 0 .. lowest - 1 */
    return on_grid(b, hi) - on_grid(b, lo - 1);
}

/* step x's place in the role's order of the bar, 0 (weakest) .. len - 1 */
static int bar_index(const struct bar *b, int role, int x)
{
    int p = psi(b->q, b->z, b->len, x), cls, c, below = 0, lowest, me;
    if (role == SEED_ROLE_METER)
        return p;
    cls = class_of(b, depth(b, x));
    for (c = 0; c < C_N; c++)
        if (role_prio[role][c] < role_prio[role][cls])
            below += class_size(b, c, &lowest);
    class_size(b, cls, &lowest);
    me = p - lowest;
    return below + me;
}

int seed_rank(int metr, int beat, int role, int step, int n)
{
    static const uint8_t twos[6] = { 2, 2, 2, 2, 2, 2 };
    struct bar b;
    int rc = bar_of(&b, metr, beat), bars, z = 0, p = 1, top;
    if (rc < 0)
        return rc;
    if (role < 0 || role >= SEED_ROLE_N || step < 0)
        return SEED_E_ARG;
    if (n < 1)
        n = 1;
    bars = (n + b.len - 1) / b.len;
    while (p < bars) {                          /* bars rounded up to a power of 2, at most 64 */
        p *= 2;
        z++;
    }
    top = b.len * p - 1;
    if (top == 0)
        return SEED_RANK_MAX;
    rc = bar_index(&b, role, step % b.len) * p + (p > 1 ? psi(twos, z, p, (step / b.len) % p) : 0);
    return (int)(((uint32_t)rc * SEED_RANK_MAX + (uint32_t)top / 2) / (uint32_t)top);
}

int seed_meter_weight(int metr, int beat, int step)
{
    static const uint8_t level[C_N] = { 0, 1, 2, 3, 4 };
    struct bar b;
    int rc = bar_of(&b, metr, beat);
    if (rc < 0)
        return rc;
    if (step < 0)
        return 0;
    return level[class_of(&b, depth(&b, step % b.len))];
}
