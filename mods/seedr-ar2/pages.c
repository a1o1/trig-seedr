/* SPDX-License-Identifier: GPL-2.0-or-later */
/* pages: every page's knobs (docs/design-v1.md section 5) and what a turn does to them. Owner:
 * task 0.3, which created every row; a workstream edits only its own rows (docs/plan-v1.md
 * section 4). The order of pages is LEFT / RIGHT's. Freestanding: no C library. */
#include <stddef.h>
#include "pages.h"

const char *const pg_names[PG_N] = {
    "TAKE", "RHYTHM", "PLACE", "DYNAMICS", "NOTE", "LOCK", "TRIG", "TIME", "VIEW", "TOOLS"
};

/* ---- choices ---- */

static const struct pg_choice c_src[SEED_SRC_N] = {
    { "TRACK", 0 }, { "RAND", &seed_have_random }, { "EUCLID", &seed_have_euclid },
    { "MAP", &seed_have_map }, { "COPY", &seed_have_copy }
};
static const struct pg_choice c_role[SEED_ROLE_N] = {
    { "METER", 0 }, { "KICK", 0 }, { "BACK", 0 }, { "HAT", 0 }, { "PERC", 0 }
};
static const struct pg_choice c_metr[SEED_METR_N] = {
    { "4/4", 0 }, { "3/4", 0 }, { "6/8", 0 }, { "12/16", 0 }, { "5/8", 0 }, { "7/8", 0 }
};
static const struct pg_choice c_phrs[SEED_PHRS_N] = {
    { "OFF", 0 }, { "AAAA", 0 }, { "AAAB", 0 }, { "ABAB", 0 }, { "ABAC", 0 }, { "ABCD", 0 }
};
static const struct pg_choice c_cvar[SEED_CVAR_N] = { { "OFF", 0 }, { "1:2", 0 }, { "AAAB", 0 } };
static const struct pg_choice c_mask[SEED_MASK_N] = { { "OFF", 0 }, { "AVOID", 0 }, { "ONLY", 0 } };
/* SEED_FLD_ALL is ROTATE ALL's (TOOLS), not FLD's */
static const struct pg_choice c_fld[SEED_FLD_LANE2 + 1] = {
    { "OFF", 0 }, { "VEL", 0 }, { "NOTE", 0 }, { "LEN", 0 }, { "UTIM", 0 }, { "COND", 0 },
    { "RTRG", 0 }, { "ACC", 0 }, { "LANE1", 0 }, { "LANE2", 0 }
};
static const struct pg_choice c_rmode[SEED_RMODE_N] = { { "TRIG", 0 }, { "GRID", 0 } };
static const struct pg_choice c_shape[SEED_SHAPE_N] = {
    { "FLAT", 0 }, { "STRONG", 0 }, { "WEAK", 0 }, { "OFF", 0 }, { "NOT 1", 0 }, { "NOTBAR", 0 },
    { "RANK", &seed_have_rank }, { "NOISE", &seed_have_noise }, { "EVRY", 0 }, { "MAP", &seed_have_map }
};
static const struct pg_choice c_amod[SEED_AMOD_N] = { { "RAND", 0 }, { "TOP", &seed_have_rank } };
static const struct pg_choice c_lmod[SEED_LMOD_N] = { { "RANGE", 0 }, { "LEGATO", &seed_have_list } };
static const struct pg_choice c_vsrc[SEED_VSRC_N] = {
    { "RANGE", 0 }, { "LIST", &seed_have_list }, { "NOISE", &seed_have_noise }
};
static const struct pg_choice c_order[SEED_ORDR_N] = {
    { "UP", 0 }, { "DOWN", 0 }, { "PING", 0 }, { "PNG2", 0 }, { "ODD", 0 }, { "RAND", 0 }
};
static const struct pg_choice c_ctyp[SEED_CTYP_N] = {
    { "X%", 0 }, { "A:B", 0 }, { "1ST", 0 }, { "FILL", 0 }, { "MIX", 0 }
};
static const struct pg_choice c_tool[SEED_TOOL_N] = { { "ROT ALL", 0 } };

/* ---- the rows ---- */

#define REC(f)  PB_RECIPE, (uint16_t)offsetof(struct seed_recipe, f)
#define LANE(f) PB_LANE, (uint16_t)offsetof(struct seed_lane, f)
#define SHR(f)  PB_SHARED, (uint16_t)offsetof(struct seed_shared, f)
#define TOOL(f) PB_TOOL, (uint16_t)offsetof(struct seed_tool, f)
#define WHEN(f, mask)  (int16_t)offsetof(struct seed_recipe, f), (mask)
#define LWHEN(f, mask) (int16_t)offsetof(struct seed_lane, f), (mask)
#define ALWAYS -1, PG_ANY
#define NOT(v) (0xffffffffu & ~PG_V(v))

