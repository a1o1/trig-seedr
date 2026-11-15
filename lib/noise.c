/* SPDX-License-Identifier: GPL-2.0-or-later */
/* noise: integer value noise and the NOISE value source.
 * Owner: task F7 (docs/plan-v1.md section 4).
 * Value noise, one octave: a random level 0..255 at every `zoom`-th step (a cell's corner), and a
 * smoothstep (3t^2 - 2t^3) from one corner to the next. ZOOM 1 is a random level per step; higher
 * ZOOM a slower contour. Step 0 is a corner, and the contour wraps at the track's length.
 *
 * In the chain each corner's level is the take's random number of its own slot (corner c: slot + c),
 * so VARY keeps that share of the corners from take to take (P3) and each layer has its own corners
 * (P1). The NOISE value source uses slot + 0x200 + c (seed.h); SHAPE = NOISE is dyn_noise.c.
 * A trig's value depends on its step, not on how many trigs are before it, so DENS, the lock % and
 * SKIP do not change the value of a trig that stays. Freestanding, integer only. */
#include "noise.h"

const uint8_t seed_have_noise = 1;

#define NOISE_SLOT 0x200u             /* the NOISE source's corners: slot + 0x200 + corner (seed.h) */

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* the corner at or before x, and how far past it x is (0 .. zoom - 1); zoom >= 1 */
static int corner(int x, int zoom, int *f)
{
    int c = x >= 0 ? x / zoom : -((-x + zoom - 1) / zoom);
    *f = x - c * zoom;
    return c;
}

/* from level a (at f = 0) to b (at f = zoom), smoothstep, 0..255 */
static int ease(int a, int b, int f, int zoom)
{
    int t = f * 256 / zoom;                         /* 0..255 */
    int s = (t * t * (768 - 2 * t)) >> 16;          /* 3t^2 - 2t^3 at 8 bits: 0..255 */
    return (a * (256 - s) + b * s + 128) >> 8;
}

int seed_noise(uint32_t seed, int x, int zoom)
{
    int f, c;
    zoom = clamp(zoom, 1, SEED_STEPS);
    c = corner(x, zoom, &f);
    return ease((int)(seed_hash(seed ^ seed_hash((uint32_t)c)) >> 24),
                (int)(seed_hash(seed ^ seed_hash((uint32_t)(c + 1))) >> 24), f, zoom);
}

/* The contour wraps at the track's length (k->length): its corners are at steps 0, zoom, 2 zoom, ...
 * below the length, and the last cell runs from the last corner back to corner 0 over the steps
 * that are left, so the loop has no seam. */
int seed_noise_take(const struct seed_take *k, uint32_t slot, int step, int zoom)
{
    int f, c, n = clamp(k->length, 1, SEED_STEPS), corners, next, cell;
    zoom = clamp(zoom, 1, SEED_STEPS);
    c = corner(clamp(step, 0, n - 1), zoom, &f);
    corners = (n + zoom - 1) / zoom;
    next = c + 1 < corners ? c + 1 : 0;
    cell = c + 1 < corners ? zoom : n - c * zoom;
    return ease((int)(seed_rand(k, slot + (uint32_t)c) >> 24), (int)(seed_rand(k, slot + (uint32_t)next) >> 24),
                f, cell);
}

/* The level mapped onto lo..hi in equal shares (at ZOOM 1 as likely as RANGE's values): level 0
 * gives lo, 255 gives hi, so a range given high to low turns the contour over. */
int seed_value_noise(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int step)
{
    int lev = seed_noise_take(k, slot + NOISE_SLOT, step, v->zoom), lo = v->lo, hi = v->hi;
    if (hi >= lo)
        return lo + (lev * (hi - lo + 1) >> 8);
    return lo - (lev * (lo - hi + 1) >> 8);
}
