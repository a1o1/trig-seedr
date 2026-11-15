/* SPDX-License-Identifier: GPL-2.0-or-later */
/* gen: GENERATOR and ROTATE. Freestanding: no C library, no division by variables in hot paths
 * that libgcc would have to provide (ColdFire has divu.l / remu.l, so % and / are fine). */
#include "gen.h"

#define KNOWN (GEN_TRIG | GEN_RETRIG | GEN_MUTE | GEN_ACCENT | GEN_SLIDE)

/* ---- random numbers ---- */

void gen_seed(struct gen_rng *r, uint32_t seed)
{
    /* spread small seeds (0..9999) over the state; 0 is not a valid xorshift state */
    uint32_t x = seed * 2654435761u + 0x9e3779b9u;
    r->x = x ? x : 0x6d2b79f5u;
    gen_next(r);
    gen_next(r);
}

uint32_t gen_next(struct gen_rng *r)
{
    uint32_t x = r->x;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return r->x = x;
}

int gen_range(struct gen_rng *r, int lo, int hi)
{
    uint32_t span;
    if (hi < lo) {
        int t = lo;
        lo = hi;
        hi = t;
    }
    span = (uint32_t)(hi - lo) + 1u;
    return lo + (int)(gen_next(r) % span);
}

static int pct(struct gen_rng *r, int percent)
{
    if (percent <= 0)
        return 0;
    if (percent >= 100)
        return 1;
    return (int)(gen_next(r) % 100u) < percent;
}

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

int gen_cond_ab(int a, int b)
{
    b = clamp(b, 2, 8);
    a = clamp(a, 1, b);
    return 29 + b * (b - 1) / 2 + a - 1;      /* 1:2 = 30, 1:4 = 35, 1:8 = 57, 8:8 = 64 */
}

/* ---- GENERATOR ---- */

void gen_defaults(struct gen_params *p)
{
    p->grid = 1;
    p->dens = 50;
    p->vel_lo = 80;
    p->vel_hi = 120;
    p->len_lo = p->len_hi = GEN_DEFAULT;
    p->note_lo = p->note_hi = 0;
    p->utim = 0;
    p->seed = 1;
    p->rot = 0;
    p->acc = 0;
    p->cond = 0;
    p->ctyp = GEN_CT_PROB;
    p->cval_lo = 10;                           /* 50% */
    p->cval_hi = 21;                           /* 100% */
    p->rtrg = 0;
    p->rate_lo = p->rate_hi = GEN_NEW_RRATE;
    p->rcur = 0;
    p->smp = 0;
    p->smp_lo = 1;
    p->smp_hi = 8;
    p->beat = 4;
    p->skip = 0;
    p->bias = 0;
    p->vshp = GEN_VS_FLAT;
    p->vamt = 0;
}

int gen_weight(int i, int beat)
{
    int bar;
    beat = clamp(beat, 1, 16);
    bar = 4 * beat;
    if (i < 0)
        return 0;
    if (i % bar == 0)
        return 4;
    if (i % (bar / 2) == 0)
        return 3;
    if (i % beat == 0)
        return 2;
    if (beat % 2 == 0 && i % (beat / 2) == 0)
        return 1;
    return 0;
}

/* how loud a step of weight w is in a shape, 0 (VEL's low end) .. 4 (its high end) */
static int shape_level(enum gen_vshp s, int w)
{
    switch (s) {
    case GEN_VS_STRONG: return w;
    case GEN_VS_WEAK:   return 4 - w;
    case GEN_VS_OFF:    return w == 1 ? 4 : w == 0 ? 2 : 0;
    case GEN_VS_NOT1:   return w >= 2 ? 0 : 4;
    case GEN_VS_NOTBAR: return w == 4 ? 0 : 4;
    default:            return -1;
    }
}

static int imod(int k, int n)
{
    k %= n;
    return k < 0 ? k + n : k;
}

static void clear_step(struct gen_step *s)
{
    s->flags &= (uint16_t)~KNOWN;
    s->note = s->vel = s->len = GEN_DEFAULT;
    s->utime = 0;
    s->cond = GEN_NO_COND;
    s->rlen = GEN_NEW_RLEN;
    s->rrate = GEN_NEW_RRATE;
    s->rvel = 0;
}

