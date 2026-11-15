/* SPDX-License-Identifier: GPL-2.0-or-later */
/* noise.h: noise.c's helper for dyn_noise.c (task F7). Not part of the contract (seed.h). */
#ifndef NOISE_H
#define NOISE_H

#include "seed.h"

/* the take's noise level 0..255 at step, with `zoom` steps per cell; corner c's level is the take's
 * random number of slot + c (P1, P3) */
int seed_noise_take(const struct seed_take *k, uint32_t slot, int step, int zoom);

#endif
