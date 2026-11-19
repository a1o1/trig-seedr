/* SPDX-License-Identifier: GPL-2.0-or-later */
/* props: the framework for property tests of the chain (design section 11; plan tasks 0.5, M2.2).
 * Host only (test_*.c include it). Random originals, contexts and recipes, a field-by-field diff,
 * which fields each layer owns, and checks for the invariants that hold for every recipe: I1, I2,
 * I3, I6. A layer's owner adds its own checks (I4 shares, I5 counts) in its test file with these
 * helpers; test_fuzz.c runs all of them over long runs.
 *
 * Random recipes draw from every source (TRACK, RANDOM, EUCLID, MAP, COPY), every layer and every
 * value source, shape, order and mode, but only from what is built (seed_have_*), so a test using
 * them covers more as each workstream lands, with no change to the test. */
#ifndef PROPS_H
#define PROPS_H

#include <string.h>
#include "seed.h"
#include "../device/rytm_params.h"

/* ---- random numbers for the tests (not the chain's: those are seed_rng.c's) ---- */

struct prop_rng { uint32_t s; };

static inline uint32_t prop_next(struct prop_rng *g)
{
    uint32_t x = g->s ? g->s : 0x9e3779b9u;  /* xorshift32 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g->s = x;
}

/* lo..hi inclusive */
static inline int prop_int(struct prop_rng *g, int lo, int hi) { return lo + (int)(prop_next(g) % (uint32_t)(hi - lo + 1)); }
static inline int prop_chance(struct prop_rng *g, int pct) { return prop_int(g, 0, 99) < pct; }

/* ---- the device the tests run on: the Rytm's capabilities, as tools/seedref.c (device/rytm_params.h);
 * with PROP_DEVICE_ST a Syntakt-shaped profile (the Makefiles build each test a second time with it,
 * as test_<name>_st; docs/plan-syntakt.md task S1.2) ---- */

#ifndef PROP_DEVICE_ST
static const struct seed_param prop_params[RT_NPARAMS] = RT_PARAMS;
static const struct seed_caps prop_caps = { 13, 64, RT_VOICE_FIELDS, -23, 23, SEED_LANES, RT_NPARAMS, prop_params,
                                            { RT_P_SMP, RT_P_FRQ } };
#else
#include "../device/st_params.h"
/* 12 tracks and the FX track with device/st_params.h's fields (S0.5): note, velocity, length, micro timing
 * -23..+23, condition and locks; no retrig, accent or slide. The lockable parameters: track 1's list with SP
 * TWINSHOT (type 44), the longest a track has (its 8 SYN controls, the filter, base-width, amp and LFO pages),
 * as st_track_params builds it for the device (stock's ranges, all 0 or more: TUNE 40..88). Built at start-up,
 * since st_track_params runs, as on the device. */
#define PROP_ST_FIELDS ST_VOICE_FIELDS
static struct seed_param prop_params[ST_TRACK_PARAMS_MAX];
static struct seed_caps prop_caps = { 13, 64, PROP_ST_FIELDS, ST_UTIME_LO, ST_UTIME_HI, SEED_LANES, 0, prop_params,
                                      { 0, 0 } };
__attribute__((constructor)) static void prop_st_caps(void)
{
    static const struct st_machine m[ST_NMACHINES] = ST_MACHINES;
    uint16_t ids[ST_TRACK_PARAMS_MAX];
    prop_caps.nparams = (uint8_t)st_track_params(st_machine_of(m, ST_NMACHINES, 44), 0, prop_params, ids,
                                                 prop_caps.lane_parm);
}
#endif
/* the capabilities the generators and contexts use; a test may point it at another device's
 * (test_fuzz.c) */
static const struct seed_caps *prop_cap = &prop_caps;

/* three grooves for TIME GRV: a swing, 16 steps with velocity, 7 steps without */
#define PROP_GROOVES 3
static struct seed_groove prop_grooves[PROP_GROOVES];

static inline void prop_grooves_init(void)
{
    static int done;
    struct prop_rng g = { 0x6700e5u };
    int k, i;
    if (done)
        return;
    done = 1;
    for (k = 0; k < PROP_GROOVES; k++) {
        struct seed_groove *gr = &prop_grooves[k];
        memset(gr, 0, sizeof *gr);
        gr->name[0] = (char)('A' + k);
        gr->len = (uint8_t)(k == 0 ? 2 : k == 1 ? 16 : 7);
        for (i = 0; i < gr->len; i++) {
            gr->off[i] = (int8_t)(k == 0 ? (i & 1) * 6 : prop_int(&g, -30, 30));
            gr->vel[i] = (uint8_t)(k == 1 ? prop_int(&g, 0, 127) : 0);
            if (gr->vel[i] > gr->vmax)
                gr->vmax = gr->vel[i];
        }
    }
}

/* ---- originals ---- */

/* a track as the device reads it: random trigs (about pct %), every field, some lanes, unknown
 * flag bits, origin = index; length len */
static inline void prop_track(struct seed_track *t, struct prop_rng *g, int len, int pct)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = (int16_t)len;
    t->fields = prop_cap->fields;
    t->default_note = 60;
    t->default_vel = 100;
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = (uint16_t)((prop_chance(g, pct) ? SEED_TRIG : 0) | (i & 1 ? SEED_SWING : 0)
                              | (prop_chance(g, 10) ? SEED_ACCENT : 0) | (prop_chance(g, 10) ? 0x0400u : 0));
        s->note = prop_chance(g, 50) ? SEED_DEFAULT : (uint8_t)prop_int(g, 36, 84);
        s->vel = prop_chance(g, 50) ? SEED_DEFAULT : (uint8_t)prop_int(g, 1, 127);
        s->len = prop_chance(g, 50) ? SEED_DEFAULT : (uint8_t)prop_int(g, 0, 127);
        s->utime = (int8_t)(prop_chance(g, 70) ? 0 : prop_int(g, prop_cap->utime_lo, prop_cap->utime_hi));
        s->cond = (int8_t)(prop_chance(g, 80) ? SEED_NO_COND : prop_int(g, 0, 64));
        s->rlen = SEED_NEW_RLEN;
        s->rrate = SEED_NEW_RRATE;
        s->origin = (int8_t)i;
        s->lane[0] = prop_chance(g, 70) ? SEED_NO_LOCK : (int16_t)prop_int(g, 0, 127);
        s->lane[1] = SEED_NO_LOCK;
    }
}

