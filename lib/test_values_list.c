/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for values_list.c: LIST (the items and their slots, each ORDR, the length, VARY) and
 * LMOD = LEGATO (every gap's length code), then through the chain on NOTE, LEN, both LOCK lanes and
 * VEL, with the invariants a value source owes: I1 (DEF leaves the field), I2, I3 (only the field's
 * own values change), I4 (a lane's % nests, values included) and ranges kept. */
#include "tests.h"

static struct seed_track orig, out, out2;
static struct seed_recipe r;
static struct seed_ctx c;

#define SLOT SEED_SLOT(SEED_L_NOTE, SEED_P_NOTE, 0)

static void list(struct seed_values *v, int lo, int hi, int order, int len)
{
    v->lo = (int16_t)lo;
    v->hi = (int16_t)hi;
    v->src = SEED_VSRC_LIST;
    v->order = (int16_t)order;
    v->len = (int16_t)len;
    v->zoom = 4;
}

/* the sorted list as UP walks it */
static void up(int *item, struct seed_values v, const struct seed_take *k)
{
    int i;
    v.order = SEED_ORDR_UP;
    for (i = 0; i < v.len; i++)
        item[i] = seed_value_list(&v, k, SLOT, i);
}

static void test_items(void)
{
    struct seed_values v;
    struct seed_take k;
    int item[SEED_LIST_MAX], drawn[SEED_LIST_MAX], seed, i, j, ok = 1;
    for (seed = 1; seed <= 50; seed++) {
        int n = 1 + seed % SEED_LIST_MAX;
        seed_take_init(&k, (uint32_t)seed, 3, 0, 0);
        list(&v, -24, 24, SEED_ORDR_UP, n);
        up(item, v, &k);
        /* item i from slot + 0x100 + i, sorted: the same multiset, low to high */
        for (i = 0; i < n; i++)
            drawn[i] = seed_rand_range(&k, SLOT + 0x100 + (uint32_t)i, -24, 24);
        for (i = 0; i < n; i++) {
            int below = 0, same = 0, at = 0;
            for (j = 0; j < n; j++) {
                below += drawn[j] < item[i];
                same += drawn[j] == item[i];
                at += item[j] == item[i];
            }
            ok &= below <= i && i < below + same && same == at;
            if (i)
                ok &= item[i - 1] <= item[i];
        }
        /* UP repeats every n trigs */
        for (i = 0; i < 64; i++)
            ok &= seed_value_list(&v, &k, SLOT, i) == item[i % n];
    }
    CHECK(ok);
}

static void test_orders(void)
{
    struct seed_values v;
    struct seed_take k;
    int item[SEED_LIST_MAX], n, i, ok = 1;
    for (n = 1; n <= SEED_LIST_MAX; n++) {
        seed_take_init(&k, (uint32_t)(100 + n), 0, 0, 0);
        list(&v, 0, 127, SEED_ORDR_UP, n);
        up(item, v, &k);
        for (i = 0; i < 64; i++) {
            int q, p;
            v.order = SEED_ORDR_DOWN;
            ok &= seed_value_list(&v, &k, SLOT, i) == item[n - 1 - i % n];
            v.order = SEED_ORDR_PING;
            q = n == 1 ? 0 : i % (2 * n - 2);
            p = q < n ? q : 2 * n - 2 - q;
            ok &= seed_value_list(&v, &k, SLOT, i) == item[p];
            v.order = SEED_ORDR_PNG2;
            q = i % (2 * n);
            p = q < n ? q : 2 * n - 1 - q;
            ok &= seed_value_list(&v, &k, SLOT, i) == item[p];
        }
    }
    CHECK(ok);
    /* by hand, on four items */
    seed_take_init(&k, 7, 0, 0, 0);
    list(&v, 0, 127, SEED_ORDR_UP, 4);
    up(item, v, &k);
    {
        static const int ping[8] = { 0, 1, 2, 3, 2, 1, 0, 1 }, png2[9] = { 0, 1, 2, 3, 3, 2, 1, 0, 0 };
        static const int odd4[8] = { 0, 2, 1, 3, 0, 2, 1, 3 }, odd5[6] = { 0, 2, 4, 1, 3, 0 };
        for (i = 0, ok = 1; i < 8; i++) {
            v.order = SEED_ORDR_PING;
            ok &= seed_value_list(&v, &k, SLOT, i) == item[ping[i]];
            v.order = SEED_ORDR_PNG2;
            ok &= seed_value_list(&v, &k, SLOT, i) == item[png2[i]];
            v.order = SEED_ORDR_ODD;
            ok &= seed_value_list(&v, &k, SLOT, i) == item[odd4[i]];
        }
        v.order = SEED_ORDR_PNG2;
        ok &= seed_value_list(&v, &k, SLOT, 8) == item[png2[8]];
        CHECK(ok);
        list(&v, 0, 127, SEED_ORDR_UP, 5);
        up(item, v, &k);
        v.order = SEED_ORDR_ODD;
        for (i = 0, ok = 1; i < 6; i++)
            ok &= seed_value_list(&v, &k, SLOT, i) == item[odd5[i]];
        CHECK(ok);
    }
    /* RAND: every pick is an item, all of them turn up, and it is a function of the take */
    {
        int seen = 0, same = 1;
        list(&v, 0, 127, SEED_ORDR_UP, 8);
        up(item, v, &k);
        v.order = SEED_ORDR_RAND;
        for (i = 0, ok = 1; i < 64; i++) {
            int x = seed_value_list(&v, &k, SLOT, i), j, hit = -1;
            for (j = 0; j < 8; j++)
                if (item[j] == x)
                    hit = j;
            ok &= hit >= 0;
            if (hit >= 0)
                seen |= 1 << hit;
            same &= x == seed_value_list(&v, &k, SLOT, i);
        }
        CHECK(ok && same);
        CHECK(seen == 0xff || item[0] == item[7]);
    }
}

static void test_limits(void)
{
    struct seed_values v;
    struct seed_take k;
    int i, ok = 1;
    seed_take_init(&k, 9, 0, 0, 0);
    /* a range of one value; either order of lo and hi */
    list(&v, 5, 5, SEED_ORDR_PING, 6);
    for (i = 0; i < 64; i++)
        ok &= seed_value_list(&v, &k, SLOT, i) == 5;
    CHECK(ok);
    list(&v, 10, -10, SEED_ORDR_RAND, 16);
    for (i = 0, ok = 1; i < 64; i++) {
        int x = seed_value_list(&v, &k, SLOT, i);
        ok &= x >= -10 && x <= 10;
    }
    CHECK(ok);
    /* length 1: one value; lengths outside 1..16 clamped; an unknown order is UP */
    list(&v, -24, 24, SEED_ORDR_UP, 1);
    for (i = 0, ok = 1; i < 64; i++)
        ok &= seed_value_list(&v, &k, SLOT, i) == seed_value_list(&v, &k, SLOT, 0);
    CHECK(ok);
    {
        struct seed_values w = v;
        w.len = 0;
        for (i = 0, ok = 1; i < 64; i++)
            ok &= seed_value_list(&w, &k, SLOT, i) == seed_value_list(&v, &k, SLOT, i);
        w.len = 99;
        v.len = SEED_LIST_MAX;
        for (i = 0; i < 64; i++)
            ok &= seed_value_list(&w, &k, SLOT, i) == seed_value_list(&v, &k, SLOT, i);
        w = v;
        w.order = 42;
        for (i = 0; i < 64; i++)
            ok &= seed_value_list(&w, &k, SLOT, i) == seed_value_list(&v, &k, SLOT, i);
        CHECK(ok);
    }
    /* through seed_value: by trig index, not by step */
    list(&v, 0, 127, SEED_ORDR_UP, 3);
    for (i = 0, ok = 1; i < 64; i++)
        ok &= seed_value(&v, &k, SLOT, i, 4) == seed_value_list(&v, &k, SLOT, 4);
    CHECK(ok);
}

static void test_vary(void)
{
    struct seed_values v;
    struct seed_take a, b;
    int i, n, same100 = 1, differ0 = 0, kept = 0, total = 0;
    list(&v, 0, 127, SEED_ORDR_UP, 16);
    for (n = 1; n <= 100; n++) {
        int x[16], y[16];
        seed_take_init(&a, (uint32_t)n, 0, 100, 0);
        seed_take_init(&b, (uint32_t)n + 1, 0, 100, 0);
        for (i = 0; i < 16; i++)
            same100 &= seed_value_list(&v, &a, SLOT, i) == seed_value_list(&v, &b, SLOT, i);
        seed_take_init(&a, (uint32_t)n, 0, 0, 0);
        seed_take_init(&b, (uint32_t)n + 1, 0, 0, 0);
        for (i = 0; i < 16; i++)
            differ0 |= seed_value_list(&v, &a, SLOT, i) != seed_value_list(&v, &b, SLOT, i);
        /* VARY 50: about half the items kept (compared as raw draws, before sorting) */
        seed_take_init(&a, (uint32_t)n, 0, 50, 0);
        seed_take_init(&b, (uint32_t)n + 1, 0, 50, 0);
        for (i = 0; i < 16; i++) {
            x[i] = (int)seed_rand(&a, SLOT + 0x100 + (uint32_t)i);
            y[i] = (int)seed_rand(&b, SLOT + 0x100 + (uint32_t)i);
            kept += x[i] == y[i];
            total++;
        }
    }
    CHECK(same100);
    CHECK(differ0);
    CHECK(kept > total * 40 / 100 && kept < total * 60 / 100);
}

/* ---- LEGATO ---- */

/* a length code's duration in 1/16 step, from the firmware notes' table, independently */
static int dur(int code)
{
    static const int from[8] = { 0, 14, 30, 46, 62, 78, 94, 110 }, base[8] = { 2, 16, 32, 64, 128, 256, 512, 1024 },
                     unit[8] = { 1, 1, 2, 4, 8, 16, 32, 64 };
    int s = 7;
    while (code < from[s])
        s--;
    return base[s] + unit[s] * (code - from[s]);
}

static void test_legato(void)
{
    struct seed_track t;
    int g, ok = 1;
    memset(&t, 0, sizeof t);
    /* a lone trig on tracks of 1..64 steps: the gap is the length */
    for (g = 1; g <= 64; g++) {
        int code;
        t.length = (int16_t)g;
        memset(t.s, 0, sizeof t.s);
        t.s[g - 1].flags = SEED_TRIG;
        code = seed_len_legato(&t, g - 1);
        ok &= code >= 0 && code <= 126 && dur(code) >= 16 * g && (code == 0 || dur(code - 1) < 16 * g);
    }
    CHECK(ok);
    /* known gaps: 1 step = 1/16 (14), 2 = 1/8 (30), 4 = 1/4 (46), 16 = 1/1 (78) */
    memset(t.s, 0, sizeof t.s);
    t.length = 32;
    t.s[0].flags = t.s[1].flags = t.s[3].flags = t.s[6].flags = t.s[10].flags = t.s[26].flags = SEED_TRIG;
    t.s[2].flags = SEED_MUTE;
    CHECK(seed_len_legato(&t, 0) == 14);
    CHECK(seed_len_legato(&t, 1) == 30);
    CHECK(seed_len_legato(&t, 3) == 38);                    /* 3 steps: 48/16, exact */
    CHECK(seed_len_legato(&t, 6) == 46);
    CHECK(seed_len_legato(&t, 10) == 78);
    CHECK(seed_len_legato(&t, 26) == 54);                   /* wraps: 6 steps to step 0 */
    /* steps past the length do not count */
    t.length = 20;
    t.s[24].flags = SEED_TRIG;
    CHECK(seed_len_legato(&t, 10) == 66);                   /* 10 steps to step 0 a loop later: 160/16 */
    /* an odd gap above 32 steps rounds up: 33 steps -> 34 */
    memset(t.s, 0, sizeof t.s);
    t.length = 64;
    t.s[0].flags = t.s[33].flags = SEED_TRIG;
    CHECK(dur(seed_len_legato(&t, 0)) == 16 * 34);
    CHECK(dur(seed_len_legato(&t, 33)) == 16 * 31);    /* 31 steps: exact */
}

/* ---- through the chain ---- */

static void base(void)
{
    random_track(&orig, 32);
    seed_recipe_defaults(&r, &test_caps);
}

static int render(struct seed_track *o)
{
    return seed_render(o, &orig, &r, &c, &work);
}

/* the value of trig index i, as the stage asks for it */
static int at(const struct seed_values *v, uint32_t slot, int idx)
{
    struct seed_take k;
    seed_take_init(&k, r.seed, (uint32_t)c.track, r.vary, 0);
    return seed_value_list(v, &k, slot, idx);
}

static void test_note(void)
{
    int i, idx, ok = 1;
    base();
    orig.default_note = 60;
    list(&r.note, -5, 7, SEED_ORDR_PING, 5);
    CHECK(render(&out) == SEED_OK);
    for (i = 0, idx = 0; i < 32; i++) {
        const struct seed_step *s = &out.s[i], *o = &orig.s[i];
        if (o->flags & SEED_TRIG)
            ok &= s->note == 60 + at(&r.note, SLOT, idx++);
        else
            ok &= s->note == o->note;
        ok &= s->len == o->len && s->vel == o->vel && s->lane[0] == o->lane[0];   /* I3 */
    }
    CHECK(ok);
    /* LEN from a list; the note left */
    base();
    list(&r.len, 14, 46, SEED_ORDR_DOWN, 3);
    CHECK(render(&out) == SEED_OK);
    for (i = 0, idx = 0, ok = 1; i < 32; i++) {
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].len == at(&r.len, SEED_SLOT(SEED_L_NOTE, SEED_P_LEN, 0), idx++);
        ok &= out.s[i].note == orig.s[i].note;
    }
    CHECK(ok);
    /* I1: LIST with DEF leaves the field; the identity holds */
    base();
    list(&r.note, 0, 0, SEED_ORDR_UP, 4);
    r.note.lo = SEED_DEF;
    CHECK(render(&out) == SEED_OK && memcmp(&out, &orig, sizeof out) == 0);
}

static void test_legato_stage(void)
{
    int i, ok = 1;
    base();
    r.lmod = SEED_LMOD_LEGATO;
    list(&r.note, -3, 3, SEED_ORDR_UP, 4);
    CHECK(render(&out) == SEED_OK);
    for (i = 0; i < 32; i++) {
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].len == seed_len_legato(&orig, i);
        else
            ok &= out.s[i].len == orig.s[i].len;
        ok &= out.s[i].vel == orig.s[i].vel && out.s[i].utime == orig.s[i].utime;
    }
    CHECK(ok);
    /* LEGATO runs after PLACE: it reaches the trigs where they end up */
    base();
    r.lmod = SEED_LMOD_LEGATO;
    r.src = SEED_SRC_RANDOM;
    r.dens = 25;
    r.rot = 3;
    r.skip = 2;
    CHECK(render(&out) == SEED_OK);
    for (i = 0, ok = 1; i < 32; i++)
        if (out.s[i].flags & SEED_TRIG)
            ok &= out.s[i].len == seed_len_legato(&out, i);
    CHECK(ok);
    /* a track without lengths: left */
    base();
    orig.fields &= (uint16_t)~SEED_F_LEN;
    r.lmod = SEED_LMOD_LEGATO;
    CHECK(render(&out) == SEED_OK && memcmp(&out, &orig, sizeof out) == 0);
}

