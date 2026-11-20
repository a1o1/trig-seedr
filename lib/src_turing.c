/* SPDX-License-Identifier: GPL-2.0-or-later */
/* src_turing: TURING (0.9, docs/design-turing.md). A loop of cells, each a trig bit and a value
 * 0..65535, that changes as it comes round, by one of three flavours (§2):
 *   DJVU  after the déjà vu loop of Mutable Instruments' Marbles (random_sequence.h, MIT, Emilie
 *         Gillet): with p = (2 AMT - 1)^2, AMT below 50 replaces the cell about to play with a new
 *         one with chance p (0: always new, 50: the loop repeats); above 50 the step jumps to a
 *         random cell with chance p (the same cells, shuffled).
 *   CRPT  after the CORRUPT knob of Moog's Labyrinth, from its manual: a cell plays, then the write
 *         head on it redraws an on cell's value with chance 0..25 % (AMT 0..50) or 25..50 % (50..100)
 *         and, above 50, flips any cell's bit with chance 0..50 %; a cell flipped on gets a new value.
 *   SHFT  the classic shift register (the Music Thing Turing Machine): the bit that comes round is
 *         inverted with chance 100 - AMT % (0: always, so the loop doubles; 50: a coin flip; 100:
 *         locked); the value is the eight newest bits as a byte, as an 8-bit DAC.
 * One step plays one cell; the offline source (SOURCE = TURING) runs the machine over the track,
 * so a track longer than the loop holds pass after pass. The machine is counter based like every
 * other layer: step `count`'s draws are the take's numbers at slots (RHYTHM, TURING, draw << 12 |
 * count), so VARY, seeds and QUAL's candidates behave as for the other sources. Freestanding. */
#include "seed.h"

const uint8_t seed_have_turing = 1;

/* a step's draws (index = draw << 12 | count) */
enum { D_DECIDE, D_NEWBIT, D_NEWVAL, D_JUMP, D_REVAL, D_REVALNEW, D_FLIP, D_FLIPVAL, D_INITBIT, D_INITVAL,
       D_SHFT };

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static uint32_t draw(const struct seed_take *k, int d, int count)
{
    return seed_rand(k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_TURING, (uint32_t)d << 12 | ((uint32_t)count & 0xfffu)));
}

static int knobs_ok(const struct seed_recipe *r)
{
    return r->tur_flav >= 0 && r->tur_flav < SEED_TURF_N && r->tur_loop >= 1 && r->tur_loop <= SEED_TUR_LOOP
        && r->tur_amt >= 0 && r->tur_amt <= 100 && r->tur_bias >= 0 && r->tur_bias <= 100
        && r->tur_strt >= 0 && r->tur_strt < SEED_TURS_N;
}

int seed_turing_used(const struct seed_recipe *r)
{
    int i;
    if (r->src == SEED_SRC_TURING || r->vel.src == SEED_VSRC_TURG || r->len.src == SEED_VSRC_TURG
        || r->note.src == SEED_VSRC_TURG || r->cval.src == SEED_VSRC_TURG || r->rate.src == SEED_VSRC_TURG)
        return 1;
    for (i = 0; i < SEED_LANES; i++)
        if (r->lane[i].val.src == SEED_VSRC_TURG)
            return 1;
    return 0;
}

/* an original step's note as a cell value: its offset from the track's note, -24..+24, over
 * 0..65535 (the default note, or none, is the middle) */
static uint16_t note_value(const struct seed_track *t, int i)
{
    const struct seed_step *s = &t->s[i];
    int off;
    if (!(s->flags & SEED_TRIG) || s->note == SEED_DEFAULT)
        return 32768u;
    off = clamp((int)s->note - (int)t->default_note, -24, 24);
    return (uint16_t)((off + 24) * 65535 / 48);
}

