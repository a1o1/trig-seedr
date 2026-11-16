/* SPDX-License-Identifier: GPL-2.0-or-later */
/* test_vary: PHRS over the pages and CVAR as conditions (task F9, lib/vary.c and the PHRS loop
 * in seed_render.c). Host only. */
#include "tests.h"
#include "props.h"

static struct seed_ctx c;
static struct seed_track orig, out, ref, ref16, tmp;

#define AB12 30                       /* seed_cond_ab(1, 2) */
#define AB22 31
#define AB44 38
#define NOT_PRE 25

/* the recipe's PHRS letters, A = 0 */
static const char *const words[SEED_PHRS_N] = { "AAAA", "AAAA", "AAAB", "ABAB", "ABAC", "ABCD" };

static void test_phrase_take(void)
{
    int f, p, bad = 0;
    for (f = 0; f < SEED_PHRS_N; f++)
        for (p = 0; p < 4; p++)
            bad += seed_phrase_take(f, p) != words[f][p] - 'A';
    CHECK(bad == 0);
    CHECK(seed_phrase_take(-1, 0) == SEED_E_ARG);
    CHECK(seed_phrase_take(SEED_PHRS_N, 0) == SEED_E_ARG);
    CHECK(seed_phrase_take(SEED_PHRS_ABCD, 4) == SEED_E_ARG);
    CHECK(seed_phrase_take(SEED_PHRS_ABCD, -1) == SEED_E_ARG);
}

/* a random track whose four pages hold the same steps (origin = index) */
static void periodic_track(struct seed_track *t, int len)
{
    int i;
    random_track(t, len);
    for (i = SEED_PAGE; i < SEED_STEPS; i++) {
        t->s[i] = t->s[i % SEED_PAGE];
        t->s[i].origin = (int8_t)i;
    }
}

/* a random recipe that PHRS repeats exactly on a periodic track: no SKIP, a groove of 2 or 16 steps */
static void phrase_recipe(struct seed_recipe *r)
{
    random_recipe(r);
    r->skip = 0;
    r->grv = (int16_t)rnd(0, 2);
}

/* the 16-step phrase PHRS should give page `base`: the original's page, steps from m on past the
 * track's end (no trig), rendered with PHRS OFF as take seed */
static int phrase_ref(struct seed_track *t, const struct seed_track *o, const struct seed_recipe *r, int base,
                      int m, uint32_t seed)
{
    struct seed_recipe q = *r;
    int i;
    seed_track_copy(&ref16, o);
    /* a short last page of placed trigs (TRACK, COPY, EUCLID) is a phrase of its own length (M2.1) */
    ref16.length = (int16_t)(m < SEED_PAGE && r->src != SEED_SRC_RANDOM && r->src != SEED_SRC_MAP ? m : SEED_PAGE);
    for (i = 0; i < SEED_PAGE; i++) {
        ref16.s[i] = o->s[base + i];
        ref16.s[i].origin = (int8_t)i;
        if (i >= m)
            seed_step_clear(&ref16.s[i]);
    }
    q.phrs = SEED_PHRS_OFF;
    q.seed = seed;
    return seed_render(t, &ref16, &q, &c, &work);
}

/* steps [base, base + m) of t are the phrase's steps 0..m-1, origins moved by base */
static int page_is(const struct seed_track *t, const struct seed_track *phrase, int base, int m)
{
    int i;
    for (i = 0; i < m; i++) {
        struct seed_step s = phrase->s[i];
        if (s.origin != SEED_NO_ORIGIN)
            s.origin = (int8_t)(s.origin + base < t->length ? s.origin + base : SEED_NO_ORIGIN);
        if (memcmp(&s, &t->s[base + i], sizeof s))
            return 0;
    }
    return 1;
}

/* each page is the letter's take as a 16-step phrase, so pages with the same letter are equal;
 * every source (TRACK and RANDOM) and every 0.2 layer; also with CVAR */