const struct pg_row pg_rows[] = {
    /* TAKE (lead; PHRS, CVAR: F9; METR: F1; BEAT: 0.2; QUAL: F11) */
    { PG_TAKE, 0, "SEED", REC(seed), PK_SEED, PF_NUM, 0, 9999, 0, 0, 0, 0, ALWAYS, 0 },
    { PG_TAKE, 1, "VARY", REC(vary), PK_INT, PF_PCT, 0, 100, 0, 0, 0, 0, ALWAYS, 0 },
    /* PR_SENSE (M2.1): PHRS on a track longer than a page; CVAR and QUAL while the trig positions
     * draw random numbers (sense() below) */
    { PG_TAKE, 2, "PHRS", REC(phrs), PK_CHOICE, PF_NUM, 0, SEED_PHRS_N - 1, 0, PR_SENSE, &seed_have_vary, 0, ALWAYS, c_phrs },
    { PG_TAKE, 3, "CVAR", REC(cvar), PK_CHOICE, PF_NUM, 0, SEED_CVAR_N - 1, 0, PR_SENSE, &seed_have_vary, 0, ALWAYS, c_cvar },
    { PG_TAKE, 4, "METR", REC(metr), PK_CHOICE, PF_NUM, 0, SEED_METR_N - 1, 0, 0, &seed_have_rank, 0, ALWAYS, c_metr },
    { PG_TAKE, 5, "BEAT", REC(beat), PK_INT, PF_NUM, 1, 16, 0, 0, &seed_have_random, 0, ALWAYS, 0 },
    { PG_TAKE, 6, "QUAL", REC(qual), PK_INT, PF_NUM, 1, 8, 0, PR_SENSE, &seed_have_score, 0, ALWAYS, 0 },

    /* RHYTHM: SRC, then the source's own knobs */
    { PG_RHYTHM, 0, "SRC", REC(src), PK_CHOICE, PF_NUM, 0, SEED_SRC_N - 1, 0, PR_MULTI, 0, 0, ALWAYS, c_src },
    /* RANDOM (0.2; ROLE: F1). ROLE picks the rank table BIAS mixes in: shown while BIAS is not 0 (M2.1) */
    { PG_RHYTHM, 1, "GRID", REC(grid), PK_GRID, PF_GRID, 1, 8, 0, 0, &seed_have_random, 0, WHEN(src, PG_V(SEED_SRC_RANDOM)), 0 },
    { PG_RHYTHM, 2, "DENS", REC(dens), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_random, 0, WHEN(src, PG_V(SEED_SRC_RANDOM)), 0 },
    { PG_RHYTHM, 3, "BIAS", REC(bias), PK_INT, PF_SPCT, -100, 100, 0, 0, &seed_have_random, 0, WHEN(src, PG_V(SEED_SRC_RANDOM)), 0 },
    { PG_RHYTHM, 4, "ROLE", REC(role), PK_CHOICE, PF_NUM, 0, SEED_ROLE_N - 1, 0, PR_SENSE, &seed_have_rank, 0, WHEN(src, PG_V(SEED_SRC_RANDOM)), c_role },
    /* EUCLID (F2) */
    { PG_RHYTHM, 1, "STEPS", REC(eu_steps), PK_INT, PF_NUM, 1, 64, 0, 0, &seed_have_euclid, 0, WHEN(src, PG_V(SEED_SRC_EUCLID)), 0 },
    { PG_RHYTHM, 2, "PULS", REC(eu_puls), PK_INT, PF_NUM, 0, 64, 0, 0, &seed_have_euclid, 0, WHEN(src, PG_V(SEED_SRC_EUCLID)), 0 },
    /* MAP (F3): PART and DENS per track, X, Y and CHAOS the pattern's (A1) */
    { PG_RHYTHM, 1, "PART", REC(map_part), PK_INT, PF_PLUS1, 0, 2, 0, 0, &seed_have_map, 0, WHEN(src, PG_V(SEED_SRC_MAP)), 0 },
    { PG_RHYTHM, 2, "DENS", REC(map_dens), PK_INT, PF_NUM, 0, 255, 0, 0, &seed_have_map, 0, WHEN(src, PG_V(SEED_SRC_MAP)), 0 },
    { PG_RHYTHM, 3, "X", SHR(map_x), PK_INT, PF_NUM, 0, 255, 0, 0, &seed_have_map, 0, WHEN(src, PG_V(SEED_SRC_MAP)), 0 },
    { PG_RHYTHM, 4, "Y", SHR(map_y), PK_INT, PF_NUM, 0, 255, 0, 0, &seed_have_map, 0, WHEN(src, PG_V(SEED_SRC_MAP)), 0 },
    { PG_RHYTHM, 5, "CHAOS", SHR(map_chaos), PK_INT, PF_NUM, 0, 255, 0, 0, &seed_have_map, 0, WHEN(src, PG_V(SEED_SRC_MAP)), 0 },
    /* COPY (F4) */
    { PG_RHYTHM, 1, "TRK", REC(copy_trk), PK_INT, PF_TRACK, 0, SEED_TRACKS - 1, 0, 0, &seed_have_copy, 0, WHEN(src, PG_V(SEED_SRC_COPY)), 0 },

    /* PLACE (ROT, SKIP, FLD..FMOD: 0.2; SYNC: F5; MASK, MTRK: F4) */
    { PG_PLACE, 0, "ROT", REC(rot), PK_INT, PF_SIGNED, -63, 63, 0, 0, &seed_have_place, 0, ALWAYS, 0 },
    { PG_PLACE, 1, "SYNC", REC(sync), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_sync, 0, ALWAYS, 0 },
    { PG_PLACE, 2, "MASK", REC(mask), PK_CHOICE, PF_NUM, 0, SEED_MASK_N - 1, 0, 0, &seed_have_mask, 0, ALWAYS, c_mask },
    { PG_PLACE, 3, "MTRK", REC(mtrk), PK_INT, PF_TRACK, 0, SEED_TRACKS - 1, 0, 0, &seed_have_mask, 0, WHEN(mask, NOT(SEED_MASK_OFF)), 0 },
    { PG_PLACE, 4, "SKIP", REC(skip), PK_INT, PF_NUM, 0, 64, 0, 0, &seed_have_place, 0, ALWAYS, 0 },
    { PG_PLACE, 5, "FLD", REC(fld), PK_CHOICE, PF_NUM, 0, SEED_FLD_LANE2, 0, 0, &seed_have_place, 0, ALWAYS, c_fld },
    { PG_PLACE, 6, "FROT", REC(frot), PK_INT, PF_SIGNED, -63, 63, 0, 0, &seed_have_place, 0, WHEN(fld, NOT(SEED_FLD_OFF)), 0 },
    { PG_PLACE, 7, "FMOD", REC(fmod), PK_CHOICE, PF_NUM, 0, SEED_RMODE_N - 1, 0, 0, &seed_have_place, 0, WHEN(fld, NOT(SEED_FLD_OFF)), c_rmode },

    /* DYNAMICS (0.2; SHAPE = RANK, AMOD = TOP: F1; SHAPE = NOISE: F7; SHAPE = MAP: F3) */
    { PG_DYN, 0, "VEL", REC(vel), PK_RANGE, PF_NUM, 1, 127, 100, PR_DEF, &seed_have_dyn, SEED_F_VEL, ALWAYS, 0 },
    { PG_DYN, 1, "SHAPE", REC(shape), PK_CHOICE, PF_NUM, 0, SEED_SHAPE_N - 1, 0, 0, &seed_have_dyn, SEED_F_VEL, ALWAYS, c_shape },
    { PG_DYN, 2, "AMT", REC(amt), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_dyn, SEED_F_VEL, WHEN(shape, NOT(SEED_SHAPE_FLAT)), 0 },
    { PG_DYN, 3, "N", REC(n), PK_INT, PF_NUM, 1, 64, 0, 0, &seed_have_dyn, SEED_F_VEL,
      WHEN(shape, PG_V(SEED_SHAPE_EVRY) | PG_V(SEED_SHAPE_NOISE)), 0 },
    { PG_DYN, 4, "ACC", REC(acc), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_dyn, SEED_F_ACCENT, ALWAYS, 0 },
    /* AMOD: shown while ACC is above 0 (M2.1) */
    { PG_DYN, 5, "AMOD", REC(amod), PK_CHOICE, PF_NUM, 0, SEED_AMOD_N - 1, 0, PR_MULTI | PR_SENSE, &seed_have_dyn, SEED_F_ACCENT, ALWAYS, c_amod },
    { PG_DYN, 6, "HUM", REC(hum), PK_INT, PF_NUM, 0, 100, 0, 0, &seed_have_dyn, SEED_F_VEL, ALWAYS, 0 },

    /* NOTE (0.2; LMOD = LEGATO, LIST: F6; NOISE: F7) */
    { PG_NOTE, 0, "LEN", REC(len), PK_RANGE, PF_LEN, 0, 127, 14, PR_DEF, &seed_have_values, SEED_F_LEN, ALWAYS, 0 },
    { PG_NOTE, 1, "LMOD", REC(lmod), PK_CHOICE, PF_NUM, 0, SEED_LMOD_N - 1, 0, PR_MULTI, &seed_have_values, SEED_F_LEN, ALWAYS, c_lmod },
    { PG_NOTE, 2, "NOTE", REC(note), PK_RANGE, PF_SIGNED, -24, 24, 0, PR_DEF, &seed_have_values, SEED_F_NOTE, ALWAYS, 0 },
    { PG_NOTE, 3, "NSRC", REC(note.src), PK_CHOICE, PF_NUM, 0, SEED_VSRC_N - 1, 0, PR_MULTI, &seed_have_values, SEED_F_NOTE, ALWAYS, c_vsrc },
    { PG_NOTE, 4, "ORDR", REC(note.order), PK_CHOICE, PF_NUM, 0, SEED_ORDR_N - 1, 0, 0, &seed_have_list, SEED_F_NOTE,
      WHEN(note.src, PG_V(SEED_VSRC_LIST)), c_order },
    { PG_NOTE, 5, "NLEN", REC(note.len), PK_INT, PF_NUM, 1, SEED_LIST_MAX, 0, 0, &seed_have_list, SEED_F_NOTE,
      WHEN(note.src, PG_V(SEED_VSRC_LIST)), 0 },
    { PG_NOTE, 6, "ZOOM", REC(note.zoom), PK_INT, PF_NUM, 1, 64, 0, 0, &seed_have_noise, SEED_F_NOTE,
      WHEN(note.src, PG_V(SEED_VSRC_NOISE)), 0 },

    /* LOCK, the lane in view (UP / DOWN) (0.2; LIST: F6; NOISE: F7) */
    { PG_LOCK, 0, "PARM", LANE(parm), PK_INT, PF_PARM, 0, 0, 0, PR_LANES, &seed_have_values, SEED_F_LOCKS, ALWAYS, 0 },
    { PG_LOCK, 1, "%", LANE(pct), PK_INT, PF_PCT, 0, 100, 0, PR_LANES, &seed_have_values, SEED_F_LOCKS, ALWAYS, 0 },
    { PG_LOCK, 2, "RNGE", LANE(val), PK_RANGE, PF_NUM, 0, 0, 0, PR_LANES | PR_PARM, &seed_have_values, SEED_F_LOCKS, ALWAYS, 0 },
    { PG_LOCK, 3, "SRC", LANE(val.src), PK_CHOICE, PF_NUM, 0, SEED_VSRC_N - 1, 0, PR_LANES | PR_MULTI, &seed_have_values,
      SEED_F_LOCKS, ALWAYS, c_vsrc },
    { PG_LOCK, 4, "ORDR", LANE(val.order), PK_CHOICE, PF_NUM, 0, SEED_ORDR_N - 1, 0, PR_LANES, &seed_have_list, SEED_F_LOCKS,
      LWHEN(val.src, PG_V(SEED_VSRC_LIST)), c_order },
    { PG_LOCK, 5, "LLEN", LANE(val.len), PK_INT, PF_NUM, 1, SEED_LIST_MAX, 0, PR_LANES, &seed_have_list, SEED_F_LOCKS,
      LWHEN(val.src, PG_V(SEED_VSRC_LIST)), 0 },
    { PG_LOCK, 6, "ZOOM", LANE(val.zoom), PK_INT, PF_NUM, 1, 64, 0, PR_LANES, &seed_have_noise, SEED_F_LOCKS,
      LWHEN(val.src, PG_V(SEED_VSRC_NOISE)), 0 },

    /* TRIG (0.2) */
    { PG_TRIG, 0, "COND", REC(cond), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_trig, SEED_F_COND, ALWAYS, 0 },
    { PG_TRIG, 1, "CTYP", REC(ctyp), PK_CHOICE, PF_NUM, 0, SEED_CTYP_N - 1, 0, 0, &seed_have_trig, SEED_F_COND, ALWAYS, c_ctyp },
    { PG_TRIG, 2, "CVAL", REC(cval), PK_RANGE, PF_PROB, 0, 21, 0, 0, &seed_have_trig, SEED_F_COND,
      WHEN(ctyp, PG_V(SEED_CTYP_PROB) | PG_V(SEED_CTYP_MIX)), 0 },
    { PG_TRIG, 2, "CVAL", REC(cval.lo), PK_INT, PF_CYCLE, 2, 8, 0, 0, &seed_have_trig, SEED_F_COND, WHEN(ctyp, PG_V(SEED_CTYP_AB)), 0 },
    { PG_TRIG, 3, "RTRG", REC(rtrg), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_trig, SEED_F_RETRIG, ALWAYS, 0 },
    { PG_TRIG, 4, "RATE", REC(rate), PK_RANGE, PF_RATE, 0, 16, 0, 0, &seed_have_trig, SEED_F_RETRIG, ALWAYS, 0 },
    { PG_TRIG, 5, "RCUR", REC(rcur), PK_INT, PF_SIGNED, -128, 127, 0, 0, &seed_have_trig, SEED_F_RETRIG, ALWAYS, 0 },

    /* TIME (0.2; DRFT: F8) */
    { PG_TIME, 0, "GRV", REC(grv), PK_INT, PF_GROOVE, 0, 0, 0, 0, &seed_have_timing, 0, ALWAYS, 0 },
    { PG_TIME, 1, "TIME", REC(timing), PK_INT, PF_SPCT, -100, 100, 0, 0, &seed_have_timing, SEED_F_UTIME, ALWAYS, 0 },
    { PG_TIME, 2, "GVEL", REC(gvel), PK_INT, PF_SPCT, -100, 100, 0, 0, &seed_have_timing, SEED_F_VEL, ALWAYS, 0 },
    { PG_TIME, 3, "QUANT", REC(quant), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_timing, SEED_F_UTIME, ALWAYS, 0 },
    { PG_TIME, 4, "RAND", REC(rand), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_timing, SEED_F_UTIME, ALWAYS, 0 },
    { PG_TIME, 5, "DRFT", REC(drft), PK_INT, PF_PCT, 0, 100, 0, 0, &seed_have_drift, SEED_F_UTIME, ALWAYS, 0 },
    { PG_TIME, 6, "SHFT", REC(shft), PK_INT, PF_SIGNED, -63, 63, 0, 0, &seed_have_timing, 0, ALWAYS, 0 },

    /* VIEW (0.3; F10 owns what it draws) */
    { PG_VIEW, 0, "STEP", PB_UI, 0, PK_CURSOR, PF_STEP, 0, 0, 0, PR_KEEP, 0, 0, ALWAYS, 0 },

    /* TOOLS: its own YES (V7) */
    { PG_TOOLS, 0, "TOOL", TOOL(tool), PK_CHOICE, PF_NUM, 0, SEED_TOOL_N - 1, 0, PR_KEEP, 0, 0, ALWAYS, c_tool },
    { PG_TOOLS, 1, "STEPS", TOOL(steps), PK_INT, PF_SIGNED, -63, 63, 0, PR_KEEP, 0, 0, ALWAYS, 0 },
};
const int pg_nrows = (int)(sizeof pg_rows / sizeof pg_rows[0]);

