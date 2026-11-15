/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Helpers for the host tests of task 0.2's modules: checks, a pseudo-random source of test data,
 * tracks, a device's capabilities and context, and random recipes over the layers 0.2 built.
 * Host only (it uses the C library); everything is static, so each test includes its own copy. */
#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <string.h>
#include "seed.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static uint32_t lcg = 12345;
static inline int rnd(int lo, int hi)        /* lo..hi */
{
    lcg = lcg * 1103515245u + 12345u;
    return lo + (int)((lcg >> 8) % (uint32_t)(hi - lo + 1));
}

static const struct seed_param test_params[2] = { { "SMPL", 19, 0, 127 }, { "TUNE", 1, -64, 63 } };
static struct seed_caps test_caps;
static struct seed_groove test_grooves[3];
static struct seed_work work;

static inline void test_setup(void)
{
    int g, i;
    memset(&test_caps, 0, sizeof test_caps);
    test_caps.tracks = 12;
    test_caps.steps = 64;
    test_caps.fields = 0x1ff;
    test_caps.utime_lo = -23;
    test_caps.utime_hi = 23;
    test_caps.lanes = 2;
    test_caps.nparams = 2;
    test_caps.params = test_params;
    test_caps.lane_parm[0] = 0;
    test_caps.lane_parm[1] = 1;
    for (g = 0; g < 3; g++) {
        struct seed_groove *gr = &test_grooves[g];
        memset(gr, 0, sizeof *gr);
        gr->len = (uint8_t)(g == 0 ? 2 : g == 1 ? 16 : 7);
        for (i = 0; i < gr->len; i++) {
            gr->off[i] = (int8_t)(g == 0 ? (i & 1) * 5 : rnd(-30, 30));
            gr->vel[i] = (uint8_t)(g == 2 ? 0 : rnd(0, 127));
            if (gr->vel[i] > gr->vmax)
                gr->vmax = gr->vel[i];
        }
    }
}

static inline struct seed_ctx test_ctx(int track)
{
    struct seed_ctx c;
    memset(&c, 0, sizeof c);
    c.caps = &test_caps;
    c.track = track;
    c.tracks = 1;
    c.length[0] = 16;
    c.grooves = test_grooves;
    c.ngrooves = 3;
    return c;
}

/* a track as the device would read it, with random content; origin = index */
static inline void random_track(struct seed_track *t, int len)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = (int16_t)len;
    t->fields = 0x1ff;
    t->default_note = (uint8_t)rnd(24, 96);
    t->default_vel = (uint8_t)rnd(1, 127);
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = (uint16_t)(rnd(0, 2) == 0 ? SEED_TRIG : 0);
        if (rnd(0, 3) == 0) s->flags |= SEED_SWING;
        if (rnd(0, 5) == 0) s->flags |= SEED_ACCENT;
        if (rnd(0, 7) == 0) s->flags |= SEED_RETRIG;
        if (rnd(0, 9) == 0) s->flags |= 0x0400u;
        s->note = (uint8_t)(rnd(0, 3) ? SEED_DEFAULT : rnd(0, 126));
        s->vel = (uint8_t)(rnd(0, 3) ? SEED_DEFAULT : rnd(1, 127));
        s->len = (uint8_t)(rnd(0, 3) ? SEED_DEFAULT : rnd(0, 127));
        s->utime = (int8_t)(rnd(0, 2) ? 0 : rnd(-23, 23));
        s->cond = (int8_t)(rnd(0, 3) ? SEED_NO_COND : rnd(0, 64));
        s->rlen = (uint8_t)rnd(0, 127);
        s->rrate = (uint8_t)rnd(0, 16);
        s->rvel = (int8_t)rnd(-128, 127);
        s->origin = (int8_t)i;
        s->lane[0] = (int16_t)(rnd(0, 2) ? SEED_NO_LOCK : rnd(0, 127));
        s->lane[1] = (int16_t)(rnd(0, 2) ? SEED_NO_LOCK : rnd(-64, 63));
    }
}

static inline void random_values(struct seed_values *v, int lo, int hi, int def_ok)
{
    v->src = SEED_VSRC_RANGE;
    v->order = SEED_ORDR_UP;
    v->len = 4;
    v->zoom = 4;
    if (def_ok && rnd(0, 2) == 0) {
        v->lo = v->hi = SEED_DEF;
        return;
    }
    v->lo = (int16_t)rnd(lo, hi);
    v->hi = (int16_t)rnd(lo, hi);
}

