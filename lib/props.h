/* SPDX-License-Identifier: GPL-2.0-or-later */
/* props: the framework for property tests of the chain (design section 11; plan task 0.5).
 * Host only (test_*.c include it). Random originals and recipes, a field-by-field diff, which
 * fields each layer owns, and checks for the invariants that hold for every recipe: I1, I2, I3,
 * I6. A layer's owner adds its own checks (I4 shares, I5 counts) in its test file with these
 * helpers.
 *
 * Random recipes only turn on layers whose feature is built (seed_have_*), so a test using them
 * covers more as each workstream lands, with no change to the test. */
#ifndef PROPS_H
#define PROPS_H

#include <string.h>
#include "seed.h"

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

/* ---- the Rytm's capabilities, as tools/seedref.c ---- */

static const struct seed_param prop_params[] = { { "SMPL", 19, 0, 127 } };
static const struct seed_caps prop_caps = { 13, 64, 0x1ff, -23, 23, SEED_LANES, 1, prop_params, { 0, 0 } };

/* ---- originals ---- */

/* a track as the device reads it: random trigs (about pct %), every field, some lanes, unknown
 * flag bits, origin = index; length len */
static inline void prop_track(struct seed_track *t, struct prop_rng *g, int len, int pct)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = (int16_t)len;
    t->fields = prop_caps.fields;
    t->default_note = 60;
    t->default_vel = 100;
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = (uint16_t)((prop_chance(g, pct) ? SEED_TRIG : 0) | (i & 1 ? SEED_SWING : 0)
                              | (prop_chance(g, 10) ? SEED_ACCENT : 0) | (prop_chance(g, 10) ? 0x0400u : 0));
        s->note = prop_chance(g, 50) ? SEED_DEFAULT : (uint8_t)prop_int(g, 36, 84);
        s->vel = prop_chance(g, 50) ? SEED_DEFAULT : (uint8_t)prop_int(g, 1, 127);
        s->len = prop_chance(g, 50) ? SEED_DEFAULT : (uint8_t)prop_int(g, 0, 127);
        s->utime = (int8_t)(prop_chance(g, 70) ? 0 : prop_int(g, -23, 23));
        s->cond = (int8_t)(prop_chance(g, 80) ? SEED_NO_COND : prop_int(g, 0, 64));
        s->rlen = SEED_NEW_RLEN;
        s->rrate = SEED_NEW_RRATE;
        s->origin = (int8_t)i;
        s->lane[0] = prop_chance(g, 70) ? SEED_NO_LOCK : (int16_t)prop_int(g, 0, 127);
        s->lane[1] = SEED_NO_LOCK;
    }
}

static inline void prop_ctx(struct seed_ctx *c, const struct seed_track *t, int track)
{
    int s;
    memset(c, 0, sizeof *c);
    c->caps = &prop_caps;
    c->track = track;
    c->tracks = prop_caps.tracks;
    c->length[track] = (uint8_t)t->length;
    for (s = 0; s < SEED_STEPS; s++)
        if (t->s[s].flags & SEED_TRIG)
            c->trig[track][s >> 3] |= (uint8_t)(1u << (s & 7));
}

/* ---- recipes ---- */

static inline void prop_values(struct seed_values *v, struct prop_rng *g, int lo, int hi)
{
    v->lo = (int16_t)prop_int(g, lo, hi);
    v->hi = (int16_t)prop_int(g, lo, hi);
}

/* layer L's settings, random, active about pct % of the time (only when L is built) */
static inline void prop_layer(struct seed_recipe *r, struct prop_rng *g, enum seed_layer L, int pct)
{
    int on = prop_chance(g, pct);
    switch (L) {
    case SEED_L_TAKE:
        r->seed = prop_next(g) % 10000u;
        r->vary = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 100));
        break;
    case SEED_L_RHYTHM:
        if (on && seed_have_random) {
            static const int16_t grids[4] = { 1, 2, 4, 8 };
            r->src = SEED_SRC_RANDOM;
            r->grid = grids[prop_int(g, 0, 3)];
            r->dens = (int16_t)prop_int(g, 0, 100);
            r->bias = (int16_t)(seed_have_rank || prop_chance(g, 50) ? prop_int(g, -100, 100) : 0);
        }
        break;
    case SEED_L_PLACE:
        if (on && seed_have_place) {
            r->rot = (int16_t)prop_int(g, -20, 20);
            r->skip = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 8));
        }
        break;
    case SEED_L_DYN:
        if (on && seed_have_dyn) {
            prop_values(&r->vel, g, 1, 127);
            r->shape = (int16_t)prop_int(g, SEED_SHAPE_FLAT, SEED_SHAPE_NOTBAR);
            r->amt = (int16_t)prop_int(g, 0, 100);
            r->acc = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 100));
            r->hum = (int16_t)(prop_chance(g, 70) ? 0 : prop_int(g, 0, 100));
        }
        break;
    case SEED_L_NOTE:
        if (on && seed_have_values) {
            if (prop_chance(g, 50)) prop_values(&r->len, g, 0, 127);
            if (prop_chance(g, 50)) prop_values(&r->note, g, -24, 24);
        }
        break;
    case SEED_L_LOCK:
        if (on && seed_have_values) {
            r->lane[0].pct = (int16_t)prop_int(g, 1, 100);
            prop_values(&r->lane[0].val, g, 0, 127);
        }
        break;
    case SEED_L_TRIG:
        if (on && seed_have_trig) {
            r->cond = (int16_t)(prop_chance(g, 30) ? 0 : prop_int(g, 0, 100));
            r->ctyp = (int16_t)prop_int(g, SEED_CTYP_PROB, SEED_CTYP_FILL);
            if (r->ctyp == SEED_CTYP_AB) { r->cval.lo = (int16_t)prop_int(g, 2, 8); r->cval.hi = r->cval.lo; }
            else prop_values(&r->cval, g, 0, 21);
            r->rtrg = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 100));
            prop_values(&r->rate, g, 0, 16);
            r->rcur = (int16_t)prop_int(g, -128, 127);
        }
        break;
    case SEED_L_TIME:
        if (on && seed_have_timing) {
            r->rand = (int16_t)prop_int(g, 0, 100);
            r->quant = (int16_t)(prop_chance(g, 50) ? 0 : prop_int(g, 0, 100));
        }
        break;
    case SEED_L_FIELDS:
        if (on && seed_have_place) {
            r->fld = (int16_t)prop_int(g, SEED_FLD_VEL, SEED_FLD_LANE1);
            r->frot = (int16_t)prop_int(g, -15, 15);
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
    seed_recipe_defaults(r, &prop_caps);
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
#define PROP_OTHER  0x0800u           /* a flag bit lib/ does not know, swing, length, the defaults */
#define PROP_ALL    0x0fffu

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
        if ((x->flags ^ y->flags) & ~SEED_KNOWN) d |= PROP_OTHER;
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
 * carries; the flags of DYNAMICS are the accent, those of TRIG the retrig. TIME's GVEL scales
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
    seed_recipe_defaults(&r, &prop_caps);
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
