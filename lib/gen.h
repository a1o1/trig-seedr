/* SPDX-License-Identifier: GPL-2.0-or-later */
/* gen: offline pattern algorithms (GENERATOR, ROTATE) on a device-independent track.
 *
 * No device code and no C library: the same file builds for the host (tests) and for a ColdFire
 * mod. A device layer copies a track into `struct gen_track`, runs an algorithm, and writes the
 * result back through the device's own routines (on the Rytm: the stock step setters).
 *
 * Values use the Analog Rytm MKII's encodings, which the Syntakt and Analog Four share or map
 * onto (requirements-seedr-rytm.md, rytm-step-layout-os174.md):
 *   flags  the step's flag word; the bits below are the Rytm's, others are kept as they are
 *   note   absolute MIDI note, or GEN_DEFAULT (the track's default note)
 *   vel    1..127, or GEN_DEFAULT
 *   len    the Rytm's stepped length code 0..127 (14 = 1/16, 127 = INF), or GEN_DEFAULT
 *   utime  micro timing, -23..+23 (1/24 of a step)
 *   cond   trig condition code 0..64 (0..21 = 1%..100%, 22 FILL, 28 1ST, 30..64 A:B), GEN_NO_COND
 *   rlen, rrate, rvel  retrig length 0..127, rate 0..16 (9 = 1/16), velocity curve -128..127
 *   slock  sample-slot lock: GEN_NO_LOCK, or the slot 0..127 (0 = OFF); a parameter lock on the
 *          Rytm (sound index 19), the others are trig fields
 */
#ifndef GEN_H
#define GEN_H

#include <stdint.h>

#define GEN_STEPS 64

#define GEN_TRIG   0x0001u
#define GEN_RETRIG 0x0002u
#define GEN_MUTE   0x0004u
#define GEN_ACCENT 0x0008u
#define GEN_SWING  0x0010u
#define GEN_SLIDE  0x0020u

#define GEN_DEFAULT 0xffu        /* note, vel, len: the track's default */
#define GEN_NO_COND (-1)
#define GEN_NO_LOCK (-1)

/* the values a new trig gets on the Rytm (place-trig routine 0x400c2620) */
#define GEN_NEW_RLEN  46
#define GEN_NEW_RRATE 9

struct gen_step {
    uint16_t flags;
    uint8_t note, vel, len;
    int8_t utime, cond;
    uint8_t rlen, rrate;
    int8_t rvel;
    int16_t slock;
};

struct gen_track {
    int length;                   /* steps in play, 1..64 */
    uint8_t default_note;         /* for NOTE offsets */
    struct gen_step s[GEN_STEPS];
};

/* ---- random numbers: xorshift32, seeded; never 0 ---- */
struct gen_rng { uint32_t x; };
void gen_seed(struct gen_rng *r, uint32_t seed);
uint32_t gen_next(struct gen_rng *r);
int gen_range(struct gen_rng *r, int lo, int hi);      /* lo..hi inclusive, either order */

/* ---- GENERATOR ---- */
enum gen_ctyp { GEN_CT_PROB, GEN_CT_AB, GEN_CT_1ST, GEN_CT_FILL, GEN_CT_MIX };

/* velocity shapes (METER page): which steps play loudest within the VEL range */
enum gen_vshp {
    GEN_VS_FLAT,    /* no shape: velocity random within VEL */
    GEN_VS_STRONG,  /* by metric weight: the bar's first step loudest, then the half bar, beats, "&" */
    GEN_VS_WEAK,    /* the weights inverted: the "e" and "a" loudest, the bar's first step softest */
    GEN_VS_OFF,     /* the "&" loudest, the "e" and "a" in the middle, the beats softest */
    GEN_VS_NOT1,    /* every beat soft, the rest loud */
    GEN_VS_NOTBAR,  /* the bar's first step soft, the rest loud */
    GEN_VS_N
};

/* The metric weight of step i (0-based) with `beat` steps per beat and 4 beats per bar:
 * 4 the bar's first step, 3 the half bar, 2 a beat, 1 the half beat (the "&", even beats only),
 * 0 the rest. */
int gen_weight(int i, int beat);

struct gen_params {
    /* page 1 */
    int grid;                     /* 1, 2, 4 or 8 steps */
    int dens;                     /* 0..100 % of grid positions that get a trig */
    int vel_lo, vel_hi;           /* 1..127 */
    int len_lo, len_hi;           /* length codes 0..127; both GEN_DEFAULT: leave the default */
    int note_lo, note_hi;         /* -24..+24 from the track's default note; 0, 0: leave the default */
    int utim;                     /* 0..100 % of +-23 */
    uint32_t seed;                /* any; the device turns FREE into a fresh seed */
    int rot;                      /* rotate the result by this many steps (negative: earlier) */
    /* page 2 */
    int acc;                      /* 0..100 % of trigs that get an accent */
    int cond;                     /* 0..100 % of trigs that get a condition */
    enum gen_ctyp ctyp;
    int cval_lo, cval_hi;         /* PROB: probability codes 0..21; AB: the cycle B, 2..8 (lo used) */
    int rtrg;                     /* 0..100 % of trigs that get a retrig */
    int rate_lo, rate_hi;         /* retrig rate codes 0..16 */
    int rcur;                     /* retrig velocity curve -128..127 */
    /* page 3: locks */
    int smp;                      /* 0..100 % of trigs that get a sample lock; 0: sample locks
                                     are left as they are, above 0: GEN replaces them */
    int smp_lo, smp_hi;           /* sample slots 0..127 */
    /* page 4: meter. The weights, SKIP and the velocity shape refer to the steps as written,
     * after ROT. At the defaults GENERATOR gives the same takes as before the page existed. */
    int beat;                     /* steps per beat, 1..16 (4: 1/16 steps in 4/4) */
    int skip;                     /* 0..64: the first `skip` steps are cleared and get no trig */
    int bias;                     /* -100..100: trig placement, + favours strong steps, - weak */
    enum gen_vshp vshp;
    int vamt;                     /* 0..100 % of the velocity shape */
};

void gen_defaults(struct gen_params *p);

/* Replaces every step below t->length. Keeps each step's swing bit and any flag bits outside
 * the ones gen knows (GEN_KEEP). Returns the number of trigs written. */
int gen_generate(struct gen_track *t, const struct gen_params *p);

/* ---- ROTATE ---- */
enum gen_what {
    GEN_W_ALL,      /* whole steps, swing stays (as the Rytm's trig shift) */
    GEN_W_VEL, GEN_W_NOTE, GEN_W_LEN, GEN_W_UTIM, GEN_W_COND,
    GEN_W_RTRG,     /* retrig on/off, length, rate, velocity curve together */
    GEN_W_ACC       /* the accent bit, by grid step */
};
enum gen_rmode { GEN_R_TRIG, GEN_R_GRID };

/* Rotates steps [from, from + n) of t by k (positive: later), wrapping inside that range.
 * GEN_R_TRIG: a field moves between the trigs of the range, in order (nothing is lost).
 * GEN_R_GRID: a field moves by steps; a value landing on a step without a trig is dropped and
 * that trig keeps its value. GEN_W_ALL and GEN_W_ACC always move by steps. */
void gen_rotate(struct gen_track *t, int from, int n, int k, enum gen_what what, enum gen_rmode mode);

/* condition code for A:B */
int gen_cond_ab(int a, int b);

#endif