/* ---- which rows show ---- */

int pg_built(const uint8_t *have)
{
#ifdef SEEDR_SHOW_ALL
    (void)have;
    return 1;
#else
    return !have || *have;
#endif
}

static int16_t *lane_base(const struct pg_ctx *c)
{
    return (int16_t *)&c->r->lane[c->lane];
}

/* the row's int16_t value (every base but PK_SEED's and PB_UI's) */
static int16_t *at(const struct pg_ctx *c, const struct pg_row *w)
{
    uint8_t *b;
    switch (w->base) {
    case PB_LANE:   b = (uint8_t *)lane_base(c); break;
    case PB_SHARED: b = (uint8_t *)c->shared; break;
    case PB_TOOL:   b = (uint8_t *)c->tool; break;
    default:        b = (uint8_t *)c->r; break;
    }
    return (int16_t *)(b + w->off);
}

static int choices_built(const struct pg_row *w)
{
    int i, n = 0;
    for (i = w->lo; i <= w->hi; i++)
        n += pg_built(w->names[i].have);
    return n;
}

/* PR_SENSE (task M2.1, docs/status/M2.1.md): a row that could not change the take is not shown.
 * Each rule holds exactly when the engine ignores the value, so a hidden value is harmless (the
 * host test mods/seedr-ar2/test_interplay.c renders it both ways):
 *   PHRS        the track is longer than a page (seed_render: a page or less is one phrase)
 *   CVAR, QUAL  the trig positions draw random numbers: SRC = RAND, MAP with CHAOS 4 or more
 *               (seed_map_lift), or SYNC above 0. Otherwise takes n and n + 1, and every
 *               candidate, have the same trigs, and CVAR and QUAL work only on where trigs are.
 *   ROLE        BIAS is not 0 (the rank table is only read through BIAS)
 *   AMOD        ACC is above 0 */