static void test_letters(void)
{
    struct seed_recipe r;
    int i, p, bad = 0, refused = 0, same = 0, pairs = 0;
    for (i = 0; i < 600; i++) {
        int len = i % 3 ? SEED_STEPS : rnd(SEED_PAGE + 1, SEED_STEPS - 1);
        periodic_track(&orig, len);
        phrase_recipe(&r);
        r.phrs = (int16_t)rnd(SEED_PHRS_AAAA, SEED_PHRS_ABCD);
        r.cvar = (int16_t)(rnd(0, 3) ? SEED_CVAR_OFF : rnd(SEED_CVAR_1_2, SEED_CVAR_AAAB));
        if (seed_render(&out, &orig, &r, &c, &work) != SEED_OK) {
            refused++;
            continue;
        }
        for (p = 0; p * SEED_PAGE < len; p++) {
            int base = p * SEED_PAGE, m = len - base < SEED_PAGE ? len - base : SEED_PAGE;
            if (phrase_ref(&ref, &orig, &r, base, m, r.seed + (uint32_t)(words[r.phrs][p] - 'A')) != SEED_OK
                || !page_is(&out, &ref, base, m))
                bad++;
        }
        /* pages with the same letter, full pages: equal apart from the origins */
        if (len == SEED_STEPS && r.src == SEED_SRC_RANDOM) {
            int a, b;
            for (a = 0; a < 4; a++)
                for (b = a + 1; b < 4; b++) {
                    int k, eq = 1;
                    for (k = 0; k < SEED_PAGE; k++) {
                        struct seed_step x = out.s[a * SEED_PAGE + k], y = out.s[b * SEED_PAGE + k];
                        x.origin = y.origin = 0;
                        eq &= !memcmp(&x, &y, sizeof x);
                    }
                    if (words[r.phrs][a] == words[r.phrs][b])
                        bad += !eq;
                    else if (r.vary == 0 && r.bias == 0 && r.grid == 1 && r.dens > 10 && r.dens < 90) {
                        pairs++;
                        same += eq;
                    }
                }
        }
    }
    printf("PHRS: %d renders refused (a layer not built); different letters equal on %d of %d page pairs\n",
           refused, same, pairs);
    CHECK(bad == 0);
    CHECK(refused < 60);
    CHECK(pairs >= 15 && same * 20 < pairs);
}

