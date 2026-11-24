/* SPDX-License-Identifier: GPL-2.0-or-later */
/* pages: the page framework's tables (docs/design-v1.md section 5). Every page's knobs are rows
 * of one table, pg_rows in pages.c; frame.c draws them and passes turns through pg_turn. Owner:
 * task 0.3. A workstream edits only its own rows (docs/plan-v1.md section 4).
 *
 * A row is shown when its feature is built (`have`, a seed_have_* flag), the device and track have
 * its field (`field`, SEED_F_*), and its condition holds (`when`: another setting's value is one
 * of a set, as RHYTHM's knobs by SRC). A page is shown when one of its rows is. Device-neutral,
 * freestanding: frame.c draws, this file only computes values and text.
 */
#ifndef SEEDR_PAGES_H
#define SEEDR_PAGES_H

#include <stdint.h>
#include "../lib/seed.h"

enum pg_page { PG_TAKE, PG_RHYTHM, PG_PLACE, PG_DYN, PG_NOTE, PG_LOCK, PG_TRIG, PG_TIME, PG_VIEW, PG_TOOLS,
               PG_TURING,             /* the live machine (0.9, design-turing.md TD1): the ring's end */
               PG_N };

/* where a row's value lives */
enum pg_base {
    PB_RECIPE,                        /* the track's struct seed_recipe */
    PB_LANE,                          /* the recipe's lane in view (LOCK, UP / DOWN): lane[0] + lane */
    PB_SHARED,                        /* struct seed_shared, the pattern's (MAP X, Y, CHAOS) */
    PB_TOOL,                          /* struct seed_tool (TOOLS) */
    PB_UI                             /* the mod's own: the VIEW cursor (an int) */
};

/* what a turn does */
enum pg_kind {
    PK_INT,                           /* an int16_t, lo..hi; FUNC + knob: 10 a step */
    PK_CHOICE,                        /* an int16_t, an index into `names`; unbuilt values are skipped */
    PK_RANGE,                         /* a struct seed_values' lo / hi; FUNC + knob: the low end */
    PK_SEED,                          /* the uint32_t seed, lo..hi; FUNC + knob: 100 a step */
    PK_GRID,                          /* an int16_t, 1, 2, 4 or 8 */
    PK_CURSOR,                        /* PB_UI's int, 0 .. the track's length - 1 */
    PK_SCALE                          /* NOTE's SCALE (an index into `names`) and the int16_t after it, ROOT 0..11:
                                       * the knob turns SCALE; FUNC + knob turns ROOT, while SCALE is not OFF */
};

/* how a value reads */
enum pg_fmt {
    PF_NUM, PF_SIGNED,                /* 5, +5 */
    PF_PCT, PF_SPCT,                  /* 5%, +5% */
    PF_PLUS1,                         /* the value + 1 (MAP PART 1..3) */
    PF_LEN,                           /* a length code: 1/16, 1.25, INF */
    PF_PROB,                          /* a probability code 0..21 as stock's percent */
    PF_CYCLE,                         /* A:B's B: /2 .. /8 */
    PF_RATE,                          /* a retrig rate code: 1/16 */
    PF_GRID,                          /* 1/16, 1/8, 1/4, 1/2 */
    PF_TRACK,                         /* a track name, from the device */
    PF_PARM,                          /* a lane's parameter, from the capabilities */
    PF_GROOVE,                        /* OFF, or the groove's number */
    PF_STEP                           /* the VIEW cursor: S5/16 */
};

/* row flags */
#define PR_DEF   0x01                 /* PK_RANGE: below the low end is DEF (the field is left, V4) */
#define PR_MULTI 0x02                 /* PK_CHOICE: hidden unless two or more values are built */
#define PR_PARM  0x04                 /* PK_RANGE: lo..hi from the lane's parameter (struct seed_param) */
#define PR_LANES 0x08                 /* shown only when the device has the lane in view (caps->lanes) */
#define PR_KEEP  0x10                 /* not part of the take: a turn does not count for YES's next seed */
#define PR_SENSE 0x20                 /* shown only while it can change the take: a rule in pages.c that
                                         reads other settings or the track's length (task M2.1) */
