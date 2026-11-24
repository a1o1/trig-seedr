/* SPDX-License-Identifier: GPL-2.0-or-later */
/* values: value sources (RANGE) and the NOTE and LOCK stages.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * NOTE writes the trigs' length and note; LOCK owns its lanes on every step: a lane whose % is
 * above 0 is cleared, then the top % of the trigs get a value (0.3's sample locks). A value can be
 * negative, so seed_value cannot report an error: the stages check the source first. Freestanding. */
#include "seed.h"

const uint8_t seed_have_values = 1;

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static int vsrc_ok(const struct seed_values *v)
{
    return v->src == SEED_VSRC_RANGE || (v->src == SEED_VSRC_LIST && seed_have_list)
        || (v->src == SEED_VSRC_NOISE && seed_have_noise) || (v->src == SEED_VSRC_TURG && seed_have_turing);
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

/* TURG (0.9): the take's TURING value at the step, 0..65535, over the range lo..hi (either order) */
static int turg(const struct seed_values *v, const struct seed_take *k, int step)
{
    int lo = v->lo < v->hi ? v->lo : v->hi, span = (v->lo < v->hi ? v->hi - v->lo : v->lo - v->hi) + 1;
    if (!k->tur || step < 0 || step >= SEED_STEPS)
        return lo;
    return lo + (int)(((uint32_t)k->tur->val[step] * (uint32_t)span) >> 16);
}

int seed_value(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int step, int index)
{
    switch (v->src) {
    case SEED_VSRC_RANGE: return seed_rand_range(k, slot + (uint32_t)step, v->lo, v->hi);
    case SEED_VSRC_LIST:  return seed_value_list(v, k, slot, index);
    case SEED_VSRC_NOISE: return seed_value_noise(v, k, slot, step);
    case SEED_VSRC_TURG:  return turg(v, k, step);
    default:              return SEED_E_ARG;
    }
}

int seed_note(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int n = t->length, i, idx = 0;
    int len_on = r->len.lo != SEED_DEF, note_on = r->note.lo != SEED_DEF, legato = r->lmod == SEED_LMOD_LEGATO;
    (void)c;

    if (r->lmod < 0 || r->lmod >= SEED_LMOD_N)
        return SEED_E_ARG;
    if (note_on && r->scale != SEED_SCALE_OFF && (r->scale < 0 || r->scale >= SEED_SCALE_N || r->root < 0 || r->root > 11))
        return SEED_E_ARG;
    if (note_on && r->scale != SEED_SCALE_OFF && !seed_have_scale)
        return SEED_E_NA;
    if ((len_on && !legato && !vsrc_ok(&r->len)) || (note_on && !vsrc_ok(&r->note)) || (legato && !seed_have_list))
        return SEED_E_NA;
    if (!(t->fields & SEED_F_LEN))
        len_on = legato = 0;
    if (!(t->fields & SEED_F_NOTE))
        note_on = 0;

    for (i = 0; i < n; i++) {
        struct seed_step *s = &t->s[i];
        if (!(s->flags & SEED_TRIG))
            continue;
        if (legato) {
            int v = seed_len_legato(t, i);
            if (v < 0)
                return v;
            s->len = (uint8_t)clamp(v, 0, 127);
        } else if (len_on) {
            s->len = (uint8_t)clamp(seed_value(&r->len, k, SEED_SLOT(SEED_L_NOTE, SEED_P_LEN, 0), i, idx), 0, 127);
        }
        if (note_on) {
            int off = clamp(seed_value(&r->note, k, SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0), i, idx), -24, 24);
            int note = t->default_note + off;
            if (r->scale != SEED_SCALE_OFF)       /* SCALE (0.9): the nearest note of the scale on ROOT */
                note = seed_scale_note(note, r->scale, r->root);
            s->note = (uint8_t)clamp(note, 0, 126);
        }
        idx++;
    }
    return SEED_OK;
}

int seed_lock(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    uint8_t chosen[SEED_STEPS];
    int n = t->length, l, i;
    const struct seed_caps *caps = c ? c->caps : 0;

    /* refuse before anything is written */
    for (l = 0; l < SEED_LANES; l++) {
        const struct seed_lane *ln = &r->lane[l];
        int m;
        /* Two lanes on one parameter are one lock on the device, which stores one of them: the
         * take could not keep the other's values (M2.2). Refused when either lane is set. */
        for (m = 0; m < l && (!caps || l < caps->lanes); m++)
            if (r->lane[m].parm == ln->parm && (ln->pct > 0 || r->lane[m].pct > 0))
                return SEED_E_ARG;
        if (ln->pct <= 0)
            continue;
        if (caps && (l >= caps->lanes || ln->parm < 0 || ln->parm >= caps->nparams))
            return SEED_E_NA;
        if (ln->val.lo != SEED_DEF && !vsrc_ok(&ln->val))
            return SEED_E_NA;
    }
    if (!(t->fields & SEED_F_LOCKS))
        return SEED_OK;

    for (l = 0; l < SEED_LANES; l++) {
        const struct seed_lane *ln = &r->lane[l];
        struct seed_values v = ln->val;
        int plo = -32767, phi = 32767, idx = 0;
        if (ln->pct <= 0)
            continue;
        if (caps) {
            plo = caps->params[ln->parm].lo;
            phi = caps->params[ln->parm].hi;
        }
        if (v.lo == SEED_DEF) {                /* DEF: the parameter's whole range (TURG keeps its source) */
            if (v.src != SEED_VSRC_TURG)
                v.src = SEED_VSRC_RANGE;
            v.lo = (int16_t)plo;
            v.hi = (int16_t)phi;
        }
        pick(chosen, t, k, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_KEY + l, 0), ln->pct);
        for (i = 0; i < n; i++) {
            struct seed_step *s = &t->s[i];
            s->lane[l] = SEED_NO_LOCK;
            if (!(s->flags & SEED_TRIG))
                continue;
            if (chosen[i])
                s->lane[l] = (int16_t)clamp(seed_value(&v, k, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL + l, 0), i, idx),
                                            plo, phi);
            idx++;
        }
    }
    return SEED_OK;
}