static int make_cond(struct gen_rng *r, const struct gen_params *p)
{
    enum gen_ctyp c = p->ctyp;
    if (c == GEN_CT_MIX)
        c = (enum gen_ctyp)gen_range(r, GEN_CT_PROB, GEN_CT_FILL);
    switch (c) {
    case GEN_CT_AB: {
        int b = clamp(p->cval_lo, 2, 8);
        return gen_cond_ab(gen_range(r, 1, b), b);
    }
    case GEN_CT_1ST:
        return 28;
    case GEN_CT_FILL:
        return 22;
    default:
        return gen_range(r, clamp(p->cval_lo, 0, 21), clamp(p->cval_hi, 0, 21));
    }
}

int gen_generate(struct gen_track *t, const struct gen_params *p)
{
    struct gen_rng r;
    uint8_t pos[GEN_STEPS];
    int n = clamp(t->length, 1, GEN_STEPS), grid, npos = 0, want, i, written = 0, beat, skip, bias;

    gen_seed(&r, p->seed);
    grid = p->grid == 2 || p->grid == 4 || p->grid == 8 ? p->grid : 1;
    for (i = 0; i < n; i++) {
        clear_step(&t->s[i]);
        if (p->smp > 0)
            t->s[i].slock = GEN_NO_LOCK;
    }

    beat = clamp(p->beat, 1, 16);
    skip = clamp(p->skip, 0, GEN_STEPS);
    bias = clamp(p->bias, -100, 100);
    /* a grid position q ends up on step (q + rot) mod n */
    for (i = 0; i < n; i += grid)
        if (imod(i + p->rot, n) >= skip)
            pos[npos++] = (uint8_t)i;
    /* exactly round(dens * npos / 100) positions */
    want = (clamp(p->dens, 0, 100) * npos + 50) / 100;
    for (i = 0; i < want; i++) {
        int j;
        uint8_t tmp;
        if (!bias) {
            j = gen_range(&r, i, npos - 1);    /* a partial Fisher-Yates shuffle */
        } else {
            /* a draw weighted by the final step's metric weight: each weight level doubles the
             * chance (1, 2, 4, 8, 16; reversed for a negative BIAS), mixed with uniform (4) */
            static const uint8_t dbl[5] = { 1, 2, 4, 8, 16 };
            int dw[GEN_STEPS], sum = 0, k, a = bias < 0 ? -bias : bias;
            for (k = i; k < npos; k++) {
                int w = gen_weight(imod(pos[k] + p->rot, n), beat);
                dw[k] = (100 - a) * 4 + a * dbl[bias > 0 ? w : 4 - w];
                sum += dw[k];
            }
            k = gen_range(&r, 0, sum - 1);
            for (j = i; j < npos - 1 && k >= dw[j]; j++)
                k -= dw[j];
        }
        tmp = pos[i];
        pos[i] = pos[j];
        pos[j] = tmp;
    }

    for (i = 0; i < want; i++) {
        struct gen_step *s = &t->s[pos[i]];
        s->flags |= GEN_TRIG;
        s->vel = (uint8_t)gen_range(&r, clamp(p->vel_lo, 1, 127), clamp(p->vel_hi, 1, 127));
        if (p->vamt > 0 && shape_level(p->vshp, 0) >= 0) {
            int lo = clamp(p->vel_lo, 1, 127), hi = clamp(p->vel_hi, 1, 127), v = s->vel, to;
            int lev = shape_level(p->vshp, gen_weight(imod(pos[i] + p->rot, n), beat));
            if (hi < lo) { int x = lo; lo = hi; hi = x; }
            to = lo + (hi - lo) * lev / 4;
            v += (to - v) * clamp(p->vamt, 0, 100) / 100;
            s->vel = (uint8_t)clamp(v, 1, 127);
        }
        if (p->len_lo != GEN_DEFAULT || p->len_hi != GEN_DEFAULT)
            s->len = (uint8_t)gen_range(&r, clamp(p->len_lo, 0, 127), clamp(p->len_hi, 0, 127));
        if (p->note_lo || p->note_hi) {
            int off = gen_range(&r, clamp(p->note_lo, -24, 24), clamp(p->note_hi, -24, 24));
            s->note = (uint8_t)clamp(t->default_note + off, 0, 126);
        }
        if (p->utim > 0) {
            int m = 23 * clamp(p->utim, 0, 100) / 100;
            s->utime = (int8_t)gen_range(&r, -m, m);
        }
        if (pct(&r, p->acc))
            s->flags |= GEN_ACCENT;
        if (pct(&r, p->cond))
            s->cond = (int8_t)make_cond(&r, p);
        if (pct(&r, p->rtrg)) {
            s->flags |= GEN_RETRIG;
            s->rrate = (uint8_t)gen_range(&r, clamp(p->rate_lo, 0, 16), clamp(p->rate_hi, 0, 16));
            s->rvel = (int8_t)clamp(p->rcur, -128, 127);
        }
        if (pct(&r, p->smp))
            s->slock = (int16_t)gen_range(&r, clamp(p->smp_lo, 0, 127), clamp(p->smp_hi, 0, 127));
        written++;
    }
    if (p->rot)
        gen_rotate(t, 0, n, p->rot, GEN_W_ALL, GEN_R_TRIG);
    return written;
}

