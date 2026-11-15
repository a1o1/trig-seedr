/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests: task 0.2's port against 0.3's engine (gen.c, groove.c), on the parts whose results do
 * not depend on random numbers: ROTATE (seed_rotate, ROT), the metric weight, A:B condition codes,
 * and the groove's timing and velocity (seed_time with RAND 0). Remove this file with gen.c and
 * groove.c at gate G0 (docs/plan-v1.md section 4). */
#include "tests.h"
#include "gen.h"
#include "groove.h"

static void to_gen(struct gen_track *g, const struct seed_track *t)
{
    int i;
    g->length = t->length;
    g->default_note = t->default_note;
    for (i = 0; i < SEED_STEPS; i++) {
        const struct seed_step *s = &t->s[i];
        struct gen_step *d = &g->s[i];
        d->flags = s->flags;
        d->note = s->note;
        d->vel = s->vel;
        d->len = s->len;
        d->utime = s->utime;
        d->cond = s->cond;
        d->rlen = s->rlen;
        d->rrate = s->rrate;
        d->rvel = s->rvel;
        d->slock = s->lane[0];
    }
}

/* 1 when t and g hold the same steps (gen has no origin and one lane) */
static int same(const struct seed_track *t, const struct gen_track *g)
{
    int i;
    for (i = 0; i < SEED_STEPS; i++) {
        const struct seed_step *s = &t->s[i];
        const struct gen_step *d = &g->s[i];
        if (d->flags != s->flags || d->note != s->note || d->vel != s->vel || d->len != s->len
            || d->utime != s->utime || d->cond != s->cond || d->rlen != s->rlen || d->rrate != s->rrate
            || d->rvel != s->rvel || d->slock != s->lane[0])
            return 0;
    }
    return 1;
}

static void test_rotate(void)
{
    static const int what[8][2] = {
        { GEN_W_ALL, SEED_FLD_ALL }, { GEN_W_VEL, SEED_FLD_VEL }, { GEN_W_NOTE, SEED_FLD_NOTE },
        { GEN_W_LEN, SEED_FLD_LEN }, { GEN_W_UTIM, SEED_FLD_UTIME }, { GEN_W_COND, SEED_FLD_COND },
        { GEN_W_RTRG, SEED_FLD_RTRG }, { GEN_W_ACC, SEED_FLD_ACC } };
    struct seed_track t;
    struct gen_track g;
    int i, ok = 1, origins = 1;
    for (i = 0; i < 4000; i++) {
        int w = rnd(0, 7), mode = rnd(0, 1), from = rnd(0, 63), n = rnd(1, 64 - from), k = rnd(-70, 70), j;
        random_track(&t, 64);
        to_gen(&g, &t);
        gen_rotate(&g, from, n, k, (enum gen_what)what[w][0], (enum gen_rmode)mode);
        ok &= seed_rotate(&t, from, n, k, what[w][1], mode) == SEED_OK;
        ok &= same(&t, &g);
        if (what[w][1] == SEED_FLD_ALL) {
            /* the whole step moves, origin included: step j now carries the original step j - k */
            int kk = ((k % n) + n) % n;
            for (j = 0; j < n; j++)
                origins &= t.s[from + (j + kk) % n].origin == from + j;
        }
    }
    CHECK(ok);
    CHECK(origins);
}

static void test_weight_cond(void)
{
    int beat, i, a, b, ok = 1;
    for (beat = -1; beat <= 18; beat++)
        for (i = -2; i < 200; i++)
            ok &= seed_weight(i, beat) == gen_weight(i, beat);
    for (b = -1; b <= 10; b++)
        for (a = -1; a <= 10; a++)
            ok &= seed_cond_ab(a, b) == gen_cond_ab(a, b);
    CHECK(ok);
}

/* GROOVE's timing, velocity, quantize and shift, as 0.3 applied them */
static void test_groove(void)
{
    struct seed_track t, out;
    struct gen_track g;
    struct gen_groove gg;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int i, ok = 1, moved = 0;
    for (i = 0; i < 3000; i++) {
        struct gen_groove_params p;
        const struct seed_groove *sg;
        int len = rnd(1, 64);
        random_track(&t, len);
        seed_recipe_defaults(&r, &test_caps);
        r.grv = (int16_t)rnd(1, 3);
        r.timing = (int16_t)rnd(-100, 100);
        r.gvel = (int16_t)(rnd(0, 1) ? 0 : rnd(-100, 100));
        r.quant = (int16_t)rnd(0, 100);
        r.shft = (int16_t)rnd(-63, 63);
        sg = &test_grooves[r.grv - 1];
        memcpy(gg.name, sg->name, sizeof gg.name);
        gg.len = sg->len;
        gg.vmax = sg->vmax;
        memcpy(gg.off, sg->off, sizeof gg.off);
        memcpy(gg.vel, sg->vel, sizeof gg.vel);
        p.timing = r.timing;
        p.velocity = r.gvel;
        p.quant = r.quant;
        p.random = 0;
        p.shift = r.shft;
        p.seed = 1;
        to_gen(&g, &t);
        gen_groove_apply(&g, 0, len, &gg, &p, t.default_vel);
        ok &= seed_render(&out, &t, &r, &c, &work) == SEED_OK;
        out.s[0].origin = 0;
        ok &= same(&out, &g);
        moved += memcmp(&out, &t, sizeof out) != 0;
    }
    CHECK(ok);
    CHECK(moved > 2000);
}

int main(void)
{
    test_setup();
    test_rotate();
    test_weight_cond();
    test_groove();
    printf("test_port: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