/* the context of track `track` with original t: the other tracks empty, the grooves above */
static inline void prop_ctx(struct seed_ctx *c, const struct seed_track *t, int track)
{
    int s;
    prop_grooves_init();
    memset(c, 0, sizeof *c);
    c->caps = prop_cap;
    c->track = track;
    c->tracks = prop_cap->tracks;
    c->length[track] = (uint8_t)t->length;
    for (s = 0; s < SEED_STEPS; s++)
        if (t->s[s].flags & SEED_TRIG)
            c->trig[track][s >> 3] |= (uint8_t)(1u << (s & 7));
    c->grooves = prop_grooves;
    c->ngrooves = PROP_GROOVES;
}

/* prop_ctx, with every other track random (length 1..64, about a random share of trigs; now and
 * then one of length 0, a track the pattern lacks) and the shared MAP settings random */
static inline void prop_ctx_full(struct seed_ctx *c, const struct seed_track *t, int track, struct prop_rng *g)
{
    int j, s;
    prop_ctx(c, t, track);
    for (j = 0; j < c->tracks; j++) {
        int pct = prop_int(g, 0, 100);
        if (j == track)
            continue;
        c->length[j] = (uint8_t)(prop_chance(g, 5) ? 0 : prop_chance(g, 50) ? 16 : prop_int(g, 1, SEED_STEPS));
        for (s = 0; s < c->length[j]; s++)
            if (prop_chance(g, pct))
                c->trig[j][s >> 3] |= (uint8_t)(1u << (s & 7));
    }
    c->shared.map_x = (int16_t)prop_int(g, 0, 255);
    c->shared.map_y = (int16_t)prop_int(g, 0, 255);
    c->shared.map_chaos = (int16_t)(prop_chance(g, 40) ? 0 : prop_int(g, 0, 255));
}

