/* SPDX-License-Identifier: GPL-2.0-or-later */
/* timing_drift: TIME: DRFT, correlated drift.
 * Owner: task F8 (docs/plan-v1.md section 4).
 * A slow wander of the micro timing along the track, added to each trig after seed_time's RAND,
 * QUANT and groove: neighbouring steps drift the same way, as a player who runs ahead of or behind
 * the beat for a while (RAND gives each trig its own offset). The curve is a sum of octaves of
 * value noise over the step positions, cells of 2, 4, ... steps (each shorter than the length),
 * linearly interpolated and wrapping at the length, so the loop has no jump:
 *   drift(i) = sum of the octaves' values at i / sqrt(octaves), scaled to +-DRFT % of the device's
 *              range (utime_hi, as RAND) and clamped to it
 *   utime    = utime + drift(i), clamped to the device's range
 * Each cell's value is its own slot (SEED_P_DRIFT, index octave * 256 + cell), so VARY keeps or
 * re-rolls cells (P3) and DRFT shares no numbers with another knob (P1). Raising DRFT scales the
 * same curve. Freestanding. */
#include "seed.h"

const uint8_t seed_have_drift = 1;

#define CELL 1024                     /* a cell's value: -CELL..CELL */
#define OCTAVES 5                     /* cells of 2 .. 32 steps */

/* 16 * sqrt(k) for k octaves, rounded */
static const uint8_t root16[OCTAVES + 1] = { 0, 16, 23, 28, 32, 36 };

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* a / b rounded to nearest, halves away from zero; b > 0 */
static int rdiv(int a, int b)
{
    return a >= 0 ? (a + b / 2) / b : -((-a + b / 2) / b);
}

/* the octaves' cells for a track of n steps, each value drawn once; returns how many octaves */
static int cells(int16_t v[OCTAVES][SEED_STEPS / 2], const struct seed_take *k, int n)
{
    int o, z, j;
    for (o = 0, z = 2; o < OCTAVES && z < n; o++, z <<= 1)
        for (j = 0; j < (n + z - 1) / z; j++)
            v[o][j] = (int16_t)seed_rand_range(k, SEED_SLOT(SEED_L_TIME, SEED_P_DRIFT, o << 8 | j), -CELL, CELL);
    return o;
}

/* the sum of the octaves' values at step i of n, each interpolated between its cells */
static int curve(int16_t v[OCTAVES][SEED_STEPS / 2], int octaves, int i, int n)
{
    int o, z, sum = 0;
    for (o = 0, z = 2; o < octaves; o++, z <<= 1) {
        int m = (n + z - 1) / z, j = i / z, a = i - j * z, span = j == m - 1 ? n - j * z : z;
        int v0 = v[o][j], v1 = v[o][j + 1 < m ? j + 1 : 0];
        sum += v0 + rdiv((v1 - v0) * a, span);
    }
    return sum;
}

int seed_time_drift(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int16_t v[OCTAVES][SEED_STEPS / 2];
    int n = t->length, i, lo = -23, hi = 23, m, octaves;

    if (c && c->caps) {
        lo = c->caps->utime_lo;
        hi = c->caps->utime_hi;
    }
    m = hi * clamp(r->drft, 0, 100) / 100;
    if (!m || !(t->fields & SEED_F_UTIME))
        return SEED_OK;
    octaves = cells(v, k, n);
    if (!octaves)
        return SEED_OK;               /* a track of 1 or 2 steps has no cells to drift between */
    for (i = 0; i < n; i++) {
        struct seed_step *s = &t->s[i];
        int d;
        if (!(s->flags & SEED_TRIG))
            continue;
        d = clamp(rdiv(curve(v, octaves, i, n) * m * 16, CELL * root16[octaves]), -m, m);
        s->utime = (int8_t)clamp(s->utime + d, lo, hi);
    }
    return SEED_OK;
}
