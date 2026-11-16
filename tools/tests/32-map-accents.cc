/* SPDX-License-Identifier: GPL-3.0-or-later */
/* mapacc: AMOD = TOP with SRC = MAP (lib/dyn_rank.c) against the original code's accents, on the
 * host. Built by tools/tests/32-map-accents.sh as 23-map.cc is: MAP_RESOURCES is
 * eurorack/grids/resources.cc, MAP_READER the lines of pattern_generator.cc from drum_map[5][5] to
 * the end of ReadDrumMap. Under GPL-3.0-or-later as it links them; a test tool, never part of a
 * build of the mod.
 *
 * The original's accent rule (EvaluateDrums): a part's step fires when its level, plus the part's
 * perturbation for the pattern (U8U8MulShift8(byte, randomness >> 2)) and clipped to 255, is above
 * ~density; a step that fires is accented when that level is above 192. 32-map-accents.sh checks
 * pattern_generator.cc still says so.
 *
 *   mapacc                                   for 6 x 6 X / Y, 3 parts, every DENS, 5 CHAOS, lengths
 *                                            16, 32 and 64, VARY 0 and 60: seed_render with SRC = MAP,
 *                                            AMOD = TOP and ACC at the share that counts the original's
 *                                            accents gives exactly them; prints PASS or FAIL and counts
 *   mapacc PART DENS X Y CHAOS SEED LEN      the original's accents for one take: "acc P" (that ACC
 *                                            share) and "accents x . ..." (one mark per step) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

/* the original's trig and accent for step i: 0 none, 1 a trig, 2 an accented trig. The byte is the
 * one the source draws for the part's pass (the original draws one per pattern start). */
static int original(const struct seed_take *k, int part, int dens, int x, int y, int chaos, int i)
{
    uint8_t byte = (uint8_t)(seed_rand(k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_CHAOS, part * 256 + i / 32)) >> 24);
    uint8_t pert = grids::U8U8MulShift8(byte, (uint8_t)(chaos >> 2));
    uint8_t level = grids::PatternGenerator::ReadDrumMap((uint8_t)(i % 32), (uint8_t)part, (uint8_t)x, (uint8_t)y);
    uint8_t threshold = (uint8_t)~(uint8_t)dens;
    if (level < 255 - pert)
        level += pert;
    else
        level = 255;
    if (level > threshold)
        return level > 192 ? 2 : 1;
    return 0;
}

/* the smallest ACC % whose share of n trigs is m, or -1 */
static int acc_for(int m, int n)
{
    for (int p = 0; p <= 100; p++)
        if (seed_share(p, n) == m)
            return p;
    return -1;
}

static struct seed_track orig, out;
static struct seed_work work;
static struct seed_ctx c;

static void setup(int len)
{
    memset(&orig, 0, sizeof orig);
    orig.length = (int16_t)len;
    orig.fields = 0x1ff;
    orig.default_note = 60;
    orig.default_vel = 100;
    for (int i = 0; i < SEED_STEPS; i++) {
        orig.s[i].note = orig.s[i].vel = orig.s[i].len = SEED_DEFAULT;
        orig.s[i].cond = SEED_NO_COND;
        orig.s[i].origin = (int8_t)i;
        orig.s[i].lane[0] = orig.s[i].lane[1] = SEED_NO_LOCK;
    }
    memset(&c, 0, sizeof c);
}

int main(int argc, char **argv)
{
    struct seed_recipe r;
    struct seed_take k;

    if (argc == 8) {
        int part = atoi(argv[1]), dens = atoi(argv[2]), x = atoi(argv[3]), y = atoi(argv[4]), chaos = atoi(argv[5]);
        int len = atoi(argv[7]), m = 0, n = 0, i, want[SEED_STEPS];
        seed_take_init(&k, (uint32_t)strtoul(argv[6], 0, 10), 0, 0, 0);
        for (i = 0; i < len; i++) {
            want[i] = original(&k, part, dens, x, y, chaos, i);
            n += want[i] != 0;
            m += want[i] == 2;
        }
        printf("acc %d\naccents", acc_for(m, n));
        for (i = 0; i < len; i++)
            printf(" %s", want[i] == 2 ? "x" : ".");
        printf("\n");
        return 0;
    }

    static const int xs[] = { 0, 40, 100, 128, 200, 255 }, chaos[] = { 0, 3, 4, 100, 255 }, lens[] = { 16, 32, 64 },
                     vary[] = { 0, 60 };
    long cases = 0, steps = 0, bad = 0, nofit = 0, some = 0;
    for (int xi = 0; xi < 6; xi++)
        for (int yi = 0; yi < 6; yi++)
            for (int p = 0; p < 3; p++)
                for (int d = 0; d < 256; d++)
                    for (int ci = 0; ci < 5; ci++)
                        for (int li = 0; li < 3; li++)
                            for (int vi = 0; vi < 2; vi++) {
                                int x = xs[xi], y = xs[5 - yi], n = lens[li], m = 0, t = 0, want[SEED_STEPS], pct;
                                uint32_t seed = 1u + (uint32_t)(cases % 13);
                                setup(n);
                                seed_recipe_defaults(&r, NULL);
                                r.src = SEED_SRC_MAP;
                                r.map_part = (int16_t)p;
                                r.map_dens = (int16_t)d;
                                r.amod = SEED_AMOD_TOP;
                                r.seed = seed;
                                r.vary = (int16_t)vary[vi];
                                c.shared.map_x = (int16_t)x;
                                c.shared.map_y = (int16_t)y;
                                c.shared.map_chaos = (int16_t)chaos[ci];
                                seed_take_init(&k, seed, 0, vary[vi], 0);
                                for (int i = 0; i < n; i++) {
                                    want[i] = original(&k, p, d, x, y, chaos[ci], i);
                                    t += want[i] != 0;
                                    m += want[i] == 2;
                                }
                                cases++;
                                if ((pct = acc_for(m, t)) < 0) {
                                    nofit++;
                                    continue;
                                }
                                some += m > 0 && m < t;
                                r.acc = (int16_t)pct;
                                if (seed_render(&out, &orig, &r, &c, &work) != SEED_OK) {
                                    bad++;
                                    continue;
                                }
                                for (int i = 0; i < n; i++) {
                                    int got = out.s[i].flags & SEED_TRIG ? out.s[i].flags & SEED_ACCENT ? 2 : 1 : 0;
                                    steps++;
                                    if (got != want[i] && bad++ < 5)
                                        printf("  x %d y %d part %d dens %d chaos %d len %d vary %d seed %u acc %d step %d: "
                                               "want %d got %d\n", x, y, p, d, chaos[ci], n, vary[vi], (unsigned)seed, pct, i,
                                               want[i], got);
                                }
                            }
    printf("%s accents: %ld takes, %ld steps compared, %ld differ, %ld without an ACC share\n",
           bad || nofit ? "FAIL" : "PASS", cases, steps, bad, nofit);
    printf("takes with some but not all trigs accented: %ld\n", some);
    return bad || nofit;
}