static int sense(const struct pg_ctx *c, const struct pg_row *w)
{
    const struct seed_recipe *r = c->r;
    if (w->base != PB_RECIPE)
        return 1;
    switch (w->off) {
    case offsetof(struct seed_recipe, phrs):
        return c->length > SEED_PAGE;
    case offsetof(struct seed_recipe, cvar):
    case offsetof(struct seed_recipe, qual):
        return r->src == SEED_SRC_RANDOM || (r->src == SEED_SRC_MAP && c->shared && c->shared->map_chaos >= 4)
            || r->sync > 0;
    case offsetof(struct seed_recipe, role):
        return r->bias != 0;
    case offsetof(struct seed_recipe, amod):
        return r->acc > 0;
    default:
        return 1;
    }
}

int pg_row_shown(const struct pg_ctx *c, const struct pg_row *w)
{
    if (!pg_built(w->have) || (c->fields & w->field) != w->field)
        return 0;
    if ((w->flags & PR_SENSE) && !sense(c, w))
        return 0;
    if ((w->flags & PR_LANES) && (!c->caps || c->lane >= c->caps->lanes || !c->caps->nparams))
        return 0;
    if (w->when_off >= 0) {
        const uint8_t *b = w->base == PB_LANE ? (const uint8_t *)lane_base(c) : (const uint8_t *)c->r;
        int v = *(const int16_t *)(b + w->when_off);
        if (v < 0 || v > 31 || !(w->when_mask & PG_V(v)))
            return 0;
    }
    if ((w->flags & PR_MULTI) && choices_built(w) < 2)
        return 0;
    return 1;
}

