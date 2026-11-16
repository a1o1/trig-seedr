/* SPDX-License-Identifier: GPL-2.0-or-later */
/* TOOLS YOLO (0.7): a recipe for one track rolled at random, within ranges chosen to sound like a
 * drum part rather than noise. A setting picked at random over its whole range is mostly unusable
 * (DENS 0, a condition on every trig, a retrig on every step), so each layer has its own musical
 * ranges, shaped by what the track plays (its role) and by WILD:
 *   WILD 0..39    one or two layers, mostly on the track's own trigs (it keeps them 70 % of the time to 30)
 *   WILD 40..79   two or three layers, other sources more often; notes from 50, phrases from 40
 *   WILD 80..100  three or four layers, wider ranges, sample locks (from 70)
 * Everything is drawn from the roll, so the same roll, WILD and track give the same recipe. */
#include "seed.h"

const uint8_t seed_have_yolo = 1;

struct dice {
    uint32_t s;
};

static int die(struct dice *d, int lo, int hi)
{
    d->s = seed_hash(d->s + 0x9e3779b9u);
    return hi <= lo ? lo : lo + (int)(d->s % (uint32_t)(hi - lo + 1));
}

static int chance(struct dice *d, int pct)
{
    return die(d, 0, 99) < pct;
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void range(struct seed_values *v, int lo, int hi)
{
    v->lo = (int16_t)lo;
    v->hi = (int16_t)hi;
    v->src = SEED_VSRC_RANGE;
}

/* a range whose ends are drawn low end first. Every draw is a statement of its own: C leaves the
 * order of two calls in one expression open, and the Rytm's compiler and the host's differ */
static void range_of(struct seed_values *v, struct dice *d, int lo0, int lo1, int hi0, int hi1)
{
    int lo = die(d, lo0, lo1), hi = die(d, hi0, hi1);
    range(v, lo, hi);
}

/* the rhythm: the track's own trigs, or a source with settings that suit the role */
static void rhythm(struct seed_recipe *r, struct dice *d, int w, const struct seed_yolo_in *in)
{
    int keep = !in->any ? 0 : w <= 30 ? 70 : w <= 70 ? 35 : 15, pick, steps;
    if (chance(d, keep))
        return;                                           /* SRC = TRACK */
    pick = die(d, 0, 99);
    if (pick < 30 && seed_have_map) {
        r->src = SEED_SRC_MAP;
        r->map_part = (int16_t)(in->role == SEED_ROLE_KICK ? 0 : in->role == SEED_ROLE_BACK ? 1
                                : in->role == SEED_ROLE_HAT ? 2 : die(d, 0, 2));
        r->map_dens = (int16_t)die(d, 110, 170 + w / 2);
    } else if (pick < 55 && seed_have_euclid) {
        r->src = SEED_SRC_EUCLID;
        steps = in->length <= 16 || chance(d, 60) ? clampi(in->length, 1, 16) : in->length;
        r->eu_steps = (int16_t)steps;
        switch (in->role) {
        case SEED_ROLE_KICK: r->eu_puls = (int16_t)die(d, 3, 5); break;
        case SEED_ROLE_BACK: r->eu_puls = (int16_t)die(d, 2, 4); break;
        case SEED_ROLE_HAT:  r->eu_puls = (int16_t)die(d, 5, 11); break;
        default:             r->eu_puls = (int16_t)die(d, 3, 7); break;
        }
        r->eu_puls = (int16_t)clampi(r->eu_puls * steps / 16, 1, steps);
        if (chance(d, 40))
            r->rot = (int16_t)die(d, 1, steps - 1);       /* a turn of the necklace */
    } else if (seed_have_random) {
        int spread = w / 5;
        r->src = SEED_SRC_RANDOM;
        r->role = (int16_t)(in->role >= 0 && in->role < SEED_ROLE_N ? in->role : SEED_ROLE_METER);
        switch (in->role) {
        case SEED_ROLE_KICK: r->dens = (int16_t)die(d, 20, 35 + spread); r->bias = (int16_t)die(d, 50, 90); break;
        case SEED_ROLE_BACK: r->dens = (int16_t)die(d, 12, 25 + spread); r->bias = (int16_t)die(d, 50, 90); break;
        case SEED_ROLE_HAT:  r->dens = (int16_t)die(d, 40, 70 + spread); r->bias = (int16_t)die(d, 0, 60); break;
        default:             r->dens = (int16_t)die(d, 20, 45 + spread); r->bias = (int16_t)die(d, -30, 50); break;
        }
        r->grid = (int16_t)(w > 60 && in->role != SEED_ROLE_HAT && chance(d, 25) ? 2 : 1);
        if (seed_have_score && chance(d, 50))
            r->qual = (int16_t)die(d, 3, 6);              /* the best spread of a few candidates */
    }
}

enum { L_DYN, L_TRIG, L_TIME, L_PLACE, L_NOTE, L_LOCK, L_PHRS, L_N };

/* 1 when the layer could be rolled on this track at this WILD */
static int layer_ok(int l, const struct seed_recipe *r, int w, const struct seed_yolo_in *in)
{
    switch (l) {
    case L_DYN:   return seed_have_dyn && (in->fields & SEED_F_VEL);
    case L_TRIG:  return seed_have_trig && (in->fields & (SEED_F_COND | SEED_F_RETRIG));
    case L_TIME:  return seed_have_timing && (in->fields & SEED_F_UTIME);
    case L_PLACE: return seed_have_sync && r->src != SEED_SRC_EUCLID;
    case L_NOTE:  return w >= 50 && seed_have_values && (in->fields & SEED_F_NOTE);
    case L_LOCK:  return w >= 70 && seed_have_values && (in->fields & SEED_F_LOCKS) && in->caps && in->caps->lanes > 0;
    case L_PHRS:  return w >= 40 && seed_have_vary && in->length > SEED_PAGE && r->src != SEED_SRC_TRACK;
    default:      return 0;
    }
}

static void layer(int l, struct seed_recipe *r, struct dice *d, int w, const struct seed_yolo_in *in)
{
    switch (l) {
    case L_DYN:                                           /* accents where they belong, a little life */
        if (r->src == SEED_SRC_MAP && seed_have_map && chance(d, 50))
            r->shape = SEED_SHAPE_MAP;
        else {
            static const int16_t shapes[] = { SEED_SHAPE_STRONG, SEED_SHAPE_NOT1, SEED_SHAPE_RANK, SEED_SHAPE_NOISE,
                                              SEED_SHAPE_EVRY };
            r->shape = shapes[die(d, 0, 4)];
            if ((r->shape == SEED_SHAPE_RANK && !seed_have_rank) || (r->shape == SEED_SHAPE_NOISE && !seed_have_noise))
                r->shape = SEED_SHAPE_STRONG;
            r->n = (int16_t)(r->shape == SEED_SHAPE_NOISE ? die(d, 4, 16) : die(d, 2, 4));
        }
        r->amt = (int16_t)die(d, 40, 90);
        range_of(&r->vel, d, 50, 90, 110, 127);
        if ((in->fields & SEED_F_ACCENT) && chance(d, 50)) {
            r->acc = (int16_t)die(d, 10, 30);
            r->amod = (int16_t)(seed_have_rank || r->src == SEED_SRC_MAP ? SEED_AMOD_TOP : SEED_AMOD_RAND);
        }
        r->hum = (int16_t)die(d, 0, 15);
        break;
    case L_TRIG:                                          /* a few trigs that come and go, or roll */
        if ((in->fields & SEED_F_COND) && (!(in->fields & SEED_F_RETRIG) || chance(d, 65))) {
            r->cond = (int16_t)die(d, 10, 25 + w / 5);
            switch (die(d, 0, w > 50 ? 2 : 1)) {
            case 0:  r->ctyp = SEED_CTYP_PROB; range_of(&r->cval, d, 10, 14, 15, 18); break;
            case 1:  r->ctyp = SEED_CTYP_AB; range(&r->cval, die(d, 2, 4), 8); break;
            default: r->ctyp = SEED_CTYP_FILL; break;
            }
        } else {
            r->rtrg = (int16_t)die(d, 5, in->role == SEED_ROLE_HAT || in->role == SEED_ROLE_PERC ? 20 : 12);
            range(&r->rate, SEED_NEW_RRATE, die(d, SEED_NEW_RRATE, SEED_NEW_RRATE + 3));
            r->rcur = (int16_t)die(d, -30, 30);
        }
        break;
    case L_TIME:                                          /* a groove, or a human feel */
        if (in->ngrooves > 0 && chance(d, 50)) {
            r->grv = (int16_t)die(d, 1, in->ngrooves);
            r->timing = (int16_t)die(d, 50, 100);
        } else
            r->rand = (int16_t)die(d, 5, 15 + w / 10);
        if (seed_have_drift && chance(d, 30))
            r->drft = (int16_t)die(d, 10, 30);
        break;
    case L_PLACE:                                         /* trigs pulled onto the grid's stronger steps */
        r->sync = (int16_t)die(d, 10, 30 + w / 5);
        if (w > 60 && r->rot == 0 && chance(d, 30)) {
            int sign = chance(d, 50) ? -1 : 1;
            r->rot = (int16_t)(sign * die(d, 1, 2));
        }
        break;
    case L_NOTE: {                                        /* a small tune: a few pitches near the sound's own */
        int span = in->role == SEED_ROLE_KICK || in->role == SEED_ROLE_BACK ? 2 : 3 + w / 20;
        range_of(&r->note, d, 0, span, 1, span);
        r->note.lo = (int16_t)-r->note.lo;
        if (seed_have_list && chance(d, 60)) {
            static const int16_t orders[] = { SEED_ORDR_UP, SEED_ORDR_PING, SEED_ORDR_RAND };
            r->note.src = SEED_VSRC_LIST;
            r->note.len = (int16_t)die(d, 3, 6);
            r->note.order = orders[die(d, 0, 2)];
        }
        break;
    }
    case L_LOCK:                                          /* lane 1 on its parameter (the Rytm: the sample) */
        r->lane[0].pct = (int16_t)die(d, 15, 35);
        {
            int lo = 0, hi = 127, a;
            if (in->caps && in->caps->nparams > 0 && r->lane[0].parm >= 0 && r->lane[0].parm < in->caps->nparams) {
                lo = in->caps->params[r->lane[0].parm].lo;
                hi = in->caps->params[r->lane[0].parm].hi;
            }
            a = die(d, lo, clampi(lo + 8, lo, hi));
            range(&r->lane[0].val, clampi(a, lo, hi), clampi(a + die(d, 1, 4), lo, hi));
        }
        break;
    case L_PHRS: {                                        /* the take as a phrase over the pages */
        static const int16_t phrs[] = { SEED_PHRS_AAAB, SEED_PHRS_ABAB, SEED_PHRS_ABAC };
        r->phrs = phrs[die(d, 0, 2)];
        r->vary = (int16_t)die(d, 40, 80);                /* the pages related, not unrelated */
        break;
    }
    }
}

void seed_yolo(struct seed_recipe *r, const struct seed_recipe *from, int roll, int wild, const struct seed_yolo_in *in)
{
    struct dice d;
    int w = clampi(wild, 0, 100), n, l, i, tries, used = 0;
    seed_recipe_defaults(r, in->caps);
    if (from) {                                           /* the meter and the lanes' parameters are the user's */
        r->metr = from->metr;
        r->beat = from->beat;
        for (i = 0; i < SEED_LANES; i++)
            r->lane[i].parm = from->lane[i].parm;
    }
    d.s = seed_hash((uint32_t)roll * 0x10001u + (uint32_t)w * 0x3b9u + 0x5eedu);
    r->seed = (uint32_t)die(&d, 0, 9999);
    rhythm(r, &d, w, in);
    n = 1 + w / 40 + die(&d, 0, 1);                      /* 1..4 layers */
    for (i = 0; i < n; i++)
        for (tries = 0; tries < 12; tries++) {
            l = die(&d, 0, L_N - 1);
            if ((used & (1 << l)) || !layer_ok(l, r, w, in))
                continue;
            used |= 1 << l;
            layer(l, r, &d, w, in);
            break;
        }
}