/* a track of one page or less is one phrase: PHRS changes nothing */
static void test_short(void)
{
    struct seed_recipe r;
    int i, bad = 0;
    for (i = 0; i < 300; i++) {
        random_track(&orig, rnd(1, SEED_PAGE));
        random_recipe(&r);
        r.phrs = SEED_PHRS_OFF;
        if (seed_render(&ref, &orig, &r, &c, &work) != SEED_OK)
            continue;
        r.phrs = (int16_t)rnd(SEED_PHRS_AAAA, SEED_PHRS_ABCD);
        bad += seed_render(&out, &orig, &r, &c, &work) != SEED_OK || memcmp(&out, &ref, sizeof out);
    }
    CHECK(bad == 0);
    random_track(&orig, 40);
    seed_recipe_defaults(&r, &test_caps);
    r.phrs = SEED_PHRS_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    random_track(&orig, 8);
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

/* SOURCE = TRACK, every layer at 0: PHRS leaves the track byte-identical (I1) */
static void test_identity(void)
{
    struct seed_recipe r;
    int i, bad = 0;
    for (i = 0; i < 200; i++) {
        random_track(&orig, rnd(1, SEED_STEPS));
        seed_recipe_defaults(&r, &test_caps);
        r.seed = (uint32_t)rnd(0, 9999);
        r.vary = (int16_t)rnd(0, 100);
        r.phrs = (int16_t)rnd(SEED_PHRS_AAAA, SEED_PHRS_ABCD);
        bad += seed_render(&out, &orig, &r, &c, &work) != SEED_OK || memcmp(&out, &orig, sizeof out);
    }
    CHECK(bad == 0);
}

/* what belongs to the track's positions stays there under PHRS: SKIP clears the track's first
 * steps, the groove runs along the track, and the origins come from the page's own steps */
static void test_positions(void)
{
    struct seed_recipe r, q;
    int i, k, bad_skip = 0, bad_groove = 0, bad_origin = 0, skipped = 0;
    for (i = 0; i < 300; i++) {
        int len = rnd(SEED_PAGE + 1, SEED_STEPS);
        random_track(&orig, len);
        seed_recipe_defaults(&r, &test_caps);
        r.seed = (uint32_t)rnd(0, 9999);
        r.src = SEED_SRC_RANDOM;
        r.dens = (int16_t)rnd(20, 100);
        r.phrs = (int16_t)rnd(SEED_PHRS_AAAA, SEED_PHRS_ABCD);
        q = r;
        q.skip = (int16_t)rnd(1, len);
        if (seed_render(&ref, &orig, &r, &c, &work) != SEED_OK || seed_render(&out, &orig, &q, &c, &work) != SEED_OK) {
            bad_skip++;
            continue;
        }
        for (k = 0; k < len; k++) {
            int want = k < q.skip ? 0 : (ref.s[k].flags & SEED_TRIG);
            bad_skip += (out.s[k].flags & SEED_TRIG) != want;
            skipped += k < q.skip && (ref.s[k].flags & SEED_TRIG);
        }
        /* the 7-step groove at full TIME, nothing else on micro timing */
        q = r;
        q.grv = 3;
        q.timing = 100;
        q.shft = (int16_t)rnd(-20, 20);
        if (seed_render(&out, &orig, &q, &c, &work) != SEED_OK) {
            bad_groove++;
            continue;
        }
        for (k = 0; k < len; k++) {
            int g = test_grooves[2].off[((k - q.shft) % 7 + 7) % 7];
            if (out.s[k].flags & SEED_TRIG)
                bad_groove += out.s[k].utime != (g > 23 ? 23 : g < -23 ? -23 : g);
        }
        /* SOURCE = TRACK with ROT and SYNC: every origin is a step of the same page, inside the track */
        q = r;
        q.src = SEED_SRC_TRACK;
        q.rot = (int16_t)rnd(-20, 20);
        q.sync = (int16_t)rnd(0, 100);
        if (seed_render(&out, &orig, &q, &c, &work) != SEED_OK) {
            bad_origin++;
            continue;
        }
        for (k = 0; k < len; k++) {
            int o = out.s[k].origin;
            if (o == SEED_NO_ORIGIN)
                bad_origin += (out.s[k].flags & SEED_TRIG) != 0;
            else
                bad_origin += o >= len || o / SEED_PAGE != k / SEED_PAGE;
        }
    }
    printf("PHRS with SKIP: %d trigs cleared\n", skipped);
    CHECK(bad_skip == 0);
    CHECK(skipped > 100);
    CHECK(bad_groove == 0);
    CHECK(bad_origin == 0);
}

/* MASK and COPY read the other tracks at the track's positions, as without PHRS */
static void test_other_tracks(void)
{
    struct seed_ctx cx = c;
    struct seed_recipe r;
    int i, k, bad = 0, n;
    cx.tracks = 3;
    for (i = 0; i < 200; i++) {
        int len = rnd(SEED_PAGE + 1, SEED_STEPS);
        n = rnd(1, SEED_STEPS);
        memset(cx.trig, 0, sizeof cx.trig);
        cx.length[2] = (uint8_t)n;
        for (k = 0; k < n; k++)
            if (rnd(0, 2) == 0)
                cx.trig[2][k >> 3] |= (uint8_t)(1u << (k & 7));
        random_track(&orig, len);
        seed_recipe_defaults(&r, &test_caps);
        r.phrs = (int16_t)rnd(SEED_PHRS_AAAA, SEED_PHRS_ABCD);
        r.seed = (uint32_t)i;
        r.src = SEED_SRC_COPY;
        r.copy_trk = 2;
        if (seed_render(&out, &orig, &r, &cx, &work) != SEED_OK) {
            bad++;
            continue;
        }
        for (k = 0; k < len; k++)
            bad += ((out.s[k].flags & SEED_TRIG) != 0) != seed_ctx_trig(&cx, 2, k % n);
        /* MASK AVOID on SOURCE = TRACK: no trig is left where track 2 has one */
        seed_recipe_defaults(&r, &test_caps);
        r.phrs = SEED_PHRS_ABAB;
        r.mask = SEED_MASK_AVOID;
        r.mtrk = 2;
        if (seed_render(&out, &orig, &r, &cx, &work) != SEED_OK) {
            bad++;
            continue;
        }
        for (k = 0; k < len; k++) {
            int want = (orig.s[k].flags & SEED_TRIG) && !seed_ctx_trig(&cx, 2, k % n);
            bad += ((out.s[k].flags & SEED_TRIG) != 0) != want;
        }
    }
    CHECK(bad == 0);
}

/* ---- CVAR ---- */

static void two(struct seed_track *a, struct seed_track *b, const char *ta, const char *tb)
{
    int i;
    random_track(a, (int)strlen(ta));
    *b = *a;
    for (i = 0; ta[i]; i++) {
        a->s[i].flags = (uint16_t)((a->s[i].flags & ~SEED_TRIG) | (ta[i] == 'x' ? SEED_TRIG : 0));
        b->s[i].flags = (uint16_t)((b->s[i].flags & ~SEED_TRIG) | (tb[i] == 'x' ? SEED_TRIG : 0));
        b->s[i].vel = (uint8_t)(a->s[i].vel ^ 1);       /* tells the takes' steps apart */
    }
}

static void test_merge(void)
{
    static struct seed_track a, b, m;
    struct seed_recipe r;
    /*                                 both, a only, b only, neither ... */
    const char *ta = "xx.x.x..", *tb = "x.xx..x.";
    int i, bad = 0;
    seed_recipe_defaults(&r, &test_caps);
    two(&a, &b, ta, tb);
    a.s[0].cond = 7;                                     /* a trig in both keeps its own condition */
    r.cvar = SEED_CVAR_1_2;
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_OK);
    for (i = 0; i < 8; i++) {
        int wa = ta[i] == 'x', wb = tb[i] == 'x';
        const struct seed_step *want = wb && !wa ? &b.s[i] : &a.s[i];
        struct seed_step s = m.s[i];
        bad += ((s.flags & SEED_TRIG) != 0) != (wa || wb);
        bad += s.vel != want->vel || s.note != want->note || s.lane[0] != want->lane[0] || s.origin != want->origin;
        bad += s.cond != (wa && wb ? a.s[i].cond : wa ? AB12 : wb ? AB22 : a.s[i].cond);
    }
    for (i = 8; i < SEED_STEPS; i++)
        bad += memcmp(&m.s[i], &a.s[i], sizeof m.s[i]) != 0;       /* past the length: a's */
    CHECK(bad == 0);
    CHECK(m.length == 8 && m.fields == a.fields && m.default_vel == a.default_vel);
    /* AAAB: b only 4:4; a only !PRE after the first 4:4 trig, plain before it */
    r.cvar = SEED_CVAR_AAAB;
    two(&a, &b, "x.xx.x.x", "..x..xx.");
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_OK);
    CHECK(m.s[0].cond == a.s[0].cond);                   /* a only, before any 4:4 */
    CHECK(m.s[2].cond == a.s[2].cond);                   /* both */
    CHECK(m.s[3].cond == a.s[3].cond);                   /* a only, before the first 4:4 */
    CHECK(m.s[5].cond == a.s[5].cond);                   /* both */
    CHECK(m.s[6].cond == AB44 && m.s[6].vel == b.s[6].vel);
    CHECK(m.s[7].cond == NOT_PRE);
    two(&a, &b, "x..x", ".x..");
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_OK);
    CHECK(m.s[0].cond == a.s[0].cond && m.s[1].cond == AB44 && m.s[3].cond == NOT_PRE);
    /* out may be a */
    r.cvar = SEED_CVAR_1_2;
    two(&a, &b, ta, tb);
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_OK);
    CHECK(seed_cvar_merge(&a, &a, &b, &r) == SEED_OK);
    CHECK(memcmp(&a, &m, sizeof a) == 0);
    /* refused */
    r.cvar = SEED_CVAR_OFF;
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_E_ARG);
    r.cvar = SEED_CVAR_N;
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_E_ARG);
    r.cvar = SEED_CVAR_1_2;
    a.fields &= (uint16_t)~SEED_F_COND;
    CHECK(seed_cvar_merge(&m, &a, &b, &r) == SEED_E_NA);
}