#define PR_LIVE  0x40                 /* shown only when the device can run TURING live (pg_ctx.live, L8) */

#define PG_ANY 0                      /* `when_mask` 0: always */
#define PG_V(v) (1u << (v))           /* `when_mask` bit for value v */

/* a choice's text and the flag that must be set for it to be offered (NULL: always) */
struct pg_choice {
    const char *name;
    const uint8_t *have;
    uint16_t need;                    /* SEED_F_* the track must have for the choice to show; 0: none */
};

struct pg_row {
    uint8_t page;                     /* enum pg_page */
    uint8_t knob;                     /* 0..7 = A..H */
    const char *label;                /* at most 5 characters */
    uint8_t base;                     /* enum pg_base */
    uint16_t off;                     /* the value's offset in its base */
    uint8_t kind;                     /* enum pg_kind */
    uint8_t fmt;                      /* enum pg_fmt */
    int16_t lo, hi;                   /* the value's limits; PK_RANGE with PR_DEF: lo..hi */
    int16_t start;                    /* PK_RANGE with PR_DEF: lo = hi = start when it leaves DEF */
    uint8_t flags;                    /* PR_* */
    const uint8_t *have;              /* a seed_have_* flag, or NULL: always built */
    uint16_t field;                   /* SEED_F_* the track must have, or 0 */
    int16_t when_off;                 /* offset of the int16_t the condition reads, or -1: in the lane
                                         in view for a PB_LANE row, else in the recipe */
    uint32_t when_mask;               /* PG_V bits of the values it shows for, or PG_ANY */
    const struct pg_choice *names;    /* PK_CHOICE: hi + 1 entries */
};

/* what the tables need from the mod at a call */
struct pg_ctx {
    struct seed_recipe *r;            /* the track's recipe */
    struct seed_shared *shared;
    struct seed_tool *tool;
    int *cursor;                      /* VIEW */
    int lane;                         /* LOCK's lane in view, 0 .. caps->lanes - 1 */
    const struct seed_caps *caps;
    uint16_t fields;                  /* SEED_F_* the track has */
    int length;                       /* the track's length */
    int ngrooves;
    const char *(*track_name)(int t); /* PF_TRACK */
    int live;                         /* 1 when the device gives the playhead (TURING's live page, L8) */
};

extern const struct pg_row pg_rows[];
extern const int pg_nrows;
extern const char *const pg_names[PG_N];   /* the page names, for the title bar */

int pg_built(const uint8_t *have);           /* a seed_have_* flag, 1 for NULL (and in SEEDR_SHOW_ALL builds) */
int pg_row_shown(const struct pg_ctx *c, const struct pg_row *w);
const struct pg_row *pg_row_at(const struct pg_ctx *c, int page, int knob);  /* the row shown there, or NULL */
int pg_page_shown(const struct pg_ctx *c, int page);
int pg_page_step(const struct pg_ctx *c, int page, int d);   /* the next shown page after page in direction d */
int pg_src_built(int src);                   /* RHYTHM SRC's value src is built */

/* turn the row's value by d steps (d = +1 or -1, called once a value step); lo: FUNC held.
 * Returns 1 when the value changed. */
int pg_turn(const struct pg_ctx *c, const struct pg_row *w, int d, int lo);
/* the row's value as text at o, 0-terminated; returns the end. part 0: whole; 1: a range's low
 * end and a '+' (for a cell too narrow for both ends). */
char *pg_text(const struct pg_ctx *c, const struct pg_row *w, char *o, int part);

/* text helpers, also for VIEW: each writes at o and returns the end, without a 0 */
char *pg_put(char *o, const char *s);
char *pg_num(char *o, int v);
char *pg_snum(char *o, int v);
char *pg_len(char *o, int code);
char *pg_cond(char *o, int code);
char *pg_rate(char *o, int code);

#endif
