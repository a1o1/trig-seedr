/* SPDX-License-Identifier: GPL-2.0-or-later */
/* scale: NOTE's SCALE and ROOT (0.9, docs/design-turing.md L9). A note is moved to the nearest note
 * of the scale built on the root, the lower one on a tie. The scales are the modes and pentatonics
 * plus the sets Labyrinth's quantizer adds (its manual, p. 38: Hirajoshi, 7sus4, major 7th, minor
 * 7th); DIM is the whole-half diminished scale. CHR, every semitone, leaves a note as OFF does.
 * Freestanding. */
#include "seed.h"

const uint8_t seed_have_scale = 1;

/* the scale's notes as bits 0..11, semitones above the root */
#define S(a, b, c, d, e, f, g, h) ((1u << (a)) | (1u << (b)) | (1u << (c)) | (1u << (d)) | (1u << (e)) | \
                                   (1u << (f)) | (1u << (g)) | (1u << (h)))
static const uint16_t masks[SEED_SCALE_N] = {
    0x0fff,                         /* OFF */
    0x0fff,                         /* CHR */
    S(0, 2, 4, 5, 7, 9, 11, 0),     /* MAJ */
    S(0, 2, 3, 5, 7, 8, 10, 0),     /* MIN, natural */
    S(0, 2, 3, 5, 7, 9, 10, 0),     /* DOR */
    S(0, 1, 3, 5, 7, 8, 10, 0),     /* PHR */
    S(0, 2, 4, 6, 7, 9, 11, 0),     /* LYD */
    S(0, 2, 4, 5, 7, 9, 10, 0),     /* MIX */
    S(0, 1, 3, 5, 6, 8, 10, 0),     /* LOC */
    S(0, 2, 3, 5, 7, 8, 11, 0),     /* HMIN */
    S(0, 2, 3, 5, 7, 9, 11, 0),     /* MMIN, ascending */
    S(0, 2, 4, 7, 9, 0, 0, 0),      /* PMAJ */
    S(0, 3, 5, 7, 10, 0, 0, 0),     /* PMIN */
    S(0, 2, 4, 6, 8, 10, 0, 0),     /* WHOLE */
    S(0, 2, 3, 5, 6, 8, 9, 11),     /* DIM, whole-half */
    S(0, 2, 3, 7, 8, 0, 0, 0),      /* HIRA, Hirajoshi */
    S(0, 5, 7, 10, 0, 0, 0, 0),     /* 7SUS4: 1 4 5 b7 */
    S(0, 4, 7, 11, 0, 0, 0, 0),     /* MAJ7: 1 3 5 7 */
    S(0, 3, 7, 10, 0, 0, 0, 0),     /* MIN7: 1 b3 5 b7 */
};

static int in_scale(int note, unsigned mask, int root)
{
    int deg = ((note - root) % 12 + 12) % 12;
    return (mask >> deg) & 1;
}

int seed_scale_note(int note, int scale, int root)
{
    unsigned mask;
    int d;
    if (scale < 0 || scale >= SEED_SCALE_N || root < 0 || root > 11)
        return SEED_E_ARG;
    mask = masks[scale];
    for (d = 0; d < 12; d++) {
        if (in_scale(note - d, mask, root))
            return note - d;
        if (in_scale(note + d, mask, root))
            return note + d;
    }
    return note;
}