const struct pg_row *pg_row_at(const struct pg_ctx *c, int page, int knob)
{
    int i;
    for (i = 0; i < pg_nrows; i++)
        if (pg_rows[i].page == page && pg_rows[i].knob == knob && pg_row_shown(c, &pg_rows[i]))
            return &pg_rows[i];
    return 0;
}

int pg_page_shown(const struct pg_ctx *c, int page)
{
    int i;
    for (i = 0; i < pg_nrows; i++)
        if (pg_rows[i].page == page && pg_row_shown(c, &pg_rows[i]))
            return 1;
    return 0;
}

int pg_page_step(const struct pg_ctx *c, int page, int d)
{
    int i, p = page;
    for (i = 0; i < PG_N; i++) {
        p = (p + (d < 0 ? PG_N - 1 : 1)) % PG_N;
        if (pg_page_shown(c, p))
            return p;
    }
    return page;
}

int pg_src_built(int src)
{
    return src >= 0 && src < SEED_SRC_N && pg_built(c_src[src].have);
}

/* ---- turns ---- */

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* the row's limits now: some come from the device, the track or the lane's parameter */
static void limits(const struct pg_ctx *c, const struct pg_row *w, int *lo, int *hi)
{
    *lo = w->lo;
    *hi = w->hi;
    switch (w->fmt) {
    case PF_TRACK: *hi = (c->caps ? c->caps->tracks : SEED_TRACKS) - 1; break;
    case PF_PARM:  *hi = (c->caps && c->caps->nparams ? c->caps->nparams : 1) - 1; break;
    case PF_GROOVE: *hi = c->ngrooves; break;
    case PF_STEP:  *hi = (c->length > 0 ? c->length : 1) - 1; break;
    default: break;
    }
    if ((w->flags & PR_PARM) && c->caps && c->caps->nparams) {
        const struct seed_param *p = &c->caps->params[clampi(c->r->lane[c->lane].parm, 0, c->caps->nparams - 1)];
        *lo = p->lo;
        *hi = p->hi;
    }
}

