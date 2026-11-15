/* SPDX-License-Identifier: GPL-2.0-or-later */
/* timing: TIME: random and groove micro timing, GVEL.
 * Owner: task 0.2 (docs/plan-v1.md section 4).
 * 0.3's GROOVE (lib/groove.c, after Live's Groove Pool) and GEN's UTIM in one stage, on the trigs:
 *   utime = utime * (100 - QUANT) / 100 + the groove's offset * TIME / 100 + random up to
 *           +-RAND % of the device's range, clamped to that range
 *   vel   at GVEL +100 % scaled by the groove's velocity / its loudest; negative the other way
 * The groove's step is (step - SHFT) mod its length. DRFT (F8) runs after. Freestanding. */
#include "seed.h"

const uint8_t seed_have_timing = 1;

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* a / b rounded to nearest, halves away from zero; b > 0 */
static int rdiv(int a, int b)
{
    return a >= 0 ? (a + b / 2) / b : -((-a + b / 2) / b);
}

static int mod(int k, int n)
{
    k %= n;
    return k < 0 ? k + n : k;
}

int seed_time(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k, const struct seed_ctx *c)
{
    const struct seed_groove *g = 0;
    int n = t->length, i, lo = -23, hi = 23, quant, timing, gvel, m;

    if (c && c->caps) {
        lo = c->caps->utime_lo;
        hi = c->caps->utime_hi;
    }
    if (r->grv > 0) {
        if (!c || !c->grooves || r->grv > c->ngrooves)
            return SEED_E_ARG;
        g = &c->grooves[r->grv - 1];
        if (g->len < 1 || g->len > SEED_STEPS)
            return SEED_E_ARG;
    }
    if (r->drft > 0 && !seed_have_drift)
        return SEED_E_NA;
    quant = clamp(r->quant, 0, 100);
    timing = g ? clamp(r->timing, -100, 100) : 0;
    gvel = g && g->vmax && (t->fields & SEED_F_VEL) ? clamp(r->gvel, -100, 100) : 0;
    m = hi * clamp(r->rand, 0, 100) / 100;

    for (i = 0; i < n; i++) {
        struct seed_step *s = &t->s[i];
        int gi = g ? mod(i - r->shft, g->len) : 0;
        if (!(s->flags & SEED_TRIG))
            continue;
        if (t->fields & SEED_F_UTIME) {
            int u = rdiv(s->utime * (100 - quant), 100);
            if (g)
                u += rdiv(g->off[gi] * timing, 100);
            if (m)
                u += seed_rand_range(k, SEED_SLOT(SEED_L_TIME, SEED_P_UTIME, i), -m, m);
            s->utime = (int8_t)clamp(u, lo, hi);
        }
        if (gvel && g->vel[gi]) {
            int v = s->vel == SEED_DEFAULT ? t->default_vel : s->vel;
            v += rdiv(v * gvel * (g->vel[gi] - g->vmax), 100 * g->vmax);
            s->vel = (uint8_t)clamp(v, 1, 127);
        }
    }
    if (r->drft > 0)
        return seed_time_drift(t, r, k, c);
    return SEED_OK;
}
