/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for seed_render.c: the chain skeleton on the 0.1 stubs. Identity at SOURCE = TRACK
 * (I1 for lib/), determinism (I2), steps past the length (I6), the protective default (V6), and a
 * layer that is not built refusing instead of doing nothing. */
#include <stdio.h>
#include <string.h>
#include "seed.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static struct seed_work work;

/* a track as the device would read it: some trigs, fields and lanes, origin = index */
static void sample(struct seed_track *t, int len)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = (int16_t)len;
    t->fields = 0x1ff;
    t->default_note = 60;
    t->default_vel = 100;
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = (uint16_t)((i % 3 == 0 ? SEED_TRIG : 0) | (i & 1 ? SEED_SWING : 0) | (i % 7 == 0 ? 0x0400u : 0));
        s->note = (uint8_t)(48 + i % 12);
        s->vel = (uint8_t)(20 + i);
        s->len = SEED_DEFAULT;
        s->utime = (int8_t)(i % 5 - 2);
        s->cond = i % 4 ? SEED_NO_COND : 10;
        s->rlen = SEED_NEW_RLEN;
        s->rrate = SEED_NEW_RRATE;
        s->origin = (int8_t)i;
        s->lane[0] = i % 2 ? SEED_NO_LOCK : (int16_t)(i % 8);
        s->lane[1] = SEED_NO_LOCK;
    }
}

static struct seed_ctx ctx(int track)
{
    struct seed_ctx c;
    memset(&c, 0, sizeof c);
    c.track = track;
    c.tracks = 1;
    c.length[0] = 16;
    return c;
}

static void test_identity(void)
{
    struct seed_track orig, out, again;
    struct seed_recipe r;
    struct seed_ctx c = ctx(0);
    int len, i;
    seed_recipe_defaults(&r, NULL);
    CHECK(r.src == SEED_SRC_TRACK);
    for (i = 0; i < SEED_L_N; i++)
        CHECK(!seed_layer_active(&r, (enum seed_layer)i));
    for (len = 1; len <= SEED_STEPS; len += 7) {
        sample(&orig, len);
        memset(&out, 0xa5, sizeof out);
        CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
        CHECK(memcmp(&out, &orig, sizeof out) == 0);          /* byte-identical, lanes included */
        r.seed = 99;                                        /* the seed alone changes nothing */
        r.vary = 50;
        CHECK(seed_render(&again, &orig, &r, &c, &work) == SEED_OK);
        CHECK(memcmp(&again, &orig, sizeof again) == 0);
        r.seed = 1;
        r.vary = 0;
    }
}

/* the knobs that make each layer active; the stubs then refuse (SEED_E_NA) */
static void test_not_available(void)
{
    struct seed_track orig, out;
    struct seed_recipe r;
    struct seed_ctx c = ctx(0);
    int i;
    sample(&orig, 16);
    for (i = 0; i < 18; i++) {
        seed_recipe_defaults(&r, NULL);
        switch (i) {
        case 0: r.src = SEED_SRC_RANDOM; break;
        case 1: r.src = SEED_SRC_EUCLID; break;
        case 2: r.src = SEED_SRC_MAP; break;
        case 3: r.src = SEED_SRC_COPY; break;
        case 4: r.rot = 1; break;
        case 5: r.sync = 10; break;
        case 6: r.mask = SEED_MASK_AVOID; break;
        case 7: r.skip = 2; break;
        case 8: r.vel.lo = r.vel.hi = 90; break;
        case 9: r.note.lo = r.note.hi = 2; break;
        case 10: r.lane[1].pct = 50; break;
        case 11: r.cond = 20; break;
        case 12: r.rand = 30; break;
        case 13: r.fld = SEED_FLD_VEL; r.frot = 1; break;
        case 14: r.phrs = SEED_PHRS_AAAB; break;
        case 15: r.cvar = SEED_CVAR_1_2; break;
        case 16: r.qual = 4; break;
        case 17: r.acc = 10; break;
        }
        CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_NA);
    }
    seed_recipe_defaults(&r, NULL);
    r.src = 77;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    /* a knob of an inactive layer does not make it active */
    seed_recipe_defaults(&r, NULL);
    r.frot = 5;                                             /* FLD is OFF */
    r.amt = 50;                                             /* SHAPE is FLAT */
    r.timing = -40;                                         /* no groove */
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_OK);
    CHECK(memcmp(&out, &orig, sizeof out) == 0);
}

static void test_init(void)
{
    struct seed_track t;
    struct seed_recipe r, last;
    struct seed_caps caps;
    memset(&caps, 0, sizeof caps);
    caps.lane_parm[0] = 5;
    caps.lane_parm[1] = 2;
    sample(&t, 16);
    seed_recipe_defaults(&last, NULL);
    last.metr = SEED_METR_7_8;
    last.beat = 2;
    seed_recipe_init(&r, &t, &last, &caps);
    CHECK(r.src == SEED_SRC_TRACK);                         /* V6: a track with trigs */
    CHECK(r.metr == SEED_METR_7_8 && r.beat == 2);          /* A7 */
    CHECK(r.lane[0].parm == 5 && r.lane[1].parm == 2);
    t.s[0].flags = t.s[3].flags = t.s[6].flags = t.s[9].flags = t.s[12].flags = t.s[15].flags = 0;
    t.s[18].flags = SEED_TRIG;                              /* past the length: does not count */
    seed_recipe_init(&r, &t, NULL, NULL);
    CHECK(r.src == SEED_SRC_RANDOM);                        /* V6: an empty track */
    CHECK(r.metr == SEED_METR_4_4 && r.beat == 4);
}

static void test_clear(void)
{
    struct seed_step s;
    memset(&s, 0, sizeof s);
    s.flags = SEED_TRIG | SEED_ACCENT | SEED_SWING | 0x0400u;
    s.vel = 33;
    s.origin = 4;
    s.lane[0] = 3;
    seed_step_clear(&s);
    CHECK(s.flags == (SEED_SWING | 0x0400u));
    CHECK(s.vel == SEED_DEFAULT && s.note == SEED_DEFAULT && s.len == SEED_DEFAULT);
    CHECK(s.cond == SEED_NO_COND && s.origin == SEED_NO_ORIGIN && s.lane[0] == 3);
    CHECK(s.rlen == SEED_NEW_RLEN && s.rrate == SEED_NEW_RRATE);
}

int main(void)
{
    test_identity();
    test_not_available();
    test_init();
    test_clear();
    printf("test_seed_render: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