static int turn_range(struct seed_values *v, int d, int lo_end, int lo, int hi, int start, int def)
{
    int16_t *a = lo_end ? &v->lo : &v->hi, was;
    if (v->lo == SEED_DEF) {
        if (d < 0)
            return 0;
        v->lo = v->hi = (int16_t)clampi(start, lo, hi);
        return 1;
    }
    if (def && *a + d < lo) {
        v->lo = v->hi = SEED_DEF;
        return 1;
    }
    was = *a;
    *a = (int16_t)clampi(*a + d, lo, hi);
    if (v->lo > v->hi) {                      /* the other end follows */
        if (lo_end)
            v->hi = v->lo;
        else
            v->lo = v->hi;
    }
    return *a != was;
}

int pg_turn(const struct pg_ctx *c, const struct pg_row *w, int d, int lo_end)
{
    int lo, hi, v, was;
    int16_t *a;
    limits(c, w, &lo, &hi);
    switch (w->kind) {
    case PK_SEED: {
        uint32_t *s = (uint32_t *)((uint8_t *)c->r + w->off);
        was = (int)*s;
        *s = (uint32_t)clampi(was + d * (lo_end ? 100 : 1), lo, hi);
        return (int)*s != was;
    }
    case PK_CURSOR:
        was = *c->cursor;
        *c->cursor = clampi(was + d, lo, hi);
        return *c->cursor != was;
    case PK_RANGE:
        return turn_range((struct seed_values *)at(c, w), d, lo_end, lo, hi, w->start, w->flags & PR_DEF);
    case PK_GRID: {
        static const int16_t gs[4] = { 1, 2, 4, 8 };
        int i;
        a = at(c, w);
        i = *a >= 8 ? 3 : *a >= 4 ? 2 : *a >= 2 ? 1 : 0;
        was = *a;
        *a = gs[clampi(i + d, 0, 3)];
        return *a != was;
    }
    case PK_CHOICE:
        a = at(c, w);
        for (v = *a + d; v >= lo && v <= hi; v += d)
            if (pg_built(w->names[v].have)) {
                *a = (int16_t)v;
                return 1;
            }
        return 0;
    default:
        a = at(c, w);
        was = *a;
        v = clampi(*a + d * (lo_end && hi - lo > 20 ? 10 : 1), lo, hi);
        /* PARM never lands on the other lane's parameter: the device keeps one lock per parameter
         * and step, so two lanes on one parameter cannot both be stored (M2.2; Alan, 2026-11-16).
         * The engine also refuses such a recipe (values.c). */
        if (w->fmt == PF_PARM && c->caps && c->caps->lanes > 1 && v == c->r->lane[1 - c->lane].parm)
            v = v + d >= lo && v + d <= hi ? v + d : was;
        *a = (int16_t)v;
        return *a != was;
    }
}

