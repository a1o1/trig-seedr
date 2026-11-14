/* SPDX-License-Identifier: GPL-2.0-or-later */
/* groove: apply a groove (micro timing and velocity per step) to a gen_track, after Ableton
 * Live's Groove Pool (Quantize, Timing, Random, Velocity). Freestanding, like gen.c.
 *
 * A groove is a table per step: µtime at 100 % in 1/24-step units, and the groove's velocity at
 * that step (0: no groove note there, the step's velocity stays). tools/groove2h.py makes the
 * tables from Ableton .agr files and from swing amounts. One step = one table entry; the table
 * repeats every `len` steps. */
#ifndef GROOVE_H
#define GROOVE_H

#include "gen.h"

#define GEN_GROOVE_NAME 21

struct gen_groove {
    char name[GEN_GROOVE_NAME];   /* screen name, upper case, up to 20 characters */
    uint8_t len;                  /* steps in one cycle, 1..64 */
    uint8_t vmax;                 /* the loudest velocity in vel[] (0: no velocity data) */
    int8_t off[GEN_STEPS];        /* µtime at 100 %, -127..127 (clamped to +-23 when applied) */
    uint8_t vel[GEN_STEPS];       /* 0, or 1..127 */
};

struct gen_groove_params {
    int timing;                   /* -100..100 %: share of the groove's µtime added */
    int velocity;                 /* -100..100 %: how far velocities follow the groove's */
    int quant;                    /* 0..100 %: existing µtime pulled to 0 before the groove */
    int random;                   /* 0..100 %: random µtime, up to +-GEN_GROOVE_RAND at 100 % */
    int shift;                    /* the groove moved this many steps later (negative: earlier) */
    uint32_t seed;                /* for random */
};

#define GEN_GROOVE_RAND 12        /* half a step */

/* Applies g to the trigs among steps [from, from + n) of t. Steps without a trig are left alone.
 * def_vel: the track's default velocity, for trigs whose velocity is GEN_DEFAULT (written as a
 * value once velocity changes it). Returns the number of trigs changed. */
int gen_groove_apply(struct gen_track *t, int from, int n, const struct gen_groove *g,
                     const struct gen_groove_params *p, uint8_t def_vel);

#endif