/* ---- ROTATE ---- */

/* k into 0..n-1 */
static int mod(int k, int n)
{
    k %= n;
    return k < 0 ? k + n : k;
}

/* one field of a step, as an int, and back */
static int get_field(const struct gen_step *s, enum gen_what w)
{
    switch (w) {
    case GEN_W_VEL: return s->vel;
    case GEN_W_NOTE: return s->note;
    case GEN_W_LEN: return s->len;
    case GEN_W_UTIM: return s->utime;
    case GEN_W_COND: return s->cond;
    case GEN_W_ACC: return (s->flags & GEN_ACCENT) != 0;
    default: return 0;
    }
}

static void set_field(struct gen_step *s, enum gen_what w, int v)
{
    switch (w) {
    case GEN_W_VEL: s->vel = (uint8_t)v; break;
    case GEN_W_NOTE: s->note = (uint8_t)v; break;
    case GEN_W_LEN: s->len = (uint8_t)v; break;
    case GEN_W_UTIM: s->utime = (int8_t)v; break;
    case GEN_W_COND: s->cond = (int8_t)v; break;
    case GEN_W_ACC:
        s->flags = (uint16_t)(v ? s->flags | GEN_ACCENT : s->flags & ~GEN_ACCENT);
        break;
    default: break;
    }
}

/* the retrig group travels as one value */
struct rtrg { uint8_t on, rlen, rrate; int8_t rvel; };

static struct rtrg get_rtrg(const struct gen_step *s)
{
    struct rtrg v;
    v.on = (s->flags & GEN_RETRIG) != 0;
    v.rlen = s->rlen;
    v.rrate = s->rrate;
    v.rvel = s->rvel;
    return v;
}

static void set_rtrg(struct gen_step *s, struct rtrg v)
{
    s->flags = (uint16_t)(v.on ? s->flags | GEN_RETRIG : s->flags & ~GEN_RETRIG);
    s->rlen = v.rlen;
    s->rrate = v.rrate;
    s->rvel = v.rvel;
}

void gen_rotate(struct gen_track *t, int from, int n, int k, enum gen_what what, enum gen_rmode mode)
{
    struct gen_step old[GEN_STEPS];
    int idx[GEN_STEPS], ntr = 0, i;

    if (from < 0 || n <= 0 || from + n > GEN_STEPS)
        return;
    for (i = 0; i < n; i++)
        old[i] = t->s[from + i];
    k = mod(k, n);
    if (!k)
        return;

    if (what == GEN_W_ALL) {
        for (i = 0; i < n; i++) {
            struct gen_step *d = &t->s[from + mod(i + k, n)];
            uint16_t swing = d->flags & GEN_SWING;
            *d = old[i];
            d->flags = (uint16_t)((d->flags & ~GEN_SWING) | swing);
        }
        return;
    }
    if (what == GEN_W_ACC || mode == GEN_R_GRID) {
        for (i = 0; i < n; i++) {
            struct gen_step *d = &t->s[from + mod(i + k, n)];
            if (what != GEN_W_ACC && !(d->flags & GEN_TRIG))
                continue;                     /* no trig here: the value is dropped */
            if (what != GEN_W_ACC && !(old[i].flags & GEN_TRIG))
                continue;                     /* nothing came from an empty step: keep */
            if (what == GEN_W_RTRG)
                set_rtrg(d, get_rtrg(&old[i]));
            else
                set_field(d, what, get_field(&old[i], what));
        }
        return;
    }
    /* GEN_R_TRIG: move values between the trigs, in order */
    for (i = 0; i < n; i++)
        if (old[i].flags & GEN_TRIG)
            idx[ntr++] = i;
    if (ntr < 2)
        return;
    k = mod(k, ntr);
    for (i = 0; i < ntr; i++) {
        struct gen_step *d = &t->s[from + idx[mod(i + k, ntr)]];
        if (what == GEN_W_RTRG)
            set_rtrg(d, get_rtrg(&old[idx[i]]));
        else
            set_field(d, what, get_field(&old[idx[i]], what));
    }
}
