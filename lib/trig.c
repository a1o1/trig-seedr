/* SPDX-License-Identifier: GPL-2.0-or-later */
/* trig: TRIG: conditions and retrigs.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * COND above 0 owns the trigs' conditions: the top COND % of the trigs get one (CTYP, CVAL), the
 * others none. RTRG above 0 owns the retrig flag: the top RTRG % get it with a rate from RATE and
 * the curve RCUR; the others lose it and keep their retrig settings. Freestanding. */
#include "seed.h"

const uint8_t seed_have_trig = 1;

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
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

int seed_cond_ab(int a, int b)
{
    b = clamp(b, 2, 8);
    a = clamp(a, 1, b);
    return 29 + b * (b - 1) / 2 + a - 1;      /* 1:2 = 30, 1:4 = 35, 1:8 = 57, 8:8 = 64 */
}

static int make_cond(const struct seed_recipe *r, const struct seed_take *k, int step, int index)
{
    int ct = r->ctyp;
    if (ct == SEED_CTYP_MIX)
        ct = seed_rand_range(k, SEED_SLOT(SEED_L_TRIG, SEED_P_COND_TYPE, step), SEED_CTYP_PROB, SEED_CTYP_FILL);
    switch (ct) {
    case SEED_CTYP_AB: {
        int b = clamp(r->cval.lo, 2, 8);
        return seed_cond_ab(seed_rand_range(k, SEED_SLOT(SEED_L_TRIG, SEED_P_COND_VAL, step), 1, b), b);
    }
    case SEED_CTYP_1ST:
        return 28;
    case SEED_CTYP_FILL:
        return 22;
    default: {
        struct seed_values v = r->cval;
        if (v.lo == SEED_DEF) {                /* DEF: the recipe's default, 50% .. 100% */
            v.src = SEED_VSRC_RANGE;
            v.lo = 10;
            v.hi = 21;
        }
        return clamp(seed_value(&v, k, SEED_SLOT(SEED_L_TRIG, SEED_P_COND_VAL, 0), step, index), 0, 21);
    }
    }
}

int seed_trig(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    uint8_t chosen[SEED_STEPS];
    int n = t->length, i, idx;
    (void)c;

    /* refuse before anything is written */
    if (r->cond > 0 && (r->ctyp < 0 || r->ctyp >= SEED_CTYP_N))
        return SEED_E_ARG;
    if (r->cond > 0 && r->ctyp != SEED_CTYP_AB && r->cval.lo != SEED_DEF && !vsrc_ok(&r->cval))
        return SEED_E_NA;
    if (r->rtrg > 0 && r->rate.lo != SEED_DEF && !vsrc_ok(&r->rate))
        return SEED_E_NA;

    if (r->cond > 0 && (t->fields & SEED_F_COND)) {
        pick(chosen, t, k, SEED_SLOT(SEED_L_TRIG, SEED_P_COND_KEY, 0), r->cond);
        for (i = idx = 0; i < n; i++) {
            struct seed_step *s = &t->s[i];
            if (!(s->flags & SEED_TRIG))
                continue;
            s->cond = (int8_t)(chosen[i] ? make_cond(r, k, i, idx) : SEED_NO_COND);
            idx++;
        }
    }
    if (r->rtrg > 0 && (t->fields & SEED_F_RETRIG)) {
        struct seed_values v = r->rate;
        if (v.lo == SEED_DEF) {                /* DEF: the rate a new trig gets */
            v.src = SEED_VSRC_RANGE;
            v.lo = v.hi = SEED_NEW_RRATE;
        }
        pick(chosen, t, k, SEED_SLOT(SEED_L_TRIG, SEED_P_RTRG_KEY, 0), r->rtrg);
        for (i = idx = 0; i < n; i++) {
            struct seed_step *s = &t->s[i];
            if (!(s->flags & SEED_TRIG))
                continue;
            if (chosen[i]) {
                s->flags |= SEED_RETRIG;
                s->rrate = (uint8_t)clamp(seed_value(&v, k, SEED_SLOT(SEED_L_TRIG, SEED_P_RTRG_RATE, 0), i, idx), 0, 16);
                s->rvel = (int8_t)clamp(r->rcur, -128, 127);
            } else {
                s->flags &= (uint16_t)~SEED_RETRIG;
            }
            idx++;
        }
    }
    return SEED_OK;
}