static void test_lock(void)
{
    int l, i, idx, ok = 1, nest = 1, m = 0;
    base();
    for (l = 0; l < SEED_LANES; l++) {
        r.lane[l].pct = 50;
        list(&r.lane[l].val, l ? -40 : 1, l ? 40 : 8, l ? SEED_ORDR_ODD : SEED_ORDR_UP, l ? 6 : 8);
    }
    CHECK(render(&out) == SEED_OK);
    for (l = 0; l < SEED_LANES; l++)
        for (i = 0, idx = 0; i < 32; i++) {
            if (!(orig.s[i].flags & SEED_TRIG)) {
                ok &= out.s[i].lane[l] == SEED_NO_LOCK;
                continue;
            }
            if (out.s[i].lane[l] != SEED_NO_LOCK) {
                ok &= out.s[i].lane[l] == at(&r.lane[l].val, SEED_SLOT(SEED_L_LOCK, SEED_P_LOCK_VAL + l, 0), idx);
                m += l == 0;
            }
            idx++;
        }
    CHECK(ok);
    CHECK(m == seed_share(50, ntrigs(&orig)));
    /* I4: raising % keeps every lock with its value */
    r.lane[0].pct = 80;
    CHECK(render(&out2) == SEED_OK);
    for (i = 0; i < 32; i++)
        if (out.s[i].lane[0] != SEED_NO_LOCK)
            nest &= out2.s[i].lane[0] == out.s[i].lane[0];
    CHECK(nest);
    /* values outside the parameter clamped */
    base();
    r.lane[1].pct = 100;
    list(&r.lane[1].val, 50, 300, SEED_ORDR_RAND, 16);
    CHECK(render(&out) == SEED_OK);
    for (i = 0, ok = 1; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].lane[1] >= 50 && out.s[i].lane[1] <= 63;
    CHECK(ok);
}

