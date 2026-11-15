/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests: the invariants I1-I6 (docs/design-v1.md section 11) over random tracks and random
 * recipes on the layers task 0.2 built (RANDOM and TRACK, ROT, SKIP, the field rotation, DYNAMICS,
 * NOTE, LOCK, TRIG, TIME). Each test runs ROUNDS cases. */
#include "tests.h"

#define ROUNDS 2000

/* the fields of a step, as bits, for I3 */
enum { O_VEL = 1, O_NOTE = 2, O_LEN = 4, O_UTIME = 8, O_COND = 16, O_RTRG = 32, O_ACC = 64, O_LANE0 = 128,
       O_LANE1 = 256, O_RLEN = 512 };

/* 1 when a and b differ in a field outside `owned` on some step */
static int differs_outside(const struct seed_track *a, const struct seed_track *b, int owned)
{
    int i;
    uint16_t mask = 0xffffu;
    if (owned & O_RTRG)
        mask &= (uint16_t)~SEED_RETRIG;
    if (owned & O_ACC)
        mask &= (uint16_t)~SEED_ACCENT;
    if (a->length != b->length || a->fields != b->fields || a->default_note != b->default_note
        || a->default_vel != b->default_vel)
        return 1;
    for (i = 0; i < SEED_STEPS; i++) {
        const struct seed_step *x = &a->s[i], *y = &b->s[i];
        if ((x->flags & mask) != (y->flags & mask) || x->origin != y->origin || x->spare != y->spare)
            return 1;
        if ((!(owned & O_VEL) && x->vel != y->vel) || (!(owned & O_NOTE) && x->note != y->note)
            || (!(owned & O_LEN) && x->len != y->len) || (!(owned & O_UTIME) && x->utime != y->utime)
            || (!(owned & O_COND) && x->cond != y->cond)
            || (!(owned & O_RTRG) && (x->rrate != y->rrate || x->rvel != y->rvel))
            || (!(owned & O_RLEN) && x->rlen != y->rlen)
            || (!(owned & O_LANE0) && x->lane[0] != y->lane[0]) || (!(owned & O_LANE1) && x->lane[1] != y->lane[1]))
            return 1;
    }
    return 0;
}

static int render(struct seed_track *out, const struct seed_track *orig, const struct seed_recipe *r,
                  const struct seed_ctx *c)
{
    memset(out, 0x5a, sizeof *out);
    return seed_render(out, orig, r, c, &work);
}

/* I1: SOURCE = TRACK with every layer at 0 leaves the track byte-identical, whatever the knobs of
 * inactive layers say */
static void test_identity(void)
{
    struct seed_track orig, out;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int i, ok = 1;
    for (i = 0; i < ROUNDS; i++) {
        random_track(&orig, rnd(1, 64));
        random_recipe(&r);
        r.src = SEED_SRC_TRACK;
        r.rot = r.skip = 0;
        r.vel.lo = r.vel.hi = SEED_DEF;
        if (rnd(0, 1)) r.shape = SEED_SHAPE_FLAT; else r.amt = 0;
        r.acc = r.hum = 0;
        r.len.lo = r.note.lo = SEED_DEF;
        r.lane[0].pct = r.lane[1].pct = 0;
        r.cond = r.rtrg = 0;
        r.grv = r.quant = r.rand = 0;
        if (rnd(0, 1)) r.fld = SEED_FLD_OFF; else r.frot = 0;
        ok &= render(&out, &orig, &r, &c) == SEED_OK;
        ok &= memcmp(&out, &orig, sizeof out) == 0;
    }
    CHECK(ok);
}

/* I2: the same recipe, original and context give the same result, whatever the buffers held */
static void test_determinism(void)
{
    struct seed_track orig, a, b;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(3);
    int i, ok = 1;
    for (i = 0; i < ROUNDS; i++) {
        random_track(&orig, rnd(1, 64));
        random_recipe(&r);
        ok &= render(&a, &orig, &r, &c) == SEED_OK;
        memset(&work, rnd(0, 255), sizeof work);
        memset(&b, 0xc3, sizeof b);
        ok &= seed_render(&b, &orig, &r, &c, &work) == SEED_OK;
        ok &= memcmp(&a, &b, sizeof a) == 0;
    }
    CHECK(ok);
}