/* ---- text ---- */

char *pg_put(char *o, const char *s)
{
    while (*s)
        *o++ = *s++;
    return o;
}

char *pg_num(char *o, int v)
{
    char t[12];
    int n = 0;
    unsigned u;
    if (v < 0) {
        *o++ = '-';
        u = (unsigned)-v;
    } else
        u = (unsigned)v;
    do t[n++] = (char)('0' + u % 10); while ((u /= 10) && n < 12);
    while (n)
        *o++ = t[--n];
    return o;
}

char *pg_snum(char *o, int v)
{
    if (v > 0)
        *o++ = '+';
    return pg_num(o, v);
}

/* a LEN code as stock's stepped scale has it (libanalogrytm pattern.h): in 1/16 of a step, then
 * as a note value where it is one (14 = 1/16, 30 = 1/8 ... 126 = 8/1), else in steps (1.25) */
static unsigned len_units(int c)
{
    static const uint8_t lo[8] = { 0, 14, 30, 46, 62, 78, 94, 110 };
    static const uint16_t base[8] = { 2, 16, 32, 64, 128, 256, 512, 1024 };
    int i = 7;
    while (i > 0 && c < lo[i])
        i--;
    return base[i] + (unsigned)(c - lo[i]) * (i ? 1u << (i - 1) : 1u);
}

char *pg_len(char *o, int c)
{
    static const char *const names[11] = {
        "1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "8/1"
    };
    unsigned u, k;
    if (c >= 127)
        return pg_put(o, "INF");
    u = len_units(c < 0 ? 0 : c);
    for (k = 0; k < 11; k++)
        if (u == 2u << k)
            return pg_put(o, names[k]);
    o = pg_num(o, (int)(u / 16));             /* steps, two decimals */
    k = ((u % 16) * 100 + 8) / 16;            /* rounded: 15/16 = .94 */
    *o++ = '.';
    *o++ = (char)('0' + k / 10);
    *o++ = (char)('0' + k % 10);
    return o;
}

