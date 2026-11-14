/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "groove.h"

static int clampg(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* a / b rounded to nearest, halves away from zero; b > 0 */
static int rdiv(int a, int b)
{
    return a >= 0 ? (a + b / 2) / b : -((-a + b / 2) / b);
}

static int modg(int k, int n)
{
    k %= n;
    return k < 0 ? k + n : k;
}

int gen_groove_apply(struct gen_track *t, int from, int n, const struct gen_groove *g,
                     const struct gen_groove_params *p, uint8_t def_vel)
{
    struct gen_rng r;
    int i, changed = 0;
    int timing = clampg(p->timing, -100, 100), velocity = clampg(p->velocity, -100, 100);
    int quant = clampg(p->quant, 0, 100), rnd = rdiv(clampg(p->random, 0, 100) * GEN_GROOVE_RAND, 100);
    if (!g || g->len < 1 || g->len > GEN_STEPS)
        return 0;
    gen_seed(&r, p->seed);
    from = clampg(from, 0, GEN_STEPS);
    n = clampg(n, 0, GEN_STEPS - from);
    for (i = from; i < from + n; i++) {
        struct gen_step *s = &t->s[i];
        int gi = modg(i - p->shift, g->len), u, v, before_u = s->utime, before_v = s->vel;
        if (!(s->flags & GEN_TRIG))
            continue;
        u = rdiv(s->utime * (100 - quant), 100) + rdiv(g->off[gi] * timing, 100);
        if (rnd)
            u += gen_range(&r, -rnd, rnd);
        s->utime = (int8_t)clampg(u, -23, 23);
        if (velocity && g->vmax && g->vel[gi]) {
            v = s->vel == GEN_DEFAULT ? def_vel : s->vel;
            /* at +100 % the step's velocity is scaled by vel / vmax; at -100 % the other way */
            v += rdiv(v * velocity * (g->vel[gi] - g->vmax), 100 * g->vmax);
            s->vel = (uint8_t)clampg(v, 1, 127);
        }
        changed += s->utime != before_u || s->vel != before_v;
    }
    return changed;
}