/* each layer's settings at random, over what 0.2 built (RANDOM or TRACK, 4/4, METER role) */
static inline void random_rhythm(struct seed_recipe *r)
{
    static const int16_t grids[4] = { 1, 2, 4, 8 };
    r->src = rnd(0, 1) ? SEED_SRC_RANDOM : SEED_SRC_TRACK;
    r->grid = grids[rnd(0, 3)];
    r->dens = (int16_t)rnd(0, 100);
    r->bias = (int16_t)(rnd(0, 2) ? 0 : rnd(-100, 100));
    r->beat = (int16_t)rnd(1, 8);
}

static inline void random_place(struct seed_recipe *r)
{
    r->rot = (int16_t)(rnd(0, 1) ? 0 : rnd(-63, 63));
    r->skip = (int16_t)(rnd(0, 1) ? 0 : rnd(0, 20));
}

static inline void random_fields(struct seed_recipe *r)
{
    r->fld = (int16_t)(rnd(0, 1) ? SEED_FLD_OFF : rnd(SEED_FLD_VEL, SEED_FLD_LANE2));
    r->frot = (int16_t)rnd(-63, 63);
    r->fmod = (int16_t)rnd(0, SEED_RMODE_N - 1);
}

static inline void random_dyn(struct seed_recipe *r)
{
    static const int16_t shapes[7] = { SEED_SHAPE_FLAT, SEED_SHAPE_STRONG, SEED_SHAPE_WEAK, SEED_SHAPE_OFF,
                                       SEED_SHAPE_NOT1, SEED_SHAPE_NOTBAR, SEED_SHAPE_EVRY };
    random_values(&r->vel, 1, 127, 1);
    r->shape = shapes[rnd(0, 6)];
    r->amt = (int16_t)(rnd(0, 2) ? rnd(0, 100) : 0);
    r->n = (int16_t)rnd(1, 8);
    r->acc = (int16_t)(rnd(0, 1) ? rnd(0, 100) : 0);
    r->amod = SEED_AMOD_RAND;
    r->hum = (int16_t)(rnd(0, 2) ? 0 : rnd(0, 100));
}

static inline void random_note(struct seed_recipe *r)
{
    random_values(&r->len, 0, 127, 1);
    random_values(&r->note, -24, 24, 1);
    r->lmod = SEED_LMOD_RANGE;
}

static inline void random_lock(struct seed_recipe *r)
{
    int l;
    for (l = 0; l < SEED_LANES; l++) {
        r->lane[l].parm = (int16_t)l;
        r->lane[l].pct = (int16_t)(rnd(0, 1) ? rnd(0, 100) : 0);
        random_values(&r->lane[l].val, l ? -64 : 0, l ? 63 : 127, 1);
    }
}

static inline void random_trig(struct seed_recipe *r)
{
    r->cond = (int16_t)(rnd(0, 1) ? rnd(0, 100) : 0);
    r->ctyp = (int16_t)rnd(0, SEED_CTYP_N - 1);
    random_values(&r->cval, r->ctyp == SEED_CTYP_AB ? 2 : 0, r->ctyp == SEED_CTYP_AB ? 8 : 21, 0);
    r->rtrg = (int16_t)(rnd(0, 1) ? rnd(0, 100) : 0);
    random_values(&r->rate, 0, 16, 1);
    r->rcur = (int16_t)rnd(-128, 127);
}

static inline void random_time(struct seed_recipe *r)
{
    r->grv = (int16_t)rnd(0, 3);
    r->timing = (int16_t)rnd(-100, 100);
    r->gvel = (int16_t)rnd(-100, 100);
    r->quant = (int16_t)(rnd(0, 1) ? 0 : rnd(0, 100));
    r->rand = (int16_t)(rnd(0, 1) ? 0 : rnd(0, 100));
    r->shft = (int16_t)rnd(-20, 20);
}

static inline void random_recipe(struct seed_recipe *r)
{
    seed_recipe_defaults(r, &test_caps);
    r->seed = (uint32_t)rnd(0, 9999);
    r->vary = (int16_t)(rnd(0, 1) ? 0 : rnd(0, 100));
    random_rhythm(r);
    random_place(r);
    random_dyn(r);
    random_note(r);
    random_lock(r);
    random_trig(r);
    random_time(r);
    random_fields(r);
}

static inline int ntrigs(const struct seed_track *t)
{
    int i, m = 0;
    for (i = 0; i < t->length; i++)
        m += (t->s[i].flags & SEED_TRIG) != 0;
    return m;
}

#endif
