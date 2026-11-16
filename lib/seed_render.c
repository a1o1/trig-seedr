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

/* one take, one candidate, from the original into t up to the trigs' final positions: RHYTHM, PLACE */
static int place(struct seed_track *t, const struct seed_track *orig, const struct seed_recipe *r,
                 const struct seed_ctx *c, const struct seed_take *k)
{
    int i, len;

    seed_track_copy(t, orig);
    t->length = (int16_t)(len = clamp(orig->length, 1, SEED_STEPS));

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
    return SEED_OK;
}

/* the layers that dress the trigs where PLACE left them: DYNAMICS to TIME, the field rotation */
static int dress(struct seed_track *t, const struct seed_recipe *r, const struct seed_ctx *c,
                 const struct seed_take *k)
{
    STAGE(seed_layer_active(r, SEED_L_DYN), seed_dyn);
    STAGE(seed_layer_active(r, SEED_L_NOTE), seed_note);
    STAGE(seed_layer_active(r, SEED_L_LOCK), seed_lock);
    STAGE(seed_layer_active(r, SEED_L_TRIG), seed_trig);
    STAGE(seed_layer_active(r, SEED_L_TIME), seed_time);
    /* the field rotation moves values the layers above have written */
    STAGE(seed_layer_active(r, SEED_L_FIELDS), seed_place_fields);
    return SEED_OK;
}

/* QUAL: take n from the best of r->qual candidates (ties: the lower candidate), into t. A
 * candidate's numbers differ from candidate 0's only in RHYTHM and PLACE (seed_rng.c) and no
 * later layer moves a trig, so each candidate runs to the end of PLACE and is scored there; the
 * later layers then run once, on the winner. */
static int best(struct seed_track *t, const struct seed_track *orig, const struct seed_recipe *r,
                const struct seed_ctx *c, uint32_t n, struct seed_track *scratch)
{
    struct seed_take k;
    int rc, q = clamp(r->qual, 1, 8), top = 0, win = 0, i;
    for (i = 0; i < q; i++) {
        struct seed_track *u = i ? scratch : t;
        int s;
        seed_take_init(&k, n, (uint32_t)c->track, r->vary, i);
        k.length = clamp(orig->length, 1, SEED_STEPS);      /* NOISE wraps there */
        rc = place(u, orig, r, c, &k);
        if (rc < 0)
            return rc;
        if (q == 1)
            break;
        s = seed_score(u, r, c);
        if (s < 0)
            return s;
        if (i == 0 || s > top) {
            top = s;
            win = i;
            if (i)
                seed_track_copy(t, u);
        }
    }
    seed_take_init(&k, n, (uint32_t)c->track, r->vary, win);
    k.length = clamp(orig->length, 1, SEED_STEPS);
    return dress(t, r, c, &k);
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

/* a struct copy without memcpy, which a mod does not link (both structs are 2-byte aligned) */
static void copy16(void *to, const void *from, unsigned n)
{
    uint16_t *d = to;
    const uint16_t *s = from;
    for (n /= 2; n > 0; n--)
        *d++ = *s++;
}

int seed_render(struct seed_track *out, const struct seed_track *orig, const struct seed_recipe *r,
                const struct seed_ctx *c, struct seed_work *w)
{
    struct seed_recipe pr;
    struct seed_ctx pc;
    struct seed_take k;
    uint8_t eu[SEED_STEPS / 8];
    int p, i, j, rc, len = clamp(orig->length, 1, SEED_STEPS), euclid = r->src == SEED_SRC_EUCLID;

    /* a track of one page or less is one phrase */
    if (r->phrs == SEED_PHRS_OFF || len <= SEED_PAGE)
        return r->phrs < 0 || r->phrs >= SEED_PHRS_N ? SEED_E_ARG : take(out, orig, r, c, r->seed, w);
    /* PHRS (F9): page p is take n + offset(p) rendered as a 16-step phrase from the original's
     * page p, so pages with the same letter repeat; a last, shorter page plays the phrase's start.
     * What belongs to the track's positions stays there: SKIP, the groove, the other tracks, and
     * EUCLID's cycle (M2.1): it draws no random numbers, so a letter could never change it, and a
     * cycle restarted on each page broke every STEPS that does not divide 16. EUCLID's pulses are
     * placed over the whole length first; each page then starts from them as generated steps. */
    if (euclid) {
        seed_track_copy(&w->page, orig);
        w->page.length = (int16_t)len;
        for (i = 0; i < len; i++)
            seed_step_clear(&w->page.s[i]);
        seed_take_init(&k, r->seed, (uint32_t)c->track, r->vary, 0);
        rc = seed_src_euclid(&w->page, r, &k, c);
        if (rc < 0)
            return rc;
        for (i = 0; i < SEED_STEPS / 8; i++)
            eu[i] = 0;
        for (i = 0; i < len; i++)
            if (w->page.s[i].flags & SEED_TRIG)
                eu[i >> 3] |= (uint8_t)(1u << (i & 7));
    }
    seed_track_copy(out, orig);
    for (p = 0; p * SEED_PAGE < len; p++) {
        int base = p * SEED_PAGE, off = seed_phrase_take(r->phrs, p);
        if (off < 0)
            return off;
        seed_track_copy(&w->page, orig);
        /* A last, shorter page of trigs that are already placed (TRACK, COPY, EUCLID) is a phrase
         * of its own length, so ROT and SYNC turn its trigs round inside the track and none is
         * lost past the end (M2.1). A source that makes the phrase (RAND, MAP) makes all 16 steps
         * and the track plays their start (F9). */
        w->page.length = (int16_t)(base + SEED_PAGE > len && (r->src == SEED_SRC_TRACK || r->src == SEED_SRC_COPY || euclid)
                                   ? len - base : SEED_PAGE);
        for (i = 0; i < SEED_PAGE; i++) {
            w->page.s[i] = orig->s[base + i];
            w->page.s[i].origin = (int8_t)i;
            if (base + i >= len || euclid)
                seed_step_clear(&w->page.s[i]);     /* past the track's end: no trig */
            if (euclid && base + i < len && (eu[(base + i) >> 3] >> ((base + i) & 7) & 1))
                w->page.s[i].flags |= SEED_TRIG;
        }
        copy16(&pr, r, sizeof pr);
        if (euclid)
            pr.src = SEED_SRC_TRACK;                /* the page's pulses, placed above */
        pr.skip = (int16_t)clamp(r->skip - base, 0, w->page.length);
        pr.shft = (int16_t)(r->shft - base);
        copy16(&pc, c, sizeof pc);
        for (j = 0; j < c->tracks && j < SEED_TRACKS; j++) {
            int n = c->length[j];
            if (n < 1)
                continue;
            pc.length[j] = SEED_PAGE;
            pc.trig[j][0] = pc.trig[j][1] = 0;
            for (i = 0; i < SEED_PAGE; i++)
                if (seed_ctx_trig(c, j, (base + i) % n))
                    pc.trig[j][i >> 3] |= (uint8_t)(1u << (i & 7));
        }
        rc = take(&w->a, &w->page, &pr, &pc, r->seed + (uint32_t)off, w);
        if (rc < 0)
            return rc;
        for (i = 0; i < SEED_PAGE && base + i < len; i++) {
            int o = w->a.s[i].origin;
            out->s[base + i] = w->a.s[i];
            /* an origin past the track's end (a rotated empty step) is none; EUCLID's are
             * generated steps */
            out->s[base + i].origin = (int8_t)(o != SEED_NO_ORIGIN && base + o < len && !euclid ? base + o : SEED_NO_ORIGIN);
        }
    }
    return SEED_OK;
}