/* ---- recipes ---- */

static inline void prop_values(struct seed_values *v, struct prop_rng *g, int lo, int hi)
{
    v->lo = (int16_t)prop_int(g, lo, hi);
    v->hi = (int16_t)prop_int(g, lo, hi);
}

/* a value source over lo..hi: RANGE, or LIST or NOISE when built, with any order, list length and
 * zoom; now and then one value; DEF about def % of the time */
static inline void prop_vsrc(struct seed_values *v, struct prop_rng *g, int lo, int hi, int def)
{
    int src = prop_int(g, 0, 2);
    prop_values(v, g, lo, hi);
    if (prop_chance(g, 15))
        v->hi = v->lo;
    v->src = SEED_VSRC_RANGE;
    if (src == 1 && seed_have_list)
        v->src = SEED_VSRC_LIST;
    if (src == 2 && seed_have_noise)
        v->src = SEED_VSRC_NOISE;
    v->order = (int16_t)prop_int(g, 0, SEED_ORDR_N - 1);
    v->len = (int16_t)prop_int(g, 1, SEED_LIST_MAX);
    v->zoom = (int16_t)(prop_chance(g, 30) ? prop_int(g, 1, 4) : prop_int(g, 1, SEED_STEPS));
    if (prop_chance(g, def))
        v->lo = v->hi = SEED_DEF;
}

/* a share knob: 0 about zero % of the time, 100 now and then, else 1..100 */
static inline int16_t prop_pct(struct prop_rng *g, int zero)
{
    if (prop_chance(g, zero))
        return 0;
    return (int16_t)(prop_chance(g, 10) ? 100 : prop_int(g, 1, 100));
}

/* Layer L's settings, random, active about pct % of the time (only what is built). Values stay
 * inside the ranges seed.h gives, tracks and parameters inside prop_cap's, and the two lanes on
 * different parameters, so a render of a recipe from here on a track with every field never
 * refuses (test_fuzz.c also draws values outside them). */
