/* SPDX-License-Identifier: GPL-2.0-or-later */
/* src_map: the MAP source and SHAPE = MAP's levels (map data in map_data.c, GPL-3.0-or-later).
 * Owner: task F3 (docs/plan-v1.md section 4).
 * The map is 5 x 5 nodes, each 3 parts x 32 steps of levels 0..255. X and Y (0..255, the
 * pattern's, struct seed_shared) pick a cell of four nodes (X / 64, Y / 64) and blend them by
 * their remainders, as the original's ReadDrumMap does with its 8-bit mix:
 *   mix(a, b, w) = (a * (255 - w) + b * w) / 256, w = (X * 4) mod 256 (Y for the second blend)
 *   level = mix(mix(a, b, wx), mix(c, d, wx), wy), a b at (i, j) (i + 1, j), c d at (i, j + 1) (i + 1, j + 1)
 * Steps beyond 32 repeat the 32. A step's key is its level plus the part's chaos offset for its
 * pass of 32 steps: (r * (CHAOS / 4)) / 256 with r a random byte from the slot (SEED_P_CHAOS,
 * part * 256 + pass), at most 255. The step fires when its key exceeds 255 - DENS, so raising
 * DENS only adds trigs (I4) and DENS 0 gives none. Freestanding. */
#include "seed.h"

const uint8_t seed_have_map = 1;

#define MAP_STEPS 32
#define MAP_PARTS 3

/* map_data.c */
extern const uint8_t seed_map_node[25][MAP_PARTS * MAP_STEPS];
extern const uint8_t seed_map_place[5][5];

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static int mix(int a, int b, int w)
{
    return (a * (255 - w) + b * w) >> 8;
}

int seed_map_level(int part, int x, int y, int step)
{
    int i, j, wx, wy, o;
    if (part < 0 || part >= MAP_PARTS || step < 0)
        return SEED_E_ARG;
    x = clamp(x, 0, 255);
    y = clamp(y, 0, 255);
    i = x >> 6;
    j = y >> 6;
    wx = (x << 2) & 0xff;
    wy = (y << 2) & 0xff;
    o = part * MAP_STEPS + step % MAP_STEPS;
    return mix(mix(seed_map_node[seed_map_place[i][j]][o], seed_map_node[seed_map_place[i + 1][j]][o], wx),
               mix(seed_map_node[seed_map_place[i][j + 1]][o], seed_map_node[seed_map_place[i + 1][j + 1]][o], wx),
               wy);
}

/* the part's chaos offset for the pass of 32 steps holding step (P3: one slot per part per pass);
 * also AMOD = TOP's key with SRC = MAP (dyn_rank.c) */
int seed_map_lift(const struct seed_take *k, int part, int step, int chaos)
{
    int r;
    if (part < 0 || part >= MAP_PARTS || step < 0)
        return SEED_E_ARG;
    chaos = clamp(chaos, 0, 255) >> 2;
    if (chaos == 0)
        return 0;
    r = (int)(seed_rand(k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_CHAOS, part * 256 + step / MAP_STEPS)) >> 24);
    return r * chaos >> 8;
}

int seed_src_map(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int n = t->length, part = r->map_part, i, pert = 0;
    int threshold = 255 - clamp(r->map_dens, 0, 255);

    if (part < 0 || part >= MAP_PARTS)
        return SEED_E_ARG;
    for (i = 0; i < n; i++) {
        int key = seed_map_level(part, c->shared.map_x, c->shared.map_y, i);
        if (i % MAP_STEPS == 0)
            pert = seed_map_lift(k, part, i, c->shared.map_chaos);
        key = key < 255 - pert ? key + pert : 255;
        if (key > threshold)
            t->s[i].flags |= SEED_TRIG;
    }
    return SEED_OK;
}
