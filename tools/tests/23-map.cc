/* SPDX-License-Identifier: GPL-3.0-or-later */
/* mapcmp: lib/src_map.c against the original code it follows, on the host. Built by
 * tools/tests/23-map.sh with the original files compiled in unchanged: MAP_RESOURCES is
 * eurorack/grids/resources.cc (the nodes), MAP_READER the lines of pattern_generator.cc from
 * drum_map[5][5] to the end of ReadDrumMap (the places and the blend). Under GPL-3.0-or-later as it
 * links them; it is a test tool, never part of a build of the mod.
 *
 * The original's 8-bit helpers come from avrlib (avrlib/op.h), which is not in the workspace's
 * checkout (an empty submodule). Their portable C definitions are written out below:
 *   U8Mix(a, b, w)        the high byte of a * (255 - w) + b * w
 *   U8U8MulShift8(a, b)   the high byte of a * b
 *
 * Checks, each printing PASS or FAIL and a count:
 *   levels  seed_map_level = ReadDrumMap for every X, Y 0..255, part 0..2 and step 0..63 (step mod 32)
 *   trigs   seed_src_map's trigs = EvaluateDrums' rule (level + perturbation, clipped to 255, above
 *           ~density) for every DENS on a set of X / Y and CHAOS, with the perturbation the
 *           original computes from the same random byte (U8U8MulShift8(byte, CHAOS >> 2))
 * and prints the FNV-1a hashes lib/test_src_map.c holds (the same order as its tables). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef char prog_char;
typedef uint8_t prog_uint8_t;
typedef uint16_t prog_uint16_t;
typedef uint32_t prog_uint32_t;
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))

namespace grids {

const uint8_t kNumParts = 3;
const uint8_t kStepsPerPattern = 32;

static inline uint8_t U8Mix(uint8_t a, uint8_t b, uint8_t balance)
{
    return (uint8_t)((a * (255 - balance) + b * balance) >> 8);
}

static inline uint8_t U8U8MulShift8(uint8_t a, uint8_t b)
{
    return (uint8_t)((a * b) >> 8);
}

struct PatternGenerator {
    static uint8_t ReadDrumMap(uint8_t step, uint8_t instrument, uint8_t x, uint8_t y);
};

}  // namespace grids

#include MAP_RESOURCES

namespace grids {
#include MAP_READER
}  // namespace grids

extern "C" {
#include "seed.h"
}

static uint32_t fnv(uint32_t h, uint8_t b)
{
    return (h ^ b) * 16777619u;
}

/* the X / Y lib/test_src_map.c holds a hash for */
static const uint8_t pts[] = { 0, 1, 32, 63, 64, 65, 100, 127, 128, 160, 191, 192, 224, 254, 255 };
#define NPTS (int)(sizeof pts)

int main(void)
{
    int x, y, p, s, bad = 0, n = 0, fails = 0;
    uint32_t all = 2166136261u;

    for (x = 0; x < 256; x++)
        for (y = 0; y < 256; y++)
            for (p = 0; p < 3; p++)
                for (s = 0; s < 64; s++) {
                    int want = grids::PatternGenerator::ReadDrumMap((uint8_t)(s % 32), (uint8_t)p, (uint8_t)x, (uint8_t)y);
                    int got = seed_map_level(p, x, y, s);
                    n++;
                    if (got != want && bad++ < 5)
                        printf("  x %d y %d part %d step %d: want %d got %d\n", x, y, p, s, want, got);
                    if (s < 32)
                        all = fnv(all, (uint8_t)want);
                }
    printf("%s levels: %d compared, %d differ\n", bad ? "FAIL" : "PASS", n, bad);
    fails += bad != 0;

    /* trigs: the source on a 64-step track (two passes) against the original's rule */
    {
        static const int xs[] = { 0, 40, 128, 200, 255 }, chaos[] = { 0, 3, 4, 100, 255 };
        struct seed_track t;
        struct seed_recipe r;
        struct seed_ctx c;
        struct seed_take k;
        int xi, yi, ci, d, i;
        bad = n = 0;
        memset(&c, 0, sizeof c);
        for (xi = 0; xi < 5; xi++)
            for (yi = 0; yi < 5; yi++)
                for (ci = 0; ci < 5; ci++)
                    for (p = 0; p < 3; p++)
                        for (d = 0; d < 256; d++) {
                            seed_recipe_defaults(&r, NULL);
                            r.src = SEED_SRC_MAP;
                            r.map_part = (int16_t)p;
                            r.map_dens = (int16_t)d;
                            c.shared.map_x = (int16_t)xs[xi];
                            c.shared.map_y = (int16_t)xs[4 - yi];
                            c.shared.map_chaos = (int16_t)chaos[ci];
                            seed_take_init(&k, 7u + (uint32_t)d, 3, 0, 0);
                            memset(&t, 0, sizeof t);
                            t.length = 64;
                            if (seed_src_map(&t, &r, &k, &c) != SEED_OK) {
                                bad++;
                                continue;
                            }
                            for (i = 0; i < 64; i++) {
                                uint8_t byte = (uint8_t)(seed_rand(&k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_CHAOS, p * 256 + i / 32)) >> 24);
                                uint8_t pert = grids::U8U8MulShift8(byte, (uint8_t)(chaos[ci] >> 2));
                                uint8_t level = grids::PatternGenerator::ReadDrumMap((uint8_t)(i % 32), (uint8_t)p,
                                                                                     (uint8_t)xs[xi], (uint8_t)xs[4 - yi]);
                                uint8_t threshold = (uint8_t)~(uint8_t)d;
                                int want, got = (t.s[i].flags & SEED_TRIG) != 0;
                                if (level < 255 - pert)
                                    level += pert;
                                else
                                    level = 255;
                                want = level > threshold;
                                n++;
                                if (want != got && bad++ < 5)
                                    printf("  x %d y %d chaos %d part %d dens %d step %d: want %d got %d\n",
                                           xs[xi], xs[4 - yi], chaos[ci], p, d, i, want, got);
                            }
                        }
        printf("%s trigs: %d steps compared, %d differ\n", bad ? "FAIL" : "PASS", n, bad);
        fails += bad != 0;
    }

    /* the hashes for lib/test_src_map.c: every level, then one per X of pts over every Y of pts */
    printf("hash all 0x%08x\n", all);
    for (x = 0; x < NPTS; x++) {
        uint32_t h = 2166136261u;
        for (y = 0; y < NPTS; y++)
            for (p = 0; p < 3; p++)
                for (s = 0; s < 32; s++)
                    h = fnv(h, grids::PatternGenerator::ReadDrumMap((uint8_t)s, (uint8_t)p, pts[x], pts[y]));
        printf("hash x %d 0x%08x\n", pts[x], h);
    }
    return fails != 0;
}
