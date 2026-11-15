/* SPDX-License-Identifier: GPL-2.0-or-later */
/* dyn: DYNAMICS: VEL, the shapes, ACC, HUM; 0.3's metric weight.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * Writes the trigs' velocity and accent flag at their final positions (P5), nothing else. Each
 * part runs only when its knob is set (V4): VEL, SHAPE with AMT, and HUM write velocity; ACC
 * owns the accent flag of every trig. Velocity per trig:
 *   v = VEL's value (VEL = DEF: the trig's own velocity)
 *   v += (shape's target - v) * AMT / 100, the target at the shape's level 0..255 between VEL's
 *        low and high end (VEL = DEF: half the trig's velocity .. the trig's velocity)
 *   v += a random spread up to +-HUM_MAX * HUM / 100
 * Freestanding. */
#include "seed.h"

const uint8_t seed_have_dyn = 1;

#define HUM_MAX 32                    /* HUM 100: up to +-32 */

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* a / b rounded to nearest, halves away from zero; b > 0 */
static int rdiv(int a, int b)
{
    return a >= 0 ? (a + b / 2) / b : -((-a + b / 2) / b);
}

int seed_weight(int step, int beat)
{
    int bar;
    beat = clamp(beat, 1, 16);
    bar = 4 * beat;
    if (step < 0)
        return 0;
    if (step % bar == 0)
        return 4;
    if (step % (bar / 2) == 0)
        return 3;
    if (step % beat == 0)
        return 2;
    if (beat % 2 == 0 && step % (beat / 2) == 0)
        return 1;
    return 0;
}

/* the five-level shapes: how loud a step of weight w is, 0..4 */
static int weight_level(int shape, int w)
{
    switch (shape) {
    case SEED_SHAPE_STRONG: return w;
    case SEED_SHAPE_WEAK:   return 4 - w;
    case SEED_SHAPE_OFF:    return w == 1 ? 4 : w == 0 ? 2 : 0;
    case SEED_SHAPE_NOT1:   return w >= 2 ? 0 : 4;
    default:                return w == 4 ? 0 : 4;     /* SEED_SHAPE_NOTBAR */
    }
}

/* the shape's level at the trig `index` on `step`, 0..255, or < 0 */
static int shape_level(const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c,
                       int step, int index, int n)
{
    switch (r->shape) {
    case SEED_SHAPE_STRONG: case SEED_SHAPE_WEAK: case SEED_SHAPE_OFF: case SEED_SHAPE_NOT1: case SEED_SHAPE_NOTBAR:
        if (r->metr != SEED_METR_4_4)
            return SEED_E_NA;                  /* the weights are 4/4's; other meters come with F1 */
        return weight_level(r->shape, seed_weight(step, r->beat)) * 255 / 4;
    case SEED_SHAPE_RANK:
        return seed_shape_rank(r, step, n);
    case SEED_SHAPE_NOISE:
        return seed_shape_noise(r, k, step);
    case SEED_SHAPE_EVRY:
        return index % clamp(r->n, 1, SEED_STEPS) == 0 ? 255 : 0;
    case SEED_SHAPE_MAP:
        return seed_map_level(r->map_part, c->shared.map_x, c->shared.map_y, step);
    default:
        return SEED_E_ARG;
    }
}

static int vsrc_ok(const struct seed_values *v)
{
    return v->src == SEED_VSRC_RANGE || (v->src == SEED_VSRC_LIST && seed_have_list)
        || (v->src == SEED_VSRC_NOISE && seed_have_noise);
}

/* the top pct % of the trigs by their keys from slot (P2): chosen[i] = 1 */
static void pick(uint8_t *chosen, const struct seed_track *t, const struct seed_take *k, uint32_t slot, int pct)
{
    uint32_t key[SEED_STEPS];
    uint8_t elig[SEED_STEPS], order[SEED_STEPS];
    int i, m, n = t->length;
    for (i = 0; i < n; i++) {
        elig[i] = (t->s[i].flags & SEED_TRIG) != 0;
        chosen[i] = 0;
    }
    seed_keys(key, n, k, slot, 0, 0);
    m = seed_share(pct, seed_order(order, key, elig, n));
    for (i = 0; i < m; i++)
        chosen[order[i]] = 1;
}

int seed_dyn(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    uint8_t chosen[SEED_STEPS];
    int n = t->length, i, idx = 0, rc;
    int vel_on = r->vel.lo != SEED_DEF, shape_on = r->shape != SEED_SHAPE_FLAT && r->amt > 0;
    int amt = clamp(r->amt, 0, 100), hum = rdiv(clamp(r->hum, 0, 100) * HUM_MAX, 100);

    /* refuse before anything is written */
    if (vel_on && !vsrc_ok(&r->vel))
        return SEED_E_NA;
    if (shape_on && (rc = shape_level(r, k, c, 0, 0, n)) < 0)
        return rc;
    if (r->acc > 0 && r->amod != SEED_AMOD_RAND && r->amod != SEED_AMOD_TOP)
        return SEED_E_ARG;
    if (r->acc > 0 && r->amod == SEED_AMOD_TOP && !seed_have_rank)
        return SEED_E_NA;

    if ((t->fields & SEED_F_VEL) && (vel_on || shape_on || hum))
        for (i = 0; i < n; i++) {
            struct seed_step *s = &t->s[i];
            int base, lo, hi, v;
            if (!(s->flags & SEED_TRIG))
                continue;
            base = s->vel == SEED_DEFAULT ? t->default_vel : s->vel;
            if (vel_on) {
                lo = clamp(r->vel.lo, 1, 127);
                hi = clamp(r->vel.hi, 1, 127);
                v = clamp(seed_value(&r->vel, k, SEED_SLOT(SEED_L_DYN, SEED_P_VEL, 0), i, idx), 1, 127);
            } else {
                lo = (base + 1) / 2;
                hi = v = base;
            }
            if (shape_on) {
                int lev = shape_level(r, k, c, i, idx, n), to;
                if (lev < 0)
                    return lev;
                if (hi < lo) {
                    int x = lo;
                    lo = hi;
                    hi = x;
                }
                to = lo + rdiv((hi - lo) * clamp(lev, 0, 255), 255);
                v += rdiv((to - v) * amt, 100);
            }
            if (hum)
                v += seed_rand_range(k, SEED_SLOT(SEED_L_DYN, SEED_P_HUM, i), -hum, hum);
            s->vel = (uint8_t)clamp(v, 1, 127);
            idx++;
        }

    if (r->acc > 0 && (t->fields & SEED_F_ACCENT)) {
        if (r->amod == SEED_AMOD_TOP)
            return seed_acc_top(t, r, k, c);
        pick(chosen, t, k, SEED_SLOT(SEED_L_DYN, SEED_P_ACC, 0), r->acc);
        for (i = 0; i < n; i++)
            if (t->s[i].flags & SEED_TRIG)
                t->s[i].flags = (uint16_t)(chosen[i] ? t->s[i].flags | SEED_ACCENT : t->s[i].flags & ~SEED_ACCENT);
    }
    return SEED_OK;
}
