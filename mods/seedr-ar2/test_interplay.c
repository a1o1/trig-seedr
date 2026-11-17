/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for task M2.1 (interplay, docs/status/M2.1.md): the page tables (pages.c) and the
 * chain (lib/) together, over recipes made only of values the pages can reach, from every source
 * (TRACK, RAND, EUCLID, MAP, COPY), on tracks of 13, 16, 24, 40 and 64 steps:
 *   1. a row the pages hide does not change the take (so a hidden value is harmless);
 *   2. what holds for every reachable recipe: no refusal, determinism, steps past the length
 *      untouched, SKIP and MASK on the result (P5), origins and field values in range, the
 *      counts each source promises (I5) through ROT and SYNC;
 *   3. PHRS keeps EUCLID's cycle along the track;
 *   4. the PR_SENSE rules, one by one;
 *   5. A:B's CVAL reads as trig.c uses it.
 * Built by the Makefile with the library's flags as they are (all built). */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pages.h"
#include "../../lib/props.h"

static int checks, failed;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failed++; if (failed <= 40) { printf("FAIL %s:%d: ", __FILE__, \
    __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)

#define NTRACKS 13
static const struct seed_param params[2] = { { "SMPL", 19, 0, 127 }, { "FRQ", 8, 0, 127 } };
static const struct seed_caps caps = { NTRACKS, 64, 0x1ff, -23, 23, 2, 2, params, { 0, 1 } };

static struct seed_recipe r;
static struct seed_shared sh;
static struct seed_tool tool;
static int cursor;
static struct seed_track orig, other, out, base_out;
static struct seed_ctx ctx;
static struct seed_work work;
static struct prop_rng g = { 20261116 };

static const char *names(int t)
{
    static const char *const n[NTRACKS] = { "BD", "SD", "RS", "CP", "BT", "LT", "MT", "HT", "CH", "OH", "CY", "CB", "FX" };
    return t >= 0 && t < NTRACKS ? n[t] : "?";
}

static void page_ctx(struct pg_ctx *c, int lane)
{
    memset(c, 0, sizeof *c);
    c->r = &r;
    c->shared = &sh;
    c->tool = &tool;
    c->cursor = &cursor;
    c->caps = &caps;
    c->lane = lane;
    c->fields = caps.fields;
    c->length = orig.length;
    c->ngrooves = 0;
    c->track_name = names;
}

/* where a row's value lives, and how many bytes */
static uint8_t *at(const struct pg_ctx *c, const struct pg_row *w, size_t *n)
{
    *n = w->kind == PK_RANGE ? 2 * sizeof(int16_t) : w->kind == PK_SEED ? sizeof(uint32_t) : sizeof(int16_t);
    switch (w->base) {
    case PB_RECIPE: return (uint8_t *)&r + w->off;
    case PB_LANE:   return (uint8_t *)&r.lane[c->lane] + w->off;
    case PB_SHARED: return (uint8_t *)&sh + w->off;
    default:        return 0;
    }
}

static void limits(const struct pg_ctx *c, const struct pg_row *w, int *lo, int *hi)
{
    *lo = w->lo;
    *hi = w->hi;
    if (w->fmt == PF_TRACK)
        *hi = caps.tracks - 1;
    if (w->fmt == PF_PARM)
        *hi = caps.nparams - 1;
    if (w->fmt == PF_GROOVE)
        *hi = c->ngrooves;
    if (w->flags & PR_PARM) {
        const struct seed_param *p = &caps.params[r.lane[c->lane].parm];
        *lo = p->lo;
        *hi = p->hi;
    }
}

/* a value the row can take, at random (different from the present one when it can be) */
static void set_random(const struct pg_ctx *c, const struct pg_row *w, int differ)
{
    size_t n;
    uint8_t *p = at(c, w, &n);
    int lo, hi, tries;
    uint8_t was[8];
    if (!p)
        return;
    memcpy(was, p, n);
    limits(c, w, &lo, &hi);
    for (tries = 0; tries < 20; tries++) {
        switch (w->kind) {
        case PK_SEED: *(uint32_t *)p = (uint32_t)prop_int(&g, 0, 9999); break;
        case PK_GRID: { static const int16_t gs[4] = { 1, 2, 4, 8 }; *(int16_t *)p = gs[prop_int(&g, 0, 3)]; break; }
        case PK_RANGE: {
            struct seed_values *v = (struct seed_values *)p;
            if ((w->flags & PR_DEF) && prop_chance(&g, 30)) {
                v->lo = v->hi = SEED_DEF;
            } else {
                int a = prop_int(&g, lo, hi), b = prop_int(&g, lo, hi);
                v->lo = (int16_t)(a < b ? a : b);
                v->hi = (int16_t)(a < b ? b : a);
            }
            break;
        }
        default: *(int16_t *)p = (int16_t)prop_int(&g, lo, hi); break;
        }
        /* PARM never reaches the other lane's parameter on the page (pages.c pg_turn) */
        if (w->fmt == PF_PARM && caps.lanes > 1 && *(int16_t *)p == r.lane[1 - c->lane].parm) {
            memcpy(p, was, n);              /* not that one; with no other value it stays as it was */
            continue;
        }
        if (!differ || memcmp(was, p, n))
            break;
    }
}

static void random_track(struct seed_track *t, int len)
{
    prop_track(t, &g, len, prop_int(&g, 0, 70));
    t->fields = caps.fields;
    for (int i = 0; i < SEED_STEPS; i++)
        t->s[i].lane[1] = prop_chance(&g, 80) ? SEED_NO_LOCK : (int16_t)prop_int(&g, 0, 127);
}

static void add_trigs(int track, const struct seed_track *t)
{
    ctx.length[track] = (uint8_t)t->length;
    memset(ctx.trig[track], 0, sizeof ctx.trig[track]);
    for (int s = 0; s < t->length; s++)
        if (t->s[s].flags & SEED_TRIG)
            ctx.trig[track][s >> 3] |= (uint8_t)(1u << (s & 7));
}

/* an original, the pattern around it, and a recipe of reachable values: each row's value at
 * random about half the time, else its default */
static void random_case(void)
{
    static const int lens[5] = { 13, 16, 24, 40, 64 };
    static const int olens[6] = { 16, 8, 12, 32, 64, 3 };
    struct pg_ctx c;
    int i, t, lane;
    random_track(&orig, lens[prop_int(&g, 0, 4)]);
    memset(&ctx, 0, sizeof ctx);
    ctx.caps = &caps;
    ctx.track = prop_int(&g, 0, NTRACKS - 2);
    ctx.tracks = NTRACKS;
    for (t = 0; t < NTRACKS; t++) {
        if (t == ctx.track) {
            add_trigs(t, &orig);
            continue;
        }
        random_track(&other, olens[prop_int(&g, 0, 5)]);
        add_trigs(t, &other);
    }
    seed_recipe_defaults(&r, &caps);
    memset(&sh, 0, sizeof sh);
    r.src = (int16_t)prop_int(&g, 0, SEED_SRC_N - 1);
    for (lane = 0; lane < 2; lane++) {
        page_ctx(&c, lane);
        for (i = 0; i < pg_nrows; i++) {
            const struct pg_row *w = &pg_rows[i];
            if (w->page > PG_TIME || w->base == PB_UI || w->base == PB_TOOL || (lane == 1 && w->base != PB_LANE)
                || (w->base == PB_RECIPE && w->off == offsetof(struct seed_recipe, src)))
                continue;
            if (prop_chance(&g, 50))
                set_random(&c, w, 0);
        }
    }
    ctx.shared = sh;
}

static int render(struct seed_track *o)
{
    ctx.shared = sh;
    return seed_render(o, &orig, &r, &ctx, &work);
}

/* 1 when the row's bytes overlap a row shown in either lane view (both write the same value) */
static int storage_shown(const struct pg_row *w, int lane)
{
    struct pg_ctx c, d;
    size_t n, m;
    uint8_t *p, *q;
    int i, l;
    page_ctx(&c, lane);
    p = at(&c, w, &n);
    for (l = 0; l < 2; l++) {
        page_ctx(&d, l);
        for (i = 0; i < pg_nrows; i++) {
            const struct pg_row *v = &pg_rows[i];
            if (v == w || !pg_row_shown(&d, v) || !(q = at(&d, v, &m)))
                continue;
            if (p < q + m && q < p + n)
                return 1;
        }
    }
    return 0;
}

/* the documented exception: SHAPE = MAP reads PART, X and Y with any source (the manual, RHYTHM), and
 * they are on RHYTHM, shown with SRC = MAP only */
static int map_shape_reads(const struct pg_row *w)
{
    if (r.shape != SEED_SHAPE_MAP || r.amt <= 0)
        return 0;
    return (w->base == PB_RECIPE && w->off == offsetof(struct seed_recipe, map_part))
        || (w->base == PB_SHARED && (w->off == offsetof(struct seed_shared, map_x) || w->off == offsetof(struct seed_shared, map_y)));
}

static int count(const struct seed_track *t)
{
    int i, n = 0;
    for (i = 0; i < t->length; i++)
        n += (t->s[i].flags & SEED_TRIG) != 0;
    return n;
}

static const char *what(void)
{
    static char b[200];
    snprintf(b, sizeof b, "src %d len %d seed %u vary %d phrs %d cvar %d qual %d rot %d sync %d mask %d/%d skip %d shape %d amt %d",
             r.src, orig.length, (unsigned)r.seed, r.vary, r.phrs, r.cvar, r.qual, r.rot, r.sync, r.mask, r.mtrk, r.skip,
             r.shape, r.amt);
    return b;
}

/* ---- 1 and 2 ---- */

static int hidden_tried, hidden_rows[256];

static void test_case(void)
{
    struct seed_recipe r0 = r;
    struct seed_shared s0 = sh;
    int rc = render(&base_out), i, lane, len = orig.length, n;

    CHECK(rc >= 0, "refused (%d): %s", rc, what());
    if (rc < 0)
        return;
    /* determinism (I2) */
    render(&out);
    CHECK(!memcmp(&out, &base_out, sizeof out), "not deterministic: %s", what());
    /* the track's frame and the steps past its length (I6) */
    CHECK(base_out.length == orig.length && base_out.fields == orig.fields, "frame changed: %s", what());
    for (i = len; i < SEED_STEPS; i++)
        CHECK(!memcmp(&base_out.s[i], &orig.s[i], sizeof orig.s[i]), "step %d past the length changed: %s", i, what());
    for (i = 0; i < len; i++) {
        const struct seed_step *s = &base_out.s[i];
        CHECK(s->origin >= SEED_NO_ORIGIN && s->origin < len, "step %d: origin %d: %s", i, s->origin, what());
        CHECK(!(i < r.skip && (s->flags & SEED_TRIG)), "SKIP %d: a trig on step %d: %s", r.skip, i, what());
        if (r.mask != SEED_MASK_OFF && (s->flags & SEED_TRIG)) {
            int m = ctx.length[r.mtrk], hit = m > 0 && seed_ctx_trig(&ctx, r.mtrk, i % m);
            CHECK(hit == (r.mask == SEED_MASK_ONLY), "MASK: a trig on step %d: %s", i, what());
        }
        if (s->flags & SEED_TRIG) {
            CHECK(s->vel == SEED_DEFAULT || (s->vel >= 1 && s->vel <= 127), "step %d: vel %d: %s", i, s->vel, what());
            CHECK(s->note == SEED_DEFAULT || s->note <= 127, "step %d: note %d: %s", i, s->note, what());
            CHECK(s->len == SEED_DEFAULT || s->len <= 127, "step %d: len %d: %s", i, s->len, what());
            CHECK(s->utime >= -23 && s->utime <= 23, "step %d: utime %d: %s", i, s->utime, what());
            CHECK(s->cond >= SEED_NO_COND && s->cond <= 64, "step %d: cond %d: %s", i, s->cond, what());
            CHECK(s->rrate <= 16, "step %d: rate %d: %s", i, s->rrate, what());
        }
    }
    /* counts through ROT and SYNC (I5): what the source places is what is written, when nothing
     * removes (MASK, SKIP) or adds (CVAR) trigs */
    if (r.mask == SEED_MASK_OFF && r.skip == 0 && r.cvar == SEED_CVAR_OFF) {
        n = count(&base_out);
        if (r.src == SEED_SRC_TRACK)
            CHECK(n == count(&orig), "TRACK: %d trigs of %d: %s", n, count(&orig), what());
        if (r.src == SEED_SRC_EUCLID) {
            int steps = r.eu_steps < 1 ? 1 : r.eu_steps, puls = r.eu_puls > steps ? steps : r.eu_puls, want = 0;
            for (i = 0; i < len; i++)
                want += i % steps * puls % steps < puls;
            CHECK(n == want, "EUCLID: %d trigs, %d pulses: %s", n, want, what());
        }
        if (r.src == SEED_SRC_RANDOM && (r.phrs == SEED_PHRS_OFF || len <= SEED_PAGE)) {
            int grid = r.grid;
            CHECK(n == seed_share(r.dens, (len + grid - 1) / grid), "RAND: %d trigs: %s", n, what());
        }
    }

    /* 1: every hidden row, turned to another value, leaves the take as it is */
    for (lane = 0; lane < 2; lane++) {
        struct pg_ctx c;
        page_ctx(&c, lane);
        for (i = 0; i < pg_nrows; i++) {
            const struct pg_row *w = &pg_rows[i];
            if (w->page > PG_TIME || w->base == PB_UI || w->base == PB_TOOL || (w->base != PB_LANE && lane))
                continue;
            if (pg_row_shown(&c, w) || storage_shown(w, lane) || map_shape_reads(w))
                continue;
            set_random(&c, w, 1);
            rc = render(&out);
            CHECK(rc >= 0 && !memcmp(&out, &base_out, sizeof out), "hidden %s %s (lane %d) changed the take: %s",
                  pg_names[w->page], w->label, lane, what());
            hidden_tried++;
            hidden_rows[i]++;
            r = r0;
            sh = s0;
        }
    }

    /* I3 with every source and PHRS, CVAR, QUAL: a shown knob of DYNAMICS .. TIME, turned, changes
     * only the fields its layer owns (no dressing moves a trig or changes QUAL's winner) */
    {
        static const struct { int page; enum seed_layer layer; } pl[5] = {
            { PG_DYN, SEED_L_DYN }, { PG_NOTE, SEED_L_NOTE }, { PG_LOCK, SEED_L_LOCK }, { PG_TRIG, SEED_L_TRIG },
            { PG_TIME, SEED_L_TIME }
        };
        int p;
        for (p = 0; p < 5; p++) {
            struct pg_ctx c;
            const struct pg_row *rows[8];
            int m = 0;
            unsigned d;
            page_ctx(&c, prop_int(&g, 0, 1));
            for (i = 0; i < 8; i++)
                if ((rows[m] = pg_row_at(&c, pl[p].page, i)) != 0)
                    m++;
            if (!m)
                continue;
            set_random(&c, rows[prop_int(&g, 0, m - 1)], 1);
            rc = render(&out);
            d = prop_diff(&out, &base_out, len);
            CHECK(rc >= 0 && !(d & ~prop_owned(pl[p].layer)), "%s: a turn changed fields 0x%x: %s", pg_names[pl[p].page], d,
                  what());
            r = r0;
            sh = s0;
        }
    }
}

static void test_cases(int n)
{
    int k, i, shown = 0;
    for (k = 0; k < n; k++) {
        random_case();
        test_case();
    }
    for (i = 0; i < pg_nrows; i++)
        shown += hidden_rows[i] > 0;
    printf("  %d recipes, %d hidden rows turned (%d rows were hidden at least once)\n", n, hidden_tried, shown);
}

/* ---- 3. PHRS keeps EUCLID's cycle along the track ---- */

static void test_phrs_euclid(void)
{
    static const int lens[4] = { 24, 40, 64, 64 };
    int k, i;
    for (k = 0; k < 400; k++) {
        struct seed_track a;
        random_case();
        random_track(&orig, lens[k % 4]);
        add_trigs(ctx.track, &orig);
        r.src = SEED_SRC_EUCLID;
        r.mask = SEED_MASK_OFF;
        r.cvar = SEED_CVAR_OFF;
        r.rot = r.sync = r.skip = 0;
        r.eu_steps = (int16_t)prop_int(&g, 1, 64);
        r.eu_puls = (int16_t)prop_int(&g, 0, r.eu_steps);
        r.phrs = SEED_PHRS_OFF;
        CHECK(render(&a) >= 0, "EUCLID, PHRS OFF refused");
        r.phrs = (int16_t)prop_int(&g, SEED_PHRS_AAAA, SEED_PHRS_N - 1);
        CHECK(render(&out) >= 0, "EUCLID, PHRS refused");
        for (i = 0; i < orig.length; i++) {
            CHECK((a.s[i].flags & SEED_TRIG) == (out.s[i].flags & SEED_TRIG), "EUCLID %d/%d, PHRS %d, %d steps: step %d",
                  r.eu_steps, r.eu_puls, r.phrs, orig.length, i);
            CHECK(out.s[i].origin == SEED_NO_ORIGIN, "EUCLID, PHRS: step %d has origin %d", i, out.s[i].origin);
        }
    }
    /* the letters still dress the pages: ABAB with a velocity range, pages 1 and 2 differ, 1 and 3
     * are dressed alike where both have a pulse on the same step */
    seed_recipe_defaults(&r, &caps);
    random_track(&orig, 64);
    add_trigs(ctx.track, &orig);
    r.src = SEED_SRC_EUCLID;
    r.eu_steps = 16;
    r.eu_puls = 7;
    r.vel.lo = 1;
    r.vel.hi = 127;
    r.phrs = SEED_PHRS_ABAB;
    CHECK(render(&out) >= 0, "EUCLID 16/7, PHRS ABAB refused");
    {
        int same13 = 1, same12 = 1;
        for (i = 0; i < 16; i++) {
            same13 &= out.s[i].vel == out.s[32 + i].vel;
            same12 &= out.s[i].vel == out.s[16 + i].vel;
        }
        CHECK(same13 && !same12, "EUCLID 16/7, PHRS ABAB: pages 1 and 3 alike (%d), 1 and 2 not (%d)", same13, !same12);
    }
}

/* ---- 4. the PR_SENSE rules ---- */

static const struct pg_row *find(const struct pg_ctx *c, int page, const char *label)
{
    int k;
    for (k = 0; k < 8; k++) {
        const struct pg_row *w = pg_row_at(c, page, k);
        if (w && !strcmp(w->label, label))
            return w;
    }
    return 0;
}

static void test_sense(void)
{
    struct pg_ctx c;
    int s;
    seed_recipe_defaults(&r, &caps);
    memset(&sh, 0, sizeof sh);
    orig.length = 16;
    page_ctx(&c, 0);
    CHECK(!find(&c, PG_TAKE, "PHRS"), "PHRS shown on 16 steps");
    c.length = 17;
    CHECK(find(&c, PG_TAKE, "PHRS") != 0, "PHRS not shown on 17 steps");
    c.length = 64;
    for (s = 0; s < SEED_SRC_N; s++) {
        int want = s == SEED_SRC_RANDOM;
        r.src = (int16_t)s;
        r.sync = 0;
        sh.map_chaos = 3;
        CHECK(!!find(&c, PG_TAKE, "QUAL") == want && !!find(&c, PG_TAKE, "CVAR") == want, "src %d: QUAL / CVAR", s);
        r.sync = 1;
        CHECK(find(&c, PG_TAKE, "QUAL") && find(&c, PG_TAKE, "CVAR"), "src %d, SYNC 1: QUAL / CVAR hidden", s);
        r.sync = 0;
        sh.map_chaos = 4;
        CHECK(!!find(&c, PG_TAKE, "QUAL") == (want || s == SEED_SRC_MAP), "src %d, CHAOS 4: QUAL", s);
    }
    r.src = SEED_SRC_RANDOM;
    r.bias = 0;
    CHECK(!find(&c, PG_RHYTHM, "ROLE"), "ROLE shown at BIAS 0");
    r.bias = -1;
    CHECK(find(&c, PG_RHYTHM, "ROLE") != 0, "ROLE hidden at BIAS -1");
    r.acc = 0;
    CHECK(!find(&c, PG_DYN, "AMOD"), "AMOD shown at ACC 0");
    r.acc = 1;
    CHECK(find(&c, PG_DYN, "AMOD") != 0, "AMOD hidden at ACC 1");
}

/* ---- 5. A:B's CVAL ---- */

static void test_cycle_text(void)
{
    struct pg_ctx c;
    const struct pg_row *w;
    char b[16];
    seed_recipe_defaults(&r, &caps);
    page_ctx(&c, 0);
    r.cond = 50;
    r.ctyp = SEED_CTYP_AB;
    w = find(&c, PG_TRIG, "CVAL");
    CHECK(w && w->kind == PK_INT, "TRIG CVAL for A:B");
    if (!w)
        return;
    pg_text(&c, w, b, 0);                     /* the default range 10..21: trig.c reads B 8 */
    CHECK(!strcmp(b, "/8"), "CVAL A:B with cval.lo 10: %s", b);
}

/* test_interplay [RECIPES [SEED]]: 3000 recipes by default (the Makefile's run) */
/* LOCK PARM never lands on the other lane's parameter (Alan, 2026-11-16): from the defaults, turning
 * either lane's PARM either way leaves the lanes on different parameters */
static void test_parm_skip(void)
{
    struct pg_ctx c;
    const struct pg_row *w;
    int lane, d, i;
    for (lane = 0; lane < 2; lane++)
        for (d = -1; d <= 1; d += 2) {
            seed_recipe_defaults(&r, &caps);
            page_ctx(&c, lane);
            w = pg_row_at(&c, PG_LOCK, 0);
            CHECK(w && w->fmt == PF_PARM, "LOCK's knob A is PARM");
            if (!w || w->fmt != PF_PARM)
                continue;
            for (i = 0; i < 3; i++) {
                pg_turn(&c, w, d, 0);
                CHECK(r.lane[0].parm != r.lane[1].parm, "PARM lane %d turned %d: both lanes on %d", lane + 1, d,
                      r.lane[0].parm);
            }
        }
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 3000;
    if (argc > 2)
        g.s = (uint32_t)strtoul(argv[2], 0, 0);
    test_parm_skip();
    test_cases(n > 0 ? n : 3000);
    test_phrs_euclid();
    test_sense();
    test_cycle_text();
    printf("test_interplay: %d checks, %d failed\n", checks, failed);
    return failed != 0;
}