int seed_turing_init(struct seed_turing *m, const struct seed_recipe *r, const struct seed_track *orig,
                     const struct seed_take *k)
{
    int c, len = clamp(orig->length, 1, SEED_STEPS);
    if (!knobs_ok(r))
        return SEED_E_ARG;
    m->flav = r->tur_flav;
    m->loop = (int16_t)(r->tur_flav == SEED_TURF_SHFT ? clamp(r->tur_loop, 2, SEED_TUR_LOOP) : r->tur_loop);
    m->amt = r->tur_amt;
    m->bias = r->tur_bias;
    m->pos = 0;
    for (c = 0; c < SEED_TUR_LOOP; c++) {
        if (r->tur_strt == SEED_TURS_TRACK && c < len) {
            m->bit[c] = (orig->s[c].flags & SEED_TRIG) != 0;
            m->val[c] = note_value(orig, c);
        } else {
            m->bit[c] = (int)(draw(k, D_INITBIT, c) % 100u) < m->bias;
            m->val[c] = (uint16_t)(draw(k, D_INITVAL, c) & 0xffffu);
        }
    }
    return SEED_OK;
}

int seed_turing_step(struct seed_turing *m, const struct seed_take *k, int count, uint16_t *val)
{
    int L = clamp(m->loop, 1, SEED_TUR_LOOP), p = ((int)m->pos % L + L) % L, a = clamp(m->amt, 0, 100), bit;
    switch (m->flav) {
    case SEED_TURF_DJVU: {
        int x = 2 * a - 100;            /* p = (2 AMT - 1)^2, in 1/10000 */
        if ((int)(draw(k, D_DECIDE, count) % 10000u) < x * x) {
            if (a <= 50) {              /* a new cell where the loop plays */
                m->bit[p] = (int)(draw(k, D_NEWBIT, count) % 100u) < m->bias;
                m->val[p] = (uint16_t)(draw(k, D_NEWVAL, count) & 0xffffu);
            } else {                    /* a jump inside the loop */
                p = (int)(draw(k, D_JUMP, count) % (uint32_t)L);
            }
        }
        bit = m->bit[p];
        *val = m->val[p];
        break;
    }
    case SEED_TURF_CRPT: {
        int pv = a <= 50 ? a * 50 : 2500 + (a - 50) * 50;   /* 0..25 %, then 25..50 %, in 1/10000 */
        bit = m->bit[p];
        *val = m->val[p];
        if (m->bit[p] && (int)(draw(k, D_REVAL, count) % 10000u) < pv)
            m->val[p] = (uint16_t)(draw(k, D_REVALNEW, count) & 0xffffu);
        if (a > 50 && (int)(draw(k, D_FLIP, count) % 100u) < a - 50) {
            m->bit[p] ^= 1u;
            if (m->bit[p])
                m->val[p] = (uint16_t)(draw(k, D_FLIPVAL, count) & 0xffffu);
        }
        break;
    }
    default: {                          /* SHFT */
        unsigned byte = 0;
        int i;
        for (i = 0; i < 8; i++)
            byte = byte << 1 | m->bit[((p - i) % L + L) % L];
        bit = m->bit[p];
        *val = (uint16_t)(byte * 257u);
        if ((int)(draw(k, D_SHFT, count) % 100u) < 100 - a)
            m->bit[p] ^= 1u;
        break;
    }
    }
    m->pos = (int16_t)((p + 1) % L);
    return bit;
}

int seed_turing_run(struct seed_tur *out, const struct seed_track *orig, const struct seed_recipe *r,
                    const struct seed_take *k)
{
    struct seed_turing m;
    int i, rc = seed_turing_init(&m, r, orig, k), len = clamp(orig->length, 1, SEED_STEPS);
    if (rc < 0)
        return rc;
    for (i = 0; i < SEED_STEPS; i++) {
        out->bit[i] = 0;
        out->val[i] = 32768u;
    }
    for (i = 0; i < len; i++)
        out->bit[i] = (uint8_t)seed_turing_step(&m, k, i, &out->val[i]);
    return SEED_OK;
}

int seed_src_turing(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k,
                    const struct seed_ctx *c)
{
    int i;
    (void)r;
    (void)c;
    if (!k->tur)
        return SEED_E_NA;
    for (i = 0; i < t->length; i++)
        if (k->tur->bit[i])
            t->s[i].flags |= SEED_TRIG;
    return SEED_OK;
}