static void test_others(void)
{
    int i, ok = 1;
    /* VEL, CVAL and RATE take a list as well (V9) */
    base();
    list(&r.vel, 20, 120, SEED_ORDR_PNG2, 3);
    r.rtrg = 100;
    list(&r.rate, 2, 12, SEED_ORDR_DOWN, 2);
    CHECK(render(&out) == SEED_OK);
    for (i = 0; i < 32; i++)
        if (orig.s[i].flags & SEED_TRIG)
            ok &= out.s[i].vel >= 20 && out.s[i].vel <= 120 && out.s[i].rrate >= 2 && out.s[i].rrate <= 12;
    CHECK(ok);
}

/* random tracks and recipes with lists on: renders, deterministic (I2), only NOTE's and LOCK's fields */
static void test_random(void)
{
    int t, i, ok = 1, det = 1, own = 1;
    for (t = 0; t < 2000; t++) {
        random_track(&orig, rnd(1, 64));
        seed_recipe_defaults(&r, &test_caps);
        r.seed = (uint32_t)rnd(0, 9999);
        r.vary = (int16_t)(rnd(0, 1) ? 0 : rnd(0, 100));
        if (rnd(0, 1))
            list(&r.note, rnd(-24, 24), rnd(-24, 24), rnd(0, SEED_ORDR_N - 1), rnd(1, 16));
        if (rnd(0, 1))
            list(&r.len, rnd(0, 127), rnd(0, 127), rnd(0, SEED_ORDR_N - 1), rnd(1, 16));
        r.lmod = (int16_t)rnd(0, 1);
        for (i = 0; i < SEED_LANES; i++) {
            r.lane[i].pct = (int16_t)(rnd(0, 1) ? rnd(1, 100) : 0);
            list(&r.lane[i].val, rnd(-80, 140), rnd(-80, 140), rnd(0, SEED_ORDR_N - 1), rnd(1, 16));
        }
        ok &= render(&out) == SEED_OK;
        det &= render(&out2) == SEED_OK && memcmp(&out, &out2, sizeof out) == 0;
        for (i = 0; i < SEED_STEPS; i++) {
            const struct seed_step *x = &out.s[i], *o = &orig.s[i];
            own &= x->flags == o->flags && x->vel == o->vel && x->utime == o->utime && x->cond == o->cond
                && x->origin == o->origin && x->rrate == o->rrate;
            if (i >= orig.length)
                own &= memcmp(x, o, sizeof *x) == 0;
            else if (x->flags & SEED_TRIG)
                own &= (r.len.lo == SEED_DEF && r.lmod == SEED_LMOD_RANGE ? x->len == o->len : x->len <= 127)
                    && (r.note.lo == SEED_DEF ? x->note == o->note : x->note <= 126) && (x->lane[0] == SEED_NO_LOCK || (x->lane[0] >= 0 && x->lane[0] <= 127))
                    && (x->lane[1] == SEED_NO_LOCK || (x->lane[1] >= -64 && x->lane[1] <= 63));
        }
    }
    CHECK(ok);
    CHECK(det);
    CHECK(own);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_items();
    test_orders();
    test_limits();
    test_vary();
    test_legato();
    test_note();
    test_legato_stage();
    test_lock();
    test_others();
    test_random();
    printf("test_values_list: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