/* through the chain: the trigs are those of takes n and n + 1 together, each with the condition
 * of the takes it is in; VARY 100 (n + 1 = n) gives take n unchanged */
static void test_cvar_render(void)
{
    struct seed_recipe r;
    int i, k, bad = 0, only = 0;
    for (i = 0; i < 400; i++) {
        random_track(&orig, rnd(1, SEED_STEPS));
        random_recipe(&r);
        r.phrs = SEED_PHRS_OFF;
        r.cond = 0;
        if (seed_render(&ref, &orig, &r, &c, &work) != SEED_OK)
            continue;
        r.seed++;
        if (seed_render(&tmp, &orig, &r, &c, &work) != SEED_OK) {
            bad++;
            continue;
        }
        r.seed--;
        r.cvar = SEED_CVAR_1_2;
        if (seed_render(&out, &orig, &r, &c, &work) != SEED_OK) {
            bad++;
            continue;
        }
        for (k = 0; k < orig.length; k++) {
            int a = (ref.s[k].flags & SEED_TRIG) != 0, b = (tmp.s[k].flags & SEED_TRIG) != 0;
            const struct seed_step *s = &out.s[k];
            bad += ((s->flags & SEED_TRIG) != 0) != (a || b);
            bad += s->cond != (a && b ? ref.s[k].cond : a ? AB12 : b ? AB22 : ref.s[k].cond);
            bad += s->vel != (b && !a ? tmp.s[k].vel : ref.s[k].vel);
            only += a != b;
        }
        r.vary = 100;
        if (seed_render(&ref, &orig, &r, &c, &work) == SEED_OK) {
            r.cvar = SEED_CVAR_OFF;
            bad += seed_render(&tmp, &orig, &r, &c, &work) != SEED_OK || memcmp(&ref, &tmp, sizeof ref);
        }
    }
    printf("CVAR 1:2: %d trigs in one take only\n", only);
    CHECK(bad == 0);
    CHECK(only > 500);
}

