/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seed_render: the chain (design section 4). Owner: the lead. One recipe per take (V2): every
 * render starts from the original track. The stages live in the files seed.h names; this file
 * decides which run, in which order, and how PHRS, CVAR and QUAL combine takes. Freestanding. */
#include "seed.h"

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void values_def(struct seed_values *v)
{
    v->lo = v->hi = SEED_DEF;
    v->src = SEED_VSRC_RANGE;
    v->order = SEED_ORDR_UP;
    v->len = 4;
    v->zoom = 4;
}

static void values_range(struct seed_values *v, int lo, int hi)
{
    values_def(v);
    v->lo = (int16_t)lo;
    v->hi = (int16_t)hi;
}

void seed_recipe_defaults(struct seed_recipe *r, const struct seed_caps *caps)
{
    int i;
    /* TAKE */
    r->seed = 1;
    r->vary = 0;
    r->phrs = SEED_PHRS_OFF;
    r->cvar = SEED_CVAR_OFF;
    r->metr = SEED_METR_4_4;
    r->beat = 4;
    r->qual = 1;
    /* RHYTHM */
    r->src = SEED_SRC_TRACK;
    r->grid = 1;
    r->dens = 50;
    r->bias = 0;
    r->role = SEED_ROLE_METER;
    r->eu_steps = 16;
    r->eu_puls = 4;
    r->map_part = 0;
    r->map_dens = 128;
    r->copy_trk = 0;
    /* PLACE */
    r->rot = 0;
    r->sync = 0;
    r->mask = SEED_MASK_OFF;
    r->mtrk = 0;
    r->skip = 0;
    r->fld = SEED_FLD_OFF;
    r->frot = 0;
    r->fmod = SEED_RMODE_TRIG;
    /* DYNAMICS */
    values_def(&r->vel);
    r->shape = SEED_SHAPE_FLAT;
    r->amt = 0;
    r->n = 4;
    r->acc = 0;
    r->amod = SEED_AMOD_RAND;
    r->hum = 0;
    /* NOTE */
    values_def(&r->len);
    r->lmod = SEED_LMOD_RANGE;
    values_def(&r->note);
    /* LOCK: the range is there for when % is raised; % 0 leaves the lane */
    for (i = 0; i < SEED_LANES; i++) {
        r->lane[i].parm = caps ? caps->lane_parm[i] : 0;
        r->lane[i].pct = 0;
        values_range(&r->lane[i].val, 1, 8);
    }
    /* TRIG */
    r->cond = 0;
    r->ctyp = SEED_CTYP_PROB;
    values_range(&r->cval, 10, 21);       /* 50% .. 100% */
    r->rtrg = 0;
    values_range(&r->rate, SEED_NEW_RRATE, SEED_NEW_RRATE);
    r->rcur = 0;
    /* TIME */
    r->grv = 0;
    r->timing = 100;
    r->gvel = 0;
    r->quant = 0;
    r->rand = 0;
    r->drft = 0;
    r->shft = 0;
}

void seed_recipe_init(struct seed_recipe *r, const struct seed_track *t, const struct seed_recipe *last,
                      const struct seed_caps *caps)
{
    int i, n = clamp(t->length, 1, SEED_STEPS), any = 0;
    seed_recipe_defaults(r, caps);
    for (i = 0; i < n; i++)
        any |= t->s[i].flags & SEED_TRIG;
    r->src = any ? SEED_SRC_TRACK : SEED_SRC_RANDOM;
    if (last) {
        r->metr = last->metr;
        r->beat = last->beat;
    }
}

static int values_on(const struct seed_values *v)
{
    return v->lo != SEED_DEF;
}

int seed_layer_active(const struct seed_recipe *r, enum seed_layer layer)
{
    int i;
    switch (layer) {
    case SEED_L_TAKE:
        return r->phrs != SEED_PHRS_OFF || r->cvar != SEED_CVAR_OFF || r->qual > 1;
    case SEED_L_RHYTHM:
        return r->src != SEED_SRC_TRACK;
    case SEED_L_PLACE:
        return r->rot != 0 || r->sync > 0 || r->mask != SEED_MASK_OFF || r->skip > 0;
    case SEED_L_DYN:
        return values_on(&r->vel) || (r->shape != SEED_SHAPE_FLAT && r->amt > 0) || r->acc > 0 || r->hum > 0;
    case SEED_L_NOTE:
        return values_on(&r->len) || r->lmod != SEED_LMOD_RANGE || values_on(&r->note);
    case SEED_L_LOCK:
        for (i = 0; i < SEED_LANES; i++)
            if (r->lane[i].pct > 0)
                return 1;
        return 0;
    case SEED_L_TRIG:
        return r->cond > 0 || r->rtrg > 0;
    case SEED_L_TIME:
        return r->grv > 0 || r->quant > 0 || r->rand > 0 || r->drft > 0;
    case SEED_L_FIELDS:
        return r->fld != SEED_FLD_OFF && r->frot != 0;
    default:
        return 0;
    }
}

void seed_step_clear(struct seed_step *s)
{
    s->flags &= (uint16_t)~SEED_KNOWN;
    s->note = s->vel = s->len = SEED_DEFAULT;
    s->utime = 0;
    s->cond = SEED_NO_COND;
    s->rlen = SEED_NEW_RLEN;
    s->rrate = SEED_NEW_RRATE;
    s->rvel = 0;
    s->origin = SEED_NO_ORIGIN;
}

