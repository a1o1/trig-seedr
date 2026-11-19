/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for the page tables (pages.c): which rows and pages show, what turns do, the value
 * text. Built twice by the Makefile: with the library's seed_have_* flags as they are, and with
 * SEEDR_SHOW_ALL (every feature shown, as the screenshot build); and with PAGES_DEVICE_ST on the Syntakt's
 * capabilities (device/st_params.h: track 1's list with SP TWINSHOT, no retrig, accent or slide; task P1). */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "pages.h"

static int checks, failed;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failed++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static struct seed_recipe r;
static struct seed_shared sh;
static struct seed_tool tool;
static int cursor;
#ifndef PAGES_DEVICE_ST
static const struct seed_param params[2] = { { "SMPL", 19, 0, 127 }, { "TUNE", 8, -24, 24 } };
static struct seed_caps caps = {
    13, 64, 0x1ff, -23, 23, 2, 2, params, { 0, 1 }
};
static const char *names(int t)
{
    static const char *const n[13] = { "BD", "SD", "RS", "CP", "BT", "LT", "MT", "HT", "CH", "OH", "CY", "CB", "FX" };
    return t >= 0 && t < 13 ? n[t] : "?";
}
#else
#include "../device/st_params.h"
static struct seed_param params[ST_TRACK_PARAMS_MAX];
static struct seed_caps caps = {
    ST_TRACKS, 64, ST_VOICE_FIELDS, ST_UTIME_LO, ST_UTIME_HI, SEED_LANES, 0, params, { 0, 0 }
};
__attribute__((constructor)) static void st_caps(void)
{
    static const struct st_machine m[ST_NMACHINES] = ST_MACHINES;
    uint16_t ids[ST_TRACK_PARAMS_MAX];
    caps.nparams = (uint8_t)st_track_params(st_machine_of(m, ST_NMACHINES, 44), 0, params, ids, caps.lane_parm);
}
static const char *names(int t)
{
    static const char *const n[13] = { "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "T9", "T10", "T11", "T12", "FX" };
    return t >= 0 && t < 13 ? n[t] : "?";
}
#endif

static void ctx(struct pg_ctx *c)
{
    memset(c, 0, sizeof *c);
    c->r = &r;
    c->shared = &sh;
    c->tool = &tool;
    c->cursor = &cursor;
    c->caps = &caps;
    c->fields = caps.fields;
    c->length = 16;
    c->ngrooves = 5;
    c->track_name = names;
}

static void fresh(void)
{
    seed_recipe_defaults(&r, &caps);
    memset(&sh, 0, sizeof sh);
    tool.tool = SEED_TOOL_ROTATE_ALL;
    tool.steps = 1;
    cursor = 0;
}

static const char *text(const struct pg_ctx *c, const struct pg_row *w, int part)
{
    static char b[64];
    pg_text(c, w, b, part);
    return b;
}

/* the recipe fields a condition reads, with how many values to try */
static const struct { int16_t off; int n; int lane; } conds[] = {
    { offsetof(struct seed_recipe, src), SEED_SRC_N, 0 },
    { offsetof(struct seed_recipe, mask), SEED_MASK_N, 0 },
    { offsetof(struct seed_recipe, fld), SEED_FLD_N, 0 },
    { offsetof(struct seed_recipe, shape), SEED_SHAPE_N, 0 },
    { offsetof(struct seed_recipe, note.src), SEED_VSRC_N, 0 },
    { offsetof(struct seed_recipe, ctyp), SEED_CTYP_N, 0 },
    { offsetof(struct seed_lane, val.src), SEED_VSRC_N, 1 },
};

/* at most one row shows on a knob, whatever the conditions */
static void test_one_row_a_knob(void)
{
    struct pg_ctx c;
    int k, v, p, knob, i, n;
    ctx(&c);
    for (k = 0; k < (int)(sizeof conds / sizeof conds[0]); k++)
        for (v = 0; v < conds[k].n; v++) {
            int16_t *f;
            fresh();
            f = conds[k].lane ? (int16_t *)((uint8_t *)&r.lane[0] + conds[k].off) : (int16_t *)((uint8_t *)&r + conds[k].off);
            *f = (int16_t)v;
            for (p = 0; p < PG_N; p++)
                for (knob = 0; knob < 8; knob++) {
                    for (i = n = 0; i < pg_nrows; i++)
                        n += pg_rows[i].page == p && pg_rows[i].knob == knob && pg_row_shown(&c, &pg_rows[i]);
                    CHECK(n <= 1, "%s knob %c: %d rows show (condition %d = %d)", pg_names[p], 'A' + knob, n, k, v);
                }
        }
}

/* every row: label length, limits, and turning it far both ways stays inside them and touches only
 * its own value */
static void test_turns(void)
{
    struct pg_ctx c;
    int i, d, j, lane;
    ctx(&c);
    for (lane = 0; lane < 2; lane++)
        for (i = 0; i < pg_nrows; i++) {
            const struct pg_row *w = &pg_rows[i];
            struct seed_recipe before;
            struct seed_shared sh0;
            struct seed_tool t0;
            size_t lo_b, hi_b;
            if (w->base != PB_LANE && lane)
                continue;
            CHECK(strlen(w->label) <= 5, "%s: label longer than 5", w->label);
            CHECK(w->knob < 8 && w->page < PG_N, "%s: knob or page out of range", w->label);
            CHECK(w->kind != PK_CHOICE || w->names, "%s: a choice without names", w->label);
            fresh();
            c.lane = lane;
            /* the condition's value, so the row is reachable */
            before = r; sh0 = sh; t0 = tool;
            for (d = -1; d <= 1; d += 2)
                for (j = 0; j < 300; j++) {
                    pg_turn(&c, w, d, j & 1);
                    pg_text(&c, w, (char[64]){ 0 }, 0);
                }
            /* the bytes outside this row's value are unchanged */
            switch (w->base) {
            case PB_SHARED:
                CHECK(memcmp(&r, &before, sizeof r) == 0 && memcmp(&tool, &t0, sizeof tool) == 0, "%s: touched another struct", w->label);
                break;
            case PB_TOOL:
                CHECK(memcmp(&r, &before, sizeof r) == 0 && memcmp(&sh, &sh0, sizeof sh) == 0, "%s: touched another struct", w->label);
                break;
            case PB_UI:
                CHECK(memcmp(&r, &before, sizeof r) == 0, "%s: touched the recipe", w->label);
                CHECK(cursor >= 0 && cursor < c.length, "%s: cursor %d", w->label, cursor);
                break;
            default:
                lo_b = (w->base == PB_LANE ? offsetof(struct seed_recipe, lane) + lane * sizeof(struct seed_lane) : 0) + w->off;
                hi_b = lo_b + (w->kind == PK_RANGE ? 2 * sizeof(int16_t) : w->kind == PK_SEED ? sizeof(uint32_t) : sizeof(int16_t));
                CHECK(memcmp(&r, &before, lo_b) == 0 && memcmp((uint8_t *)&r + hi_b, (uint8_t *)&before + hi_b, sizeof r - hi_b) == 0,
                      "%s (lane %d): touched bytes outside its value", w->label, lane);
                CHECK(memcmp(&sh, &sh0, sizeof sh) == 0 && memcmp(&tool, &t0, sizeof tool) == 0, "%s: touched another struct", w->label);
                break;
            }
            /* inside the limits */
            if (w->kind == PK_INT || w->kind == PK_CHOICE || w->kind == PK_GRID) {
                int16_t v = *(int16_t *)((w->base == PB_LANE ? (uint8_t *)&r.lane[lane] : w->base == PB_SHARED ? (uint8_t *)&sh :
                                          w->base == PB_TOOL ? (uint8_t *)&tool : (uint8_t *)&r) + w->off);
                int hi = w->fmt == PF_TRACK ? caps.tracks - 1 : w->fmt == PF_PARM ? caps.nparams - 1 : w->fmt == PF_GROOVE ? c.ngrooves : w->hi;
                CHECK(v >= w->lo && v <= hi, "%s: %d outside %d..%d", w->label, v, w->lo, hi);
            }
            if (w->kind == PK_SEED)
                CHECK(r.seed <= 9999, "SEED %u", (unsigned)r.seed);
        }
}

static const struct pg_row *row(const struct pg_ctx *c, int page, const char *label)
{
    int k;
    for (k = 0; k < 8; k++) {
        const struct pg_row *w = pg_row_at(c, page, k);
        if (w && !strcmp(w->label, label))
            return w;
    }
    return 0;
}

static void test_ranges(void)
{
    struct pg_ctx c;
    const struct pg_row *w;
    ctx(&c);
    fresh();
    w = row(&c, PG_DYN, "VEL");
#ifdef SEEDR_SHOW_ALL
    CHECK(w != 0, "DYNAMICS VEL not shown");
#endif
    if (!w)
        return;
    CHECK(!strcmp(text(&c, w, 0), "DEF"), "VEL starts at DEF: %s", text(&c, w, 0));
    pg_turn(&c, w, -1, 0);
    CHECK(r.vel.lo == SEED_DEF, "VEL left of DEF stays DEF");
    pg_turn(&c, w, 1, 0);
    CHECK(r.vel.lo == 100 && r.vel.hi == 100, "VEL leaves DEF at 100: %d %d", r.vel.lo, r.vel.hi);
    pg_turn(&c, w, 1, 0);
    CHECK(r.vel.lo == 100 && r.vel.hi == 101 && !strcmp(text(&c, w, 0), "100-101"), "VEL hi: %s", text(&c, w, 0));
    CHECK(!strcmp(text(&c, w, 1), "100+"), "VEL short: %s", text(&c, w, 1));
    pg_turn(&c, w, 1, 1);
    pg_turn(&c, w, 1, 1);
    CHECK(r.vel.lo == 101 && r.vel.hi == 101, "FUNC: the low end stops at the high end: %d %d", r.vel.lo, r.vel.hi);
    /* each end only with its own control, never pushing the other (M2.5) */
    r.vel.lo = 40; r.vel.hi = 90;
    pg_turn(&c, w, -1, 0);
    CHECK(r.vel.lo == 40 && r.vel.hi == 89, "the knob moves only the high end: %d %d", r.vel.lo, r.vel.hi);
    pg_turn(&c, w, -1, 1);
    CHECK(r.vel.lo == 39 && r.vel.hi == 89, "FUNC moves only the low end: %d %d", r.vel.lo, r.vel.hi);
    r.vel.lo = r.vel.hi = 60;
    pg_turn(&c, w, -1, 0);
    CHECK(r.vel.lo == 60 && r.vel.hi == 60, "the high end stops at the low end: %d %d", r.vel.lo, r.vel.hi);
    r.vel.lo = 90; r.vel.hi = 40;                       /* stored the other way round */
    pg_turn(&c, w, 1, 0);
    CHECK(r.vel.lo == 40 && r.vel.hi == 91, "either way round: %d %d", r.vel.lo, r.vel.hi);
    r.vel.lo = r.vel.hi = 1;
    pg_turn(&c, w, -1, 1);
    CHECK(r.vel.lo == SEED_DEF && r.vel.hi == SEED_DEF, "below 1: DEF");

    w = row(&c, PG_NOTE, "NOTE");
    pg_turn(&c, w, 1, 0);
    r.note.lo = -5; r.note.hi = 5;
    CHECK(!strcmp(text(&c, w, 0), "-5+5"), "NOTE -5+5: %s", text(&c, w, 0));
    r.note.lo = -9; r.note.hi = -2;
    CHECK(!strcmp(text(&c, w, 0), "-9-2"), "NOTE -9-2: %s", text(&c, w, 0));
    r.note.lo = -3; r.note.hi = 0;
    CHECK(!strcmp(text(&c, w, 0), "-3+0"), "NOTE -3+0: %s", text(&c, w, 0));

    w = row(&c, PG_NOTE, "LEN");
    pg_turn(&c, w, 1, 0);
    CHECK(!strcmp(text(&c, w, 0), "1/16"), "LEN leaves DEF at 1/16: %s", text(&c, w, 0));
    r.len.hi = 30;
    CHECK(!strcmp(text(&c, w, 0), "1/16-1/8") && !strcmp(text(&c, w, 1), "1/16+"), "LEN range: %s", text(&c, w, 0));
    r.len.lo = r.len.hi = 15;
    CHECK(!strcmp(text(&c, w, 0), "1.06"), "LEN 15: %s", text(&c, w, 0));
    r.len.lo = r.len.hi = 127;
    CHECK(!strcmp(text(&c, w, 0), "INF"), "LEN 127: %s", text(&c, w, 0));

    /* RATE and RCUR only while RTRG is above 0, CTYP and CVAL while COND is, TIME, GVEL and SHFT
     * with a groove (0.7: knobs that cannot change the take are hidden) */
    CHECK(!row(&c, PG_TRIG, "RATE") && !row(&c, PG_TRIG, "RCUR") && !row(&c, PG_TRIG, "CTYP") &&
          !row(&c, PG_TRIG, "CVAL") && row(&c, PG_TRIG, "COND") &&
          !row(&c, PG_TRIG, "RTRG") == !(caps.fields & SEED_F_RETRIG), "TRIG at 0: only COND, RTRG (where the device has it)");
    CHECK(!row(&c, PG_TIME, "TIME") && !row(&c, PG_TIME, "GVEL") && !row(&c, PG_TIME, "SHFT") &&
          row(&c, PG_TIME, "QUANT"), "TIME without a groove: no TIME, GVEL, SHFT");
    r.grv = 1;
    CHECK(row(&c, PG_TIME, "TIME") && row(&c, PG_TIME, "SHFT"), "TIME with a groove");
    r.grv = 0;
    r.rtrg = 50;
    r.cond = 50;
    w = row(&c, PG_TRIG, "RATE");
    r.rate.lo = 9; r.rate.hi = 12;
    if (caps.fields & SEED_F_RETRIG)
        CHECK(w && !strcmp(text(&c, w, 0), "1/16-32"), "RATE: %s", w ? text(&c, w, 0) : "hidden");
    else
        CHECK(!w, "RATE shows on a device without retrigs, RTRG at 50");
    w = row(&c, PG_TRIG, "CVAL");
    CHECK(!strcmp(text(&c, w, 0), "50-100"), "CVAL X%%: %s", text(&c, w, 0));
    r.ctyp = SEED_CTYP_AB;
    w = row(&c, PG_TRIG, "CVAL");
    r.cval.lo = 0;
    pg_turn(&c, w, 1, 0);
    CHECK(r.cval.lo == 2 && !strcmp(text(&c, w, 0), "/2"), "CVAL A:B: %d %s", r.cval.lo, text(&c, w, 0));
    r.ctyp = SEED_CTYP_1ST;
    CHECK(!row(&c, PG_TRIG, "CVAL"), "CVAL hidden for 1ST");
    r.ctyp = SEED_CTYP_PROB;
    r.rtrg = r.cond = 0;

    /* LOCK: the range from the lane's parameter */
    c.lane = 1;
    w = row(&c, PG_LOCK, "RNGE");
    r.lane[1].parm = 1;
    r.lane[1].val.lo = r.lane[1].val.hi = 0;
    {
        int j;
        for (j = 0; j < 100; j++)
            pg_turn(&c, w, -1, 1);
    }
    CHECK(r.lane[1].val.lo == caps.params[1].lo, "lane 2 %s low end %d: %d", caps.params[1].name, caps.params[1].lo,
          r.lane[1].val.lo);
    CHECK(r.lane[0].val.lo == 1, "lane 1 untouched: %d", r.lane[0].val.lo);
    c.lane = 0;
}

static void test_choices(void)
{
    struct pg_ctx c;
    const struct pg_row *w;
    ctx(&c);
    fresh();
    w = row(&c, PG_RHYTHM, "SRC");
#ifdef SEEDR_SHOW_ALL
    CHECK(w != 0, "SRC shown");
    pg_turn(&c, w, 1, 0);
    CHECK(r.src == SEED_SRC_RANDOM && !strcmp(text(&c, w, 0), "RAND"), "SRC -> RAND");
    CHECK(row(&c, PG_RHYTHM, "DENS") && row(&c, PG_RHYTHM, "GRID"), "RANDOM's knobs show");
    pg_turn(&c, w, 1, 0);
    CHECK(r.src == SEED_SRC_EUCLID && row(&c, PG_RHYTHM, "PULS") && !row(&c, PG_RHYTHM, "GRID"), "EUCLID's knobs replace RANDOM's");
    pg_turn(&c, w, 1, 0);
    pg_turn(&c, w, 1, 0);
    pg_turn(&c, w, 1, 0);
    CHECK(r.src == SEED_SRC_COPY, "SRC stops at the last value: %d", r.src);
    w = row(&c, PG_RHYTHM, "TRK");
    CHECK(!strcmp(text(&c, w, 0), "BD"), "COPY TRK reads a track name: %s", text(&c, w, 0));
    {
        int j;
        for (j = 0; j < 40; j++)
            pg_turn(&c, w, 1, 0);
    }
    CHECK(r.copy_trk == 12, "TRK stops at the device's last track: %d", r.copy_trk);
#else
    /* the library's flags as they are */
    if (!seed_have_random)
        CHECK(w == 0, "SRC hidden while TRACK is the only source built");
#endif
    fresh();
    r.grid = 1;
    w = pg_row_at(&c, PG_RHYTHM, 1);
    r.src = SEED_SRC_RANDOM;
    w = row(&c, PG_RHYTHM, "GRID");
    if (w) {
        pg_turn(&c, w, 1, 0);
        CHECK(r.grid == 2 && !strcmp(text(&c, w, 0), "1/8"), "GRID 1/16 -> 1/8");
        pg_turn(&c, w, 1, 0);
        pg_turn(&c, w, 1, 0);
        pg_turn(&c, w, 1, 0);
        CHECK(r.grid == 8, "GRID stops at 1/2");
    }
    w = row(&c, PG_TAKE, "SEED");
    CHECK(w != 0, "SEED always shown");
    r.seed = 5;
    pg_turn(&c, w, 1, 1);
    CHECK(r.seed == 105, "FUNC + SEED: 100 a step: %u", (unsigned)r.seed);
    pg_turn(&c, w, -1, 1);
    pg_turn(&c, w, -1, 1);
    CHECK(r.seed == 0, "SEED stops at 0: %u", (unsigned)r.seed);
}

/* which pages show */
static void test_pages(void)
{
    struct pg_ctx c;
    int p, shown = 0;
    ctx(&c);
    fresh();
    for (p = 0; p < PG_N; p++)
        shown += pg_page_shown(&c, p);
    CHECK(pg_page_shown(&c, PG_TAKE) && pg_page_shown(&c, PG_VIEW) && pg_page_shown(&c, PG_TOOLS), "TAKE, VIEW, TOOLS always");
#ifdef SEEDR_SHOW_ALL
    CHECK(shown == PG_N, "every page shows: %d of %d", shown, PG_N);
    /* LEFT / RIGHT visit every page and wrap */
    for (p = 0; p < PG_N; p++)
        CHECK(pg_page_step(&c, p, 1) == (p + 1) % PG_N && pg_page_step(&c, p, -1) == (p + PG_N - 1) % PG_N, "LEFT / RIGHT at %d", p);
    /* a device without lanes: no LOCK page */
    caps.lanes = 0;
    CHECK(!pg_page_shown(&c, PG_LOCK), "no lanes: no LOCK page");
    caps.lanes = 2;
    c.lane = 1;
    CHECK(pg_page_shown(&c, PG_LOCK), "lane 2");
    caps.lanes = 1;
    CHECK(!pg_page_shown(&c, PG_LOCK), "lane 2 on a one-lane device");
    caps.lanes = 2;
    c.lane = 0;
    /* a track without retrigs or accents: those knobs hide */
    c.fields = (uint16_t)(caps.fields & ~(SEED_F_RETRIG | SEED_F_ACCENT));
    CHECK(!row(&c, PG_TRIG, "RTRG") && !row(&c, PG_DYN, "ACC") && row(&c, PG_TRIG, "COND"), "fields hide knobs");
    c.fields = caps.fields;
#else
    printf("pages shown with the library as built (%d): ", shown);
    for (p = 0; p < PG_N; p++)
        if (pg_page_shown(&c, p))
            printf("%s ", pg_names[p]);
    printf("\n");
    /* with no feature built, LEFT / RIGHT go between what there is and skip the rest */
    for (p = 0; p < PG_N; p++)
        if (pg_page_shown(&c, p))
            CHECK(pg_page_shown(&c, pg_page_step(&c, p, 1)) && pg_page_shown(&c, pg_page_step(&c, p, -1)), "steps from %s land on shown pages", pg_names[p]);
#endif
}

/* every knob of design-v1.md section 5 has a row: the recipe's settings, the shared ones, TOOLS */
static void test_reachable(void)
{
#define F(f) { offsetof(struct seed_recipe, f), #f }
    static const struct { size_t off; const char *name; } need[] = {
        F(seed), F(vary), F(phrs), F(cvar), F(metr), F(beat), F(qual),
        F(src), F(grid), F(dens), F(bias), F(role), F(eu_steps), F(eu_puls), F(map_part), F(map_dens), F(copy_trk),
        F(rot), F(sync), F(mask), F(mtrk), F(skip), F(fld), F(frot), F(fmod),
        F(vel), F(shape), F(amt), F(n), F(acc), F(amod), F(hum),
        F(len), F(lmod), F(note), F(note.src), F(note.order), F(note.len), F(note.zoom),
        F(cond), F(ctyp), F(cval), F(rtrg), F(rate), F(rcur),
        F(grv), F(timing), F(gvel), F(quant), F(rand), F(drft), F(shft),
    };
    static const struct { size_t off; const char *name; } lane[] = {
        { offsetof(struct seed_lane, parm), "parm" }, { offsetof(struct seed_lane, pct), "pct" },
        { offsetof(struct seed_lane, val), "val" }, { offsetof(struct seed_lane, val.src), "val.src" },
        { offsetof(struct seed_lane, val.order), "val.order" }, { offsetof(struct seed_lane, val.len), "val.len" },
        { offsetof(struct seed_lane, val.zoom), "val.zoom" },
    };
    size_t k;
    int i, found;
    for (k = 0; k < sizeof need / sizeof need[0]; k++) {
        for (i = found = 0; i < pg_nrows; i++)
            found |= pg_rows[i].base == PB_RECIPE && pg_rows[i].off == need[k].off;
        CHECK(found, "no row for %s", need[k].name);
    }
    for (k = 0; k < sizeof lane / sizeof lane[0]; k++) {
        for (i = found = 0; i < pg_nrows; i++)
            found |= pg_rows[i].base == PB_LANE && pg_rows[i].off == lane[k].off;
        CHECK(found, "no row for lane %s", lane[k].name);
    }
    for (i = found = 0; i < pg_nrows; i++)
        found += pg_rows[i].base == PB_SHARED;
    CHECK(found == 3, "MAP X, Y, CHAOS: %d rows", found);
    for (i = found = 0; i < pg_nrows; i++)
        found += pg_rows[i].base == PB_TOOL;
    CHECK(found == 4, "TOOLS: TOOL, STEPS, WILD, ROLL: %d rows", found);
    /* STEPS with ROT ALL, WILD and ROLL with YOLO (0.7) */
    {
        struct pg_ctx c;
        ctx(&c);
        tool.tool = SEED_TOOL_ROTATE_ALL;
        CHECK(row(&c, PG_TOOLS, "STEPS") && !row(&c, PG_TOOLS, "WILD") && !row(&c, PG_TOOLS, "ROLL"), "ROT ALL: STEPS only");
        tool.tool = SEED_TOOL_YOLO;
        CHECK(!row(&c, PG_TOOLS, "STEPS") && row(&c, PG_TOOLS, "WILD") && row(&c, PG_TOOLS, "ROLL"), "YOLO: WILD, ROLL");
        tool.tool = SEED_TOOL_ROTATE_ALL;
    }
#undef F
}

static void test_text(void)
{
    char b[16];
    *pg_cond(b, 10) = 0;
    CHECK(!strcmp(b, "50%"), "cond 10: %s", b);
    *pg_cond(b, 30) = 0;
    CHECK(!strcmp(b, "1:2"), "cond 30: %s", b);
    *pg_cond(b, 64) = 0;
    CHECK(!strcmp(b, "8:8"), "cond 64: %s", b);
    *pg_cond(b, 28) = 0;
    CHECK(!strcmp(b, "1ST"), "cond 28: %s", b);
    *pg_len(b, 126) = 0;
    CHECK(!strcmp(b, "8/1"), "len 126: %s", b);
    *pg_snum(b, 0) = 0;
    CHECK(!strcmp(b, "0"), "snum 0: %s", b);
    *pg_num(b, -32768) = 0;
    CHECK(!strcmp(b, "-32768"), "num -32768: %s", b);
}

/* FLD offers only the fields the track has: a Syntakt track has no retrig and no accent, so a turn
 * never lands on RTRG or ACC there (S5's finding); a track with every field gets every choice */
static void test_fld_fields(void)
{
    struct pg_ctx c;
    const struct pg_row *w;
    int i, seen[SEED_FLD_LANE2 + 1];
    ctx(&c);
    fresh();
    c.fields = SEED_F_NOTE | SEED_F_VEL | SEED_F_LEN | SEED_F_UTIME | SEED_F_COND | SEED_F_LOCKS;
    w = row(&c, PG_PLACE, "FLD");
    CHECK(w != 0, "PLACE FLD not shown");
    if (!w)
        return;
    memset(seen, 0, sizeof seen);
    for (i = 0; i < 20; i++) {
        pg_turn(&c, w, 1, 0);
        seen[r.fld] = 1;
    }
    CHECK(!seen[SEED_FLD_RTRG] && !seen[SEED_FLD_ACC], "FLD offers RTRG or ACC without the fields");
    CHECK(seen[SEED_FLD_COND] && seen[SEED_FLD_LANE2], "FLD skips the fields the track has");
    for (i = 0; i < 20; i++)
        pg_turn(&c, w, -1, 0);
    CHECK(r.fld == SEED_FLD_OFF, "FLD back to OFF: %d", r.fld);
    c.fields = (uint16_t)~0u;
    memset(seen, 0, sizeof seen);
    for (i = 0; i < 20; i++) {
        pg_turn(&c, w, 1, 0);
        seen[r.fld] = 1;
    }
    CHECK(seen[SEED_FLD_RTRG] && seen[SEED_FLD_ACC], "FLD hides RTRG or ACC on a track that has them");
}

#ifdef PAGES_DEVICE_ST
/* the Syntakt (task P1): no row that needs retrig, accent or slide shows, on any page, whatever the recipe's
 * other values; LOCK's PARM turns only through the track's list, both lanes */
static void test_st_fields(void)
{
    struct pg_ctx c;
    int i, l, p, k;
    ctx(&c);
    fresh();
    CHECK(caps.nparams > 8 && caps.nparams <= ST_TRACK_PARAMS_MAX, "Syntakt track 1: %d parameters", caps.nparams);
    for (i = 0; i < pg_nrows; i++) {
        const struct pg_row *w = &pg_rows[i];
        if (w->field & (SEED_F_RETRIG | SEED_F_ACCENT | SEED_F_SLIDE))
            CHECK(!pg_row_shown(&c, w), "%s shows on the Syntakt", w->label);
    }
    for (p = 0; p < PG_N; p++)
        for (k = 0; k < 8; k++) {
            const struct pg_row *w = pg_row_at(&c, p, k);
            CHECK(!w || !(w->field & (SEED_F_RETRIG | SEED_F_ACCENT | SEED_F_SLIDE)), "%s %c: %s needs a field the Syntakt lacks",
                  pg_names[p], 'A' + k, w ? w->label : "");
        }
    for (l = 0; l < SEED_LANES; l++) {
        const struct pg_row *w = 0;
        c.lane = l;
        for (k = 0; k < 8 && !w; k++) {
            const struct pg_row *x = pg_row_at(&c, PG_LOCK, k);
            if (x && x->fmt == PF_PARM)
                w = x;
        }
        CHECK(w != 0, "LOCK lane %d: no PARM", l + 1);
        if (!w)
            continue;
        for (i = 0; i < 2 * ST_TRACK_PARAMS_MAX; i++) {
            pg_turn(&c, w, i < ST_TRACK_PARAMS_MAX ? 1 : -1, 0);
            CHECK(r.lane[l].parm >= 0 && r.lane[l].parm < caps.nparams, "LOCK lane %d PARM %d outside the list", l + 1, r.lane[l].parm);
        }
    }
}
#endif

int main(void)
{
    test_one_row_a_knob();
    test_turns();
    test_ranges();
    test_choices();
    test_pages();
    test_reachable();
    test_text();
    test_fld_fields();
#ifdef PAGES_DEVICE_ST
    test_st_fields();
    printf("test_pages (Syntakt): %d checks, %d failed\n", checks, failed);
    return failed != 0;
#endif
#ifdef SEEDR_SHOW_ALL
    printf("test_pages (all shown): %d checks, %d failed\n", checks, failed);
#else
    printf("test_pages: %d checks, %d failed\n", checks, failed);
#endif
    return failed != 0;
}