static inline void prop_layer(struct seed_recipe *r, struct prop_rng *g, enum seed_layer L, int pct)
{
    int on = prop_chance(g, pct), l;
    switch (L) {
    case SEED_L_TAKE:
        r->seed = prop_chance(g, 5) ? prop_next(g) : prop_next(g) % 10000u;
        r->vary = (int16_t)(prop_chance(g, 50) ? 0 : prop_chance(g, 10) ? 100 : prop_int(g, 0, 100));
        if (on && seed_have_rank) {
            r->metr = (int16_t)(prop_chance(g, 50) ? SEED_METR_4_4 : prop_int(g, 0, SEED_METR_N - 1));
            r->beat = (int16_t)(prop_chance(g, 50) ? 4 : prop_int(g, 1, 16));
        }
        if (on && seed_have_vary) {
            r->phrs = (int16_t)(prop_chance(g, 50) ? SEED_PHRS_OFF : prop_int(g, 0, SEED_PHRS_N - 1));
            r->cvar = (int16_t)(prop_chance(g, 70) ? SEED_CVAR_OFF : prop_int(g, 0, SEED_CVAR_N - 1));
        }
        if (on && seed_have_score)
            r->qual = (int16_t)(prop_chance(g, 70) ? 1 : prop_int(g, 1, 8));
        break;
    case SEED_L_RHYTHM: {
        static const int16_t grids[4] = { 1, 2, 4, 8 };
        int src = prop_int(g, SEED_SRC_RANDOM, SEED_SRC_N - 1);
        if (!on)
            break;
        /* every source's knobs, so the other sources' settings are there too */
        r->grid = grids[prop_int(g, 0, 3)];
        r->dens = (int16_t)prop_int(g, 0, 100);
        r->bias = (int16_t)(seed_have_rank || prop_chance(g, 50) ? prop_int(g, -100, 100) : 0);
        r->role = (int16_t)(seed_have_rank ? prop_int(g, 0, SEED_ROLE_N - 1) : SEED_ROLE_METER);
        r->eu_steps = (int16_t)(prop_chance(g, 50) ? 16 : prop_int(g, 1, SEED_STEPS));
        r->eu_puls = (int16_t)prop_int(g, 0, r->eu_steps);
        r->map_part = (int16_t)prop_int(g, 0, 2);
        r->map_dens = (int16_t)prop_int(g, 0, 255);
        r->copy_trk = (int16_t)prop_int(g, 0, prop_cap->tracks - 1);
        if ((src == SEED_SRC_EUCLID && !seed_have_euclid) || (src == SEED_SRC_MAP && !seed_have_map)
            || (src == SEED_SRC_COPY && !seed_have_copy))
            src = SEED_SRC_RANDOM;
        if (src != SEED_SRC_RANDOM || seed_have_random)
            r->src = (int16_t)src;
        break;
    }
    case SEED_L_PLACE:
        if (on && seed_have_place) {
            r->rot = (int16_t)(prop_chance(g, 40) ? 0 : prop_int(g, -63, 63));
            r->skip = (int16_t)(prop_chance(g, 50) ? 0 : prop_chance(g, 80) ? prop_int(g, 0, 8) : prop_int(g, 0, 64));
            if (seed_have_sync)
                r->sync = prop_pct(g, 60);
            if (seed_have_mask && prop_chance(g, 30)) {
                r->mask = (int16_t)prop_int(g, SEED_MASK_AVOID, SEED_MASK_ONLY);
                r->mtrk = (int16_t)prop_int(g, 0, prop_cap->tracks - 1);
            }
        }
        break;
    case SEED_L_DYN:
        if (on && seed_have_dyn) {
            int shape = prop_int(g, SEED_SHAPE_FLAT, SEED_SHAPE_N - 1);
            if ((shape == SEED_SHAPE_RANK && !seed_have_rank) || (shape == SEED_SHAPE_NOISE && !seed_have_noise)
                || (shape == SEED_SHAPE_MAP && !seed_have_map))
                shape = SEED_SHAPE_STRONG;
            prop_vsrc(&r->vel, g, 1, 127, 40);
            r->shape = (int16_t)shape;
            r->amt = (int16_t)prop_int(g, 0, 100);
            r->n = (int16_t)(prop_chance(g, 50) ? prop_int(g, 1, 8) : prop_int(g, 1, SEED_STEPS));
            r->acc = prop_pct(g, 50);
            r->amod = (int16_t)(seed_have_rank && prop_chance(g, 40) ? SEED_AMOD_TOP : SEED_AMOD_RAND);
            r->hum = (int16_t)(prop_chance(g, 70) ? 0 : prop_int(g, 0, 100));
        }
        break;
    case SEED_L_NOTE:
        if (on && seed_have_values) {
            if (prop_chance(g, 50)) prop_vsrc(&r->len, g, 0, 127, 0);
            if (prop_chance(g, 50)) prop_vsrc(&r->note, g, -24, 24, 0);
            if (seed_have_list && prop_chance(g, 20))
                r->lmod = SEED_LMOD_LEGATO;
        }
        break;
    case SEED_L_LOCK:
        if (on && seed_have_values) {
            int n = prop_cap->lanes < SEED_LANES ? prop_cap->lanes : SEED_LANES;
            /* each lane on its own parameter: two on one are refused (values.c) */
            for (l = 0; l < n; l++) {
                int p, again;
                do {
                    p = prop_chance(g, 50) ? prop_cap->lane_parm[l] : prop_int(g, 0, prop_cap->nparams - 1);
                    again = l && prop_cap->nparams > 1 && p == r->lane[0].parm;
                } while (again);
                r->lane[l].parm = (int16_t)p;
                r->lane[l].pct = (int16_t)(l && prop_chance(g, 50) ? 0 : prop_pct(g, 0));
                prop_vsrc(&r->lane[l].val, g, prop_cap->params[p].lo, prop_cap->params[p].hi, 20);
            }
        }
        break;
    case SEED_L_TRIG:
        if (on && seed_have_trig) {
            r->cond = (int16_t)(prop_chance(g, 30) ? 0 : prop_int(g, 0, 100));
            r->ctyp = (int16_t)prop_int(g, 0, SEED_CTYP_N - 1);
            if (r->ctyp == SEED_CTYP_AB) { r->cval.lo = (int16_t)prop_int(g, 2, 8); r->cval.hi = r->cval.lo; }
            else prop_vsrc(&r->cval, g, 0, 21, 10);
            r->rtrg = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 100));
            prop_vsrc(&r->rate, g, 0, 16, 20);
            r->rcur = (int16_t)prop_int(g, -128, 127);
        }
        break;
    case SEED_L_TIME:
        if (on && seed_have_timing) {
            r->grv = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, PROP_GROOVES));
            r->timing = (int16_t)(prop_chance(g, 50) ? 100 : prop_int(g, -100, 100));
            r->gvel = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, -100, 100));
            r->shft = (int16_t)(prop_chance(g, 60) ? 0 : prop_int(g, -20, 20));
            r->rand = (int16_t)prop_int(g, 0, 100);
            r->quant = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 100));
            r->drft = (int16_t)(seed_have_drift && prop_chance(g, 50) ? prop_int(g, 0, 100) : 0);   /* F8 */
        }
        break;
    case SEED_L_FIELDS:
        if (on && seed_have_place) {
            r->fld = (int16_t)prop_int(g, SEED_FLD_VEL, SEED_FLD_LANE2);
            r->frot = (int16_t)(prop_chance(g, 70) ? prop_int(g, -15, 15) : prop_int(g, -63, 63));
            r->fmod = (int16_t)prop_int(g, SEED_RMODE_TRIG, SEED_RMODE_GRID);
        }
        break;
    default:
        break;
    }
}