/* I2, I6 and I3 with PHRS and CVAR on (props.h) */
static void test_props_frame(void)
{
    struct prop_rng g = { 0x5eed0009u };
    int i, L, i2 = 0, i3 = 0;
    for (i = 0; i < 600; i++) {
        static struct seed_track t;
        struct seed_recipe q;
        struct seed_ctx cx;
        prop_track(&t, &g, prop_int(&g, 1, SEED_STEPS), prop_int(&g, 0, 100));
        prop_ctx(&cx, &t, prop_int(&g, 0, 12));
        prop_recipe(&q, &g, 50);
        q.phrs = (int16_t)prop_int(&g, SEED_PHRS_OFF, SEED_PHRS_ABCD);
        q.cvar = (int16_t)prop_int(&g, SEED_CVAR_OFF, SEED_CVAR_AAAB);
        i2 += !prop_i2_i6(&t, &q, &cx);
        for (L = SEED_L_DYN; L < SEED_L_N; L++) {
            unsigned ch;
            i3 += !prop_i3(&t, &q, &cx, &g, (enum seed_layer)L, &ch);
        }
    }
    CHECK(i2 == 0);
    CHECK(i3 == 0);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    CHECK(seed_have_vary == 1);
    test_phrase_take();
    test_letters();
    test_short();
    test_identity();
    test_positions();
    test_other_tracks();
    test_merge();
    test_cvar_render();
    test_props_frame();
    printf("test_vary: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