static const uint8_t prob[22] = {
    1, 3, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99, 100
};

char *pg_cond(char *o, int c)
{
    static const char *const special[8] = { "FILL", "!FIL", "PRE", "!PRE", "NEI", "!NEI", "1ST", "!1ST" };
    int b, base;
    if (c < 0)
        return pg_put(o, "-");
    if (c <= 21) {
        o = pg_num(o, prob[c]);
        return pg_put(o, "%");
    }
    if (c <= 29)
        return pg_put(o, special[c - 22]);
    for (b = 8; b >= 2; b--) {
        base = 29 + b * (b - 1) / 2;
        if (c >= base && c < base + b) {
            o = pg_num(o, c - base + 1);
            *o++ = ':';
            return pg_num(o, b);
        }
    }
    return pg_put(o, "?");
}

char *pg_rate(char *o, int c)
{
    static const char *const rates[17] = {
        "1/1", "1/2", "1/3", "1/4", "1/5", "1/6", "1/8", "1/10", "1/12", "1/16", "1/20", "1/24",
        "1/32", "1/40", "1/48", "1/64", "1/80"
    };
    return pg_put(o, rates[clampi(c, 0, 16)]);
}

/* one value in the row's format */
static char *one(const struct pg_ctx *c, const struct pg_row *w, char *o, int v)
{
    switch (w->fmt) {
    case PF_SIGNED: return pg_snum(o, v);
    case PF_PCT:    o = pg_num(o, v); return pg_put(o, "%");
    case PF_SPCT:   o = pg_snum(o, v); return pg_put(o, "%");
    case PF_PLUS1:  return pg_num(o, v + 1);
    case PF_LEN:    return pg_len(o, v);
    case PF_PROB:   return pg_num(o, prob[clampi(v, 0, 21)]);
    case PF_CYCLE:  *o++ = '/'; return pg_num(o, clampi(v, 2, 8));   /* as trig.c reads it (M2.1) */
    case PF_RATE:   return pg_rate(o, v);
    case PF_GRID:   return pg_put(o, v >= 8 ? "1/2" : v >= 4 ? "1/4" : v >= 2 ? "1/8" : "1/16");
    case PF_TRACK:  return c->track_name ? pg_put(o, c->track_name(v)) : pg_num(o, v + 1);
    case PF_PARM:
        if (c->caps && v >= 0 && v < c->caps->nparams)
            return pg_put(o, c->caps->params[v].name);
        return pg_put(o, "-");
    case PF_GROOVE: return v <= 0 ? pg_put(o, "OFF") : pg_num(o, v);
    default:        return pg_num(o, v);
    }
}

char *pg_text(const struct pg_ctx *c, const struct pg_row *w, char *o, int part)
{
    switch (w->kind) {
    case PK_SEED:
        o = pg_num(o, (int)*(const uint32_t *)((const uint8_t *)c->r + w->off));
        break;
    case PK_CURSOR:
        *o++ = 'S';
        o = pg_num(o, *c->cursor + 1);
        *o++ = '/';
        o = pg_num(o, c->length);
        break;
    case PK_CHOICE: {
        int lo, hi, v = *at(c, w);
        limits(c, w, &lo, &hi);
        o = pg_put(o, v >= lo && v <= hi ? w->names[v].name : "?");
        break;
    }
    case PK_RANGE: {
        const struct seed_values *v = (const struct seed_values *)at(c, w);
        if (v->lo == SEED_DEF) {
            o = pg_put(o, "DEF");
            break;
        }
        o = one(c, w, o, v->lo);
        if (v->hi == v->lo)
            break;
        if (part == 1) {
            *o++ = '+';
            break;
        }
        if (w->fmt == PF_SIGNED) {            /* signed ends carry their own sign: -5+5, -9-2, -3+0 */
            if (v->hi >= 0)
                *o++ = '+';
            o = pg_num(o, v->hi);
        } else if (w->fmt == PF_RATE) {
            char t[8], *e = pg_rate(t, v->hi), *s = t + 2;   /* 1/16-32 */
            *o++ = '-';
            while (s < e)
                *o++ = *s++;
        } else {
            *o++ = '-';
            o = one(c, w, o, v->hi);
        }
        break;
    }
    default:
        o = one(c, w, o, *at(c, w));
        break;
    }
    *o = 0;
    return o;
}