/* I3: changing one layer's settings changes only the fields that layer owns (P1) */
static void test_independence(void)
{
    struct seed_track orig, a, b;
    struct seed_recipe r, q;
    struct seed_ctx c = test_ctx(1);
    static const int fld_owns[SEED_FLD_N] = { 0, O_VEL, O_NOTE, O_LEN, O_UTIME, O_COND, O_RTRG | O_RLEN, O_ACC,
                                              O_LANE0, O_LANE1, 0 };
    int i, layer, ok[6] = { 1, 1, 1, 1, 1, 1 }, changed[6] = { 0 };
    for (i = 0; i < ROUNDS; i++) {
        random_track(&orig, rnd(1, 64));
        random_recipe(&r);
        if (render(&a, &orig, &r, &c) != SEED_OK) {
            ok[0] = 0;
            continue;
        }
        for (layer = 0; layer < 6; layer++) {
            int owned = 0;
            q = r;
            switch (layer) {
            case 0: random_dyn(&q); owned = O_VEL | O_ACC; break;
            case 1: random_note(&q); owned = O_NOTE | O_LEN; break;
            case 2: random_lock(&q); owned = O_LANE0 | O_LANE1; break;
            case 3: random_trig(&q); owned = O_COND | O_RTRG; break;
            case 4: random_time(&q); owned = O_UTIME | O_VEL; break;    /* GVEL: the one shared field */
            case 5: q.frot = (int16_t)rnd(-63, 63); q.fmod = (int16_t)rnd(0, 1); owned = fld_owns[q.fld]; break;
            }
            if (render(&b, &orig, &q, &c) != SEED_OK) {
                ok[layer] = 0;
                continue;
            }
            if (differs_outside(&a, &b, owned)) {
                ok[layer] = 0;
                printf("  I3 layer %d, round %d\n", layer, i);
            }
            changed[layer] += memcmp(&a, &b, sizeof a) != 0;
        }
    }
    for (layer = 0; layer < 6; layer++) {
        CHECK(ok[layer]);
        CHECK(changed[layer] > ROUNDS / 4);                   /* the perturbations did change something */
    }
}

/* P1 across RHYTHM: with another density, a trig that is there in both takes keeps its per-step
 * values (velocity, length, note, micro timing) */
static void test_values_follow_steps(void)
{
    struct seed_track orig, a, b;
    struct seed_recipe r, q;
    struct seed_ctx c = test_ctx(0);
    int i, j, ok = 1, both = 0;
    for (i = 0; i < ROUNDS; i++) {
        random_track(&orig, rnd(1, 64));
        random_recipe(&r);
        r.src = SEED_SRC_RANDOM;
        r.fld = SEED_FLD_OFF;                               /* moves values between trigs */
        if (r.shape == SEED_SHAPE_EVRY)
            r.shape = SEED_SHAPE_STRONG;                    /* counts trigs */
        q = r;
        q.dens = (int16_t)rnd(0, 100);
        ok &= render(&a, &orig, &r, &c) == SEED_OK && render(&b, &orig, &q, &c) == SEED_OK;
        for (j = 0; j < a.length; j++) {
            const struct seed_step *x = &a.s[j], *y = &b.s[j];
            if (!(x->flags & y->flags & SEED_TRIG))
                continue;
            both++;
            ok &= x->vel == y->vel && x->len == y->len && x->note == y->note && x->utime == y->utime;
        }
    }
    CHECK(ok);
    CHECK(both > ROUNDS);
}

/* I4: raising a share knob keeps every step it had already chosen (P2) */
static int has(const struct seed_step *s, int knob)
{
    switch (knob) {
    case 0: return (s->flags & SEED_TRIG) != 0;
    case 1: return (s->flags & SEED_ACCENT) != 0;
    case 2: return s->cond != SEED_NO_COND;
    case 3: return (s->flags & SEED_RETRIG) != 0;
    case 4: return s->lane[0] != SEED_NO_LOCK;
    default: return s->lane[1] != SEED_NO_LOCK;
    }
}

static void set_share(struct seed_recipe *r, int knob, int pct)
{
    switch (knob) {
    case 0: r->dens = (int16_t)pct; break;
    case 1: r->acc = (int16_t)pct; break;
    case 2: r->cond = (int16_t)pct; break;
    case 3: r->rtrg = (int16_t)pct; break;
    case 4: r->lane[0].pct = (int16_t)pct; break;
    default: r->lane[1].pct = (int16_t)pct; break;
    }
}