/* a recipe from the defaults with each built layer active about pct % of the time */
static inline void prop_recipe(struct seed_recipe *r, struct prop_rng *g, int pct)
{
    int L;
    seed_recipe_defaults(r, prop_cap);
    for (L = 0; L < SEED_L_N; L++)
        prop_layer(r, g, (enum seed_layer)L, pct);
}

/* ---- comparing tracks ---- */

/* what can differ between two steps (prop_diff) */
#define PROP_TRIG   0x0001u
#define PROP_FLAGS  0x0002u           /* a SEED_KNOWN bit other than the trig */
#define PROP_VEL    0x0004u
#define PROP_NOTE   0x0008u
#define PROP_LEN    0x0010u
#define PROP_UTIME  0x0020u
#define PROP_COND   0x0040u
#define PROP_RTRG   0x0080u           /* rlen, rrate, rvel */
#define PROP_LANE1  0x0100u
#define PROP_LANE2  0x0200u
#define PROP_ORIGIN 0x0400u
#define PROP_OTHER  0x0800u           /* swing, length, the defaults: no layer changes these */
#define PROP_XFLAG  0x1000u           /* a flag bit lib/ does not know: it travels with its step */
#define PROP_ALL    0x1fffu

/* the fields that differ between a and b at any step below n (n 0: both lengths) */
static inline unsigned prop_diff(const struct seed_track *a, const struct seed_track *b, int n)
{
    unsigned d = 0;
    int i;
    if (a->length != b->length || a->fields != b->fields || a->default_note != b->default_note
        || a->default_vel != b->default_vel)
        d |= PROP_OTHER;
    if (!n)
        n = SEED_STEPS;
    for (i = 0; i < n; i++) {
        const struct seed_step *x = &a->s[i], *y = &b->s[i];
        if ((x->flags ^ y->flags) & SEED_TRIG) d |= PROP_TRIG;
        if ((x->flags ^ y->flags) & (SEED_KNOWN & ~SEED_TRIG)) d |= PROP_FLAGS;
        if ((x->flags ^ y->flags) & SEED_SWING) d |= PROP_OTHER;
        if ((x->flags ^ y->flags) & ~(SEED_KNOWN | SEED_SWING)) d |= PROP_XFLAG;
        if (x->vel != y->vel) d |= PROP_VEL;
        if (x->note != y->note) d |= PROP_NOTE;
        if (x->len != y->len) d |= PROP_LEN;
        if (x->utime != y->utime) d |= PROP_UTIME;
        if (x->cond != y->cond) d |= PROP_COND;
        if (x->rlen != y->rlen || x->rrate != y->rrate || x->rvel != y->rvel) d |= PROP_RTRG;
        if (x->lane[0] != y->lane[0]) d |= PROP_LANE1;
        if (x->lane[1] != y->lane[1]) d |= PROP_LANE2;
        if (x->origin != y->origin) d |= PROP_ORIGIN;
        if (x->spare != y->spare) d |= PROP_OTHER;
    }
    return d;
}