void seed_track_copy(struct seed_track *to, const struct seed_track *from)
{
    int i;
    to->length = from->length;
    to->fields = from->fields;
    to->default_note = from->default_note;
    to->default_vel = from->default_vel;
    for (i = 0; i < SEED_STEPS; i++)
        to->s[i] = from->s[i];
}

/* run a stage when its part of the recipe is active */
#define STAGE(on, fn) do { if (on) { int rc_ = fn(t, r, k, c); if (rc_ < 0) return rc_; } } while (0)

/* one take, one candidate: the chain from the original into t */
static int chain(struct seed_track *t, const struct seed_track *orig, const struct seed_recipe *r,
                 const struct seed_ctx *c, uint32_t n, int cand)
{
    struct seed_take take, *k = &take;
    int i, len;

    seed_track_copy(t, orig);
    t->length = (int16_t)(len = clamp(orig->length, 1, SEED_STEPS));
    seed_take_init(k, n, (uint32_t)c->track, r->vary, cand);
    k->length = len;

    /* RHYTHM. SOURCE = TRACK: the original's steps as they are, each its own origin. */
    if (r->src == SEED_SRC_TRACK) {
        for (i = 0; i < len; i++)
            t->s[i].origin = (int8_t)i;
    } else {
        for (i = 0; i < len; i++)
            seed_step_clear(&t->s[i]);
        switch (r->src) {
        case SEED_SRC_RANDOM: STAGE(1, seed_src_random); break;
        case SEED_SRC_EUCLID: STAGE(1, seed_src_euclid); break;
        case SEED_SRC_MAP:    STAGE(1, seed_src_map); break;
        case SEED_SRC_COPY:   STAGE(1, seed_src_copy); break;
        default: return SEED_E_ARG;
        }
    }

    /* PLACE: ROT -> SYNC -> MASK -> SKIP. Nothing after this creates or moves trigs, so SKIP and
     * MASK hold on the result and DYNAMICS sees the final positions (P5). */
    STAGE(r->rot != 0, seed_place_rot);
    STAGE(r->sync > 0, seed_place_sync);
    STAGE(r->mask != SEED_MASK_OFF, seed_place_mask);
    STAGE(r->skip > 0, seed_place_skip);

    STAGE(seed_layer_active(r, SEED_L_DYN), seed_dyn);
    STAGE(seed_layer_active(r, SEED_L_NOTE), seed_note);
    STAGE(seed_layer_active(r, SEED_L_LOCK), seed_lock);
    STAGE(seed_layer_active(r, SEED_L_TRIG), seed_trig);
    STAGE(seed_layer_active(r, SEED_L_TIME), seed_time);
    /* the field rotation moves values the layers above have written */
    STAGE(seed_layer_active(r, SEED_L_FIELDS), seed_place_fields);
    return SEED_OK;
}

/* QUAL: the best of r->qual candidates of take n (ties: the lower candidate), into t */
static int best(struct seed_track *t, const struct seed_track *orig, const struct seed_recipe *r,
                const struct seed_ctx *c, uint32_t n, struct seed_track *scratch)
{
    int rc, q = clamp(r->qual, 1, 8), top, i;
    rc = chain(t, orig, r, c, n, 0);
    if (rc < 0 || q == 1)
        return rc;
    top = seed_score(t, r, c);
    if (top < 0)
        return top;
    for (i = 1; i < q; i++) {
        int s;
        rc = chain(scratch, orig, r, c, n, i);
        if (rc < 0)
            return rc;
        s = seed_score(scratch, r, c);
        if (s < 0)
            return s;
        if (s > top) {
            top = s;
            seed_track_copy(t, scratch);
        }
    }
    return SEED_OK;
}

/* take n with CVAR: n and n + 1 rendered together */
static int take(struct seed_track *t, const struct seed_track *orig, const struct seed_recipe *r,
                const struct seed_ctx *c, uint32_t n, struct seed_work *w)
{
    int rc;
    if (r->cvar == SEED_CVAR_OFF)
        return best(t, orig, r, c, n, &w->c);
    rc = best(&w->a, orig, r, c, n, &w->c);
    if (rc >= 0)
        rc = best(&w->b, orig, r, c, n + 1u, &w->c);
    if (rc >= 0)
        rc = seed_cvar_merge(t, &w->a, &w->b, r);
    return rc;
}

int seed_render(struct seed_track *out, const struct seed_track *orig, const struct seed_recipe *r,
                const struct seed_ctx *c, struct seed_work *w)
{
    int p, i, rc, len = clamp(orig->length, 1, SEED_STEPS);

    if (r->phrs == SEED_PHRS_OFF)
        return take(out, orig, r, c, r->seed, w);
    /* PHRS: page p is take n + offset(p); its steps come from that take's render */
    seed_track_copy(out, orig);
    for (p = 0; p * SEED_PAGE < len; p++) {
        int off = seed_phrase_take(r->phrs, p);
        if (off < 0)
            return off;
        rc = take(&w->page, orig, r, c, r->seed + (uint32_t)off, w);
        if (rc < 0)
            return rc;
        for (i = p * SEED_PAGE; i < (p + 1) * SEED_PAGE && i < len; i++)
            out->s[i] = w->page.s[i];
    }
    return SEED_OK;
}
