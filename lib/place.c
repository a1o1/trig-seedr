/* SPDX-License-Identifier: GPL-2.0-or-later */
/* place: PLACE: ROT, SKIP, the field rotation (FLD, FROT, FMOD) and ROTATE ALL's rotation.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * ROT moves whole steps, origin and lanes included; swing stays with the position. SKIP clears
 * the first steps (it only removes, I5). The field rotation is 0.3's ROTATE of one field and runs
 * last in the chain. Freestanding. */
#include "seed.h"

const uint8_t seed_have_place = 1;

/* k into 0..n-1 */
static int mod(int k, int n)
{
    k %= n;
    return k < 0 ? k + n : k;
}

/* one field of a step, as an int, and back */
static int get_field(const struct seed_step *s, int w)
{
    switch (w) {
    case SEED_FLD_VEL: return s->vel;
    case SEED_FLD_NOTE: return s->note;
    case SEED_FLD_LEN: return s->len;
    case SEED_FLD_UTIME: return s->utime;
    case SEED_FLD_COND: return s->cond;
    case SEED_FLD_ACC: return (s->flags & SEED_ACCENT) != 0;
    case SEED_FLD_LANE1: return s->lane[0];
    case SEED_FLD_LANE2: return s->lane[1];
    default: return 0;
    }
}

static void set_field(struct seed_step *s, int w, int v)
{
    switch (w) {
    case SEED_FLD_VEL: s->vel = (uint8_t)v; break;
    case SEED_FLD_NOTE: s->note = (uint8_t)v; break;
    case SEED_FLD_LEN: s->len = (uint8_t)v; break;
    case SEED_FLD_UTIME: s->utime = (int8_t)v; break;
    case SEED_FLD_COND: s->cond = (int8_t)v; break;
    case SEED_FLD_ACC:
        s->flags = (uint16_t)(v ? s->flags | SEED_ACCENT : s->flags & ~SEED_ACCENT);
        break;
    case SEED_FLD_LANE1: s->lane[0] = (int16_t)v; break;
    case SEED_FLD_LANE2: s->lane[1] = (int16_t)v; break;
    default: break;
    }
}

/* the retrig group travels as one value */
struct rtrg { uint8_t on, rlen, rrate; int8_t rvel; };

static struct rtrg get_rtrg(const struct seed_step *s)
{
    struct rtrg v;
    v.on = (s->flags & SEED_RETRIG) != 0;
    v.rlen = s->rlen;
    v.rrate = s->rrate;
    v.rvel = s->rvel;
    return v;
}

static void set_rtrg(struct seed_step *s, struct rtrg v)
{
    s->flags = (uint16_t)(v.on ? s->flags | SEED_RETRIG : s->flags & ~SEED_RETRIG);
    s->rlen = v.rlen;
    s->rrate = v.rrate;
    s->rvel = v.rvel;
}

static void move_field(struct seed_step *d, const struct seed_step *from, int what)
{
    if (what == SEED_FLD_RTRG)
        set_rtrg(d, get_rtrg(from));
    else
        set_field(d, what, get_field(from, what));
}

int seed_rotate(struct seed_track *t, int from, int n, int k, int what, int mode)
{
    struct seed_step old[SEED_STEPS];
    uint8_t idx[SEED_STEPS];
    int ntr = 0, i;

    if (from < 0 || n <= 0 || from + n > SEED_STEPS || what <= SEED_FLD_OFF || what >= SEED_FLD_N
        || mode < 0 || mode >= SEED_RMODE_N)
        return SEED_E_ARG;
    k = mod(k, n);
    if (!k)
        return SEED_OK;
    for (i = 0; i < n; i++)
        old[i] = t->s[from + i];

    if (what == SEED_FLD_ALL) {
        for (i = 0; i < n; i++) {
            struct seed_step *d = &t->s[from + mod(i + k, n)];
            uint16_t swing = d->flags & SEED_SWING;
            *d = old[i];
            d->flags = (uint16_t)((d->flags & ~SEED_SWING) | swing);
        }
        return SEED_OK;
    }
    if (what == SEED_FLD_ACC || mode == SEED_RMODE_GRID) {
        for (i = 0; i < n; i++) {
            struct seed_step *d = &t->s[from + mod(i + k, n)];
            if (what != SEED_FLD_ACC && !(d->flags & SEED_TRIG))
                continue;                     /* no trig here: the value is dropped */
            if (what != SEED_FLD_ACC && !(old[i].flags & SEED_TRIG))
                continue;                     /* nothing came from an empty step: keep */
            move_field(d, &old[i], what);
        }
        return SEED_OK;
    }
    /* SEED_RMODE_TRIG: move values between the trigs, in order */
    for (i = 0; i < n; i++)
        if (old[i].flags & SEED_TRIG)
            idx[ntr++] = (uint8_t)i;
    if (ntr < 2)
        return SEED_OK;
    k = mod(k, ntr);
    for (i = 0; i < ntr; i++)
        move_field(&t->s[from + idx[mod(i + k, ntr)]], &old[idx[i]], what);
    return SEED_OK;
}

int seed_place_rot(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)k; (void)c;
    return seed_rotate(t, 0, t->length, r->rot, SEED_FLD_ALL, SEED_RMODE_TRIG);
}

int seed_place_skip(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    int i;
    (void)k; (void)c;
    for (i = 0; i < r->skip && i < t->length; i++)
        seed_step_clear(&t->s[i]);
    return SEED_OK;
}

int seed_place_fields(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    (void)k; (void)c;
    if (r->fld <= SEED_FLD_OFF || r->fld > SEED_FLD_LANE2)
        return SEED_E_ARG;
    return seed_rotate(t, 0, t->length, r->frot, r->fld, r->fmod);
}