static void test_monotonic(void)
{
    struct seed_track orig, a, b;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(2);
    int i, j, knob, ok[6] = { 1, 1, 1, 1, 1, 1 }, grew[6] = { 0 };
    for (i = 0; i < ROUNDS / 4; i++) {
        random_track(&orig, rnd(1, 64));
        random_recipe(&r);
        r.src = SEED_SRC_RANDOM;
        for (knob = 0; knob < 6; knob++) {
            int pct, prev = 0;
            set_share(&r, knob, knob == 0 ? 0 : 1);         /* above 0 the layer owns the field */
            ok[knob] &= render(&a, &orig, &r, &c) == SEED_OK;
            for (pct = 2; pct <= 100; pct += rnd(1, 9)) {
                int m = 0;
                set_share(&r, knob, pct);
                ok[knob] &= render(&b, &orig, &r, &c) == SEED_OK;
                for (j = 0; j < b.length; j++) {
                    ok[knob] &= !has(&a.s[j], knob) || has(&b.s[j], knob);
                    m += has(&b.s[j], knob);
                }
                grew[knob] += m > prev;
                prev = m;
                a = b;
            }
        }
    }
    for (knob = 0; knob < 6; knob++) {
        CHECK(ok[knob]);
        CHECK(grew[knob] > 0);
    }
}

/* I5: DENS gives its exact count at the source; SKIP only removes */
static void test_counts(void)
{
    struct seed_track orig, a, b;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int i, j, ok = 1, okskip = 1;
    for (i = 0; i < ROUNDS; i++) {
        int len = rnd(1, 64), npos;
        random_track(&orig, len);
        random_recipe(&r);
        r.src = SEED_SRC_RANDOM;
        r.skip = 0;
        npos = (len + r.grid - 1) / r.grid;
        ok &= render(&a, &orig, &r, &c) == SEED_OK;
        ok &= ntrigs(&a) == seed_share(r.dens, npos);
        if (rnd(0, 1))
            r.src = SEED_SRC_TRACK;
        okskip &= render(&a, &orig, &r, &c) == SEED_OK;
        r.skip = (int16_t)rnd(1, 64);
        okskip &= render(&b, &orig, &r, &c) == SEED_OK;
        for (j = 0; j < len; j++)
            okskip &= j < r.skip ? !(b.s[j].flags & SEED_TRIG) : (b.s[j].flags & SEED_TRIG) == (a.s[j].flags & SEED_TRIG);
    }
    CHECK(ok);
    CHECK(okskip);
}

/* I6: the length is respected: steps at and past it, and the track's own settings, are unchanged */
static void test_scope(void)
{
    struct seed_track orig, out;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int i, ok = 1;
    for (i = 0; i < ROUNDS; i++) {
        int len = rnd(1, 64);
        random_track(&orig, len);
        random_recipe(&r);
        ok &= render(&out, &orig, &r, &c) == SEED_OK;
        ok &= out.length == orig.length && out.fields == orig.fields && out.default_note == orig.default_note
              && out.default_vel == orig.default_vel;
        ok &= memcmp(&out.s[len], &orig.s[len], (size_t)(SEED_STEPS - len) * sizeof out.s[0]) == 0;
    }
    CHECK(ok);
}

/* a field the track lacks is left as it is, whatever the recipe */
static void test_missing_fields(void)
{
    struct seed_track orig, out;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int i, j, ok = 1;
    for (i = 0; i < ROUNDS / 2; i++) {
        random_track(&orig, rnd(1, 64));
        orig.fields = (uint16_t)(0x1ff & ~(SEED_F_NOTE | SEED_F_UTIME | SEED_F_LOCKS));
        random_recipe(&r);
        r.src = SEED_SRC_TRACK;
        r.rot = r.skip = 0;
        r.fld = SEED_FLD_OFF;
        ok &= render(&out, &orig, &r, &c) == SEED_OK;
        for (j = 0; j < SEED_STEPS; j++)
            ok &= out.s[j].note == orig.s[j].note && out.s[j].utime == orig.s[j].utime
                  && out.s[j].lane[0] == orig.s[j].lane[0] && out.s[j].lane[1] == orig.s[j].lane[1];
    }
    CHECK(ok);
}

int main(void)
{
    test_setup();
    test_identity();
    test_determinism();
    test_independence();
    test_values_follow_steps();
    test_monotonic();
    test_counts();
    test_scope();
    test_missing_fields();
    printf("test_props: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