/* The fields a layer may change (I3, design section 4), when the source is not changed between
 * the two renders. RHYTHM and PLACE decide where steps are, so they may change everything a step
 * carries, unknown flag bits included (they move with their steps; swing stays with the position);
 * the flags of DYNAMICS are the accent, those of TRIG the retrig. TIME's GVEL scales
 * velocity (the one shared field). FIELDS rotates one field: any of the value fields. */
static inline unsigned prop_owned(enum seed_layer L)
{
    switch (L) {
    case SEED_L_RHYTHM: case SEED_L_PLACE: return PROP_ALL & ~PROP_OTHER;
    case SEED_L_DYN: return PROP_VEL | PROP_FLAGS;
    case SEED_L_NOTE: return PROP_NOTE | PROP_LEN;
    case SEED_L_LOCK: return PROP_LANE1 | PROP_LANE2;
    case SEED_L_TRIG: return PROP_COND | PROP_RTRG | PROP_FLAGS;
    case SEED_L_TIME: return PROP_UTIME | PROP_VEL;
    case SEED_L_FIELDS: return PROP_VEL | PROP_NOTE | PROP_LEN | PROP_UTIME | PROP_COND | PROP_RTRG | PROP_FLAGS
                               | PROP_LANE1 | PROP_LANE2;
    default: return PROP_ALL;
    }
}

/* ---- the checks. Each returns 1 when the invariant holds for this case; SEED_E_NA renders
 * (a layer not built) count as holding, since nothing was written. ---- */

static struct seed_work prop_work;

/* I2 and I6: rendering twice gives the same bytes; steps at and past the length are unchanged */
static inline int prop_i2_i6(const struct seed_track *orig, const struct seed_recipe *r, const struct seed_ctx *c)
{
    static struct seed_track a, b;
    int ra = seed_render(&a, orig, r, c, &prop_work), rb = seed_render(&b, orig, r, c, &prop_work), i;
    if (ra != rb)
        return 0;
    if (ra != SEED_OK)
        return 1;
    if (memcmp(&a, &b, sizeof a))
        return 0;
    for (i = orig->length; i < SEED_STEPS; i++)
        if (memcmp(&a.s[i], &orig->s[i], sizeof a.s[i]))
            return 0;
    return a.length == orig->length;
}

/* I1: SOURCE = TRACK with every layer at 0 is byte-identical, whatever the seed and VARY */
static inline int prop_i1(const struct seed_track *orig, const struct seed_ctx *c, uint32_t seed, int vary)
{
    static struct seed_track out;
    struct seed_recipe r;
    seed_recipe_defaults(&r, prop_cap);
    r.seed = seed;
    r.vary = (int16_t)vary;
    return seed_render(&out, orig, &r, c, &prop_work) == SEED_OK && !memcmp(&out, orig, sizeof out);
}

/* I3: r and the same recipe with layer L's settings drawn again differ only in L's fields.
 * *changed: the fields that did differ (0 when either render refused). */
static inline int prop_i3(const struct seed_track *orig, const struct seed_recipe *r, const struct seed_ctx *c,
                          struct prop_rng *g, enum seed_layer L, unsigned *changed)
{
    static struct seed_track a, b;
    struct seed_recipe s = *r;
    *changed = 0;
    if (L == SEED_L_TAKE || L == SEED_L_RHYTHM)
        return 1;                       /* a new take or source changes the steps themselves */
    prop_layer(&s, g, L, 100);
    if (seed_render(&a, orig, r, c, &prop_work) != SEED_OK || seed_render(&b, orig, &s, c, &prop_work) != SEED_OK)
        return 1;
    *changed = prop_diff(&a, &b, orig->length);
    return !(*changed & ~prop_owned(L));
}

#endif
