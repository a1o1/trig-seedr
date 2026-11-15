/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seedr (trig-seedr): an offline pattern generator for the Analog Rytm MKII (OS 1.74), on the current track.
 * Design: docs/design-v1.md; this file is task 0.3's page framework (docs/plan-v1.md).
 *
 *   TRIG              stock: TRIG page, TRIG again: EUCLIDEAN; TRIG on the EUCLIDEAN page opens
 *                     seedr, as a third TRIG page; TRIG on seedr closes it and stock goes back to
 *                     its TRIG page. A parameter page key (SRC, SMPL, FLTR, AMP, LFO, FX) or a
 *                     menu key closes it too. Closing keeps an audition's result.
 *   TRIG held + PAGE  opens it from anywhere (a shortcut; stock does nothing with it)
 *   LEFT / RIGHT      the previous / next page: TAKE RHYTHM PLACE DYNAMICS NOTE LOCK TRIG TIME VIEW
 *                     TOOLS. A page whose features are not built, or that the device lacks, is skipped
 *   UP / DOWN         LOCK: lane 1 / lane 2
 *   knobs A..H        the page's settings (pages.c); FUNC + knob: a range's low end, or 10 (SEED:
 *                     100) a step. VIEW: knob A moves the step cursor
 *   YES               renders the track's recipe from the original (V2): the first YES snapshots the
 *                     track (the audition), every YES after it starts again from that original. A
 *                     YES with no knob turned since the track's last YES takes the next seed.
 *                     TOOLS: runs the tool (ROTATE ALL), its own audition over every track
 *   NO                while auditioning: back to the original; otherwise: close
 *   FUNC + YES        keep the result (ends the audition)
 *   PAGE, TRIG keys   stock's: PAGE picks which 16 steps the TRIG keys show
 *
 * One recipe per track (V5), in RAM, for the pattern in play; a new one starts on SOURCE = TRACK
 * when the track has trigs (V6). The audition also ends, keeping the result, when the pattern or
 * the track changes. Writes go through the stock step routines (device/rytm_ar2.h).
 */
#include <stdint.h>
#include "analog-rytm-mk2/screen.h"
#include "analog-rytm-mk2/store.h"
#include "analog-rytm-mk2/load.h"
#include "../../device/rytm_ar2.h"
#include "pages.h"
/* the groove tables: tools/groove2h.py. grooves-live.h (from your own Ableton Live grooves) is
 * kept out of the repository; without it the swing-only grooves-swing.h is used */
#if __has_include("grooves-live.h")
#include "grooves-live.h"
#else
#include "grooves-swing.h"
#endif

#define K_FUNC 0x80
#define K_TRIG 0x30
/* keys that leave the TRIG page in stock: the parameter pages, FX, the menu keys */
static const uint8_t leave_keys[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x10, 0xb0, 0xb5, 0x11, 0x12, 0x13, 0x14 };

/* Stock's TRIG page view (ParametersSeqNoteView, 1.74's vtable 0x401e3148) keeps its sub-page at +0x8c:
 * 0 TRIG, 1 EUCLIDEAN (TRIG cycles it, 0x4003ab90; a TRIG from another page starts at 0). It is on
 * the view controller's list while the TRIG page shows: +28 the first node, +20 the end; a node's +0
 * the next, +8 its view (elekloader core-ar2 screen.c, sidecar-mods 0026). -1: not showing. */
#define VT_TRIG_VIEW 0x401e3148u
static void *ui_ctrl;                   /* the view controller, from ev_tick */
static int trig_subpage(void)
{
    const uint8_t *c = (const uint8_t *)ui_ctrl;
    const uint32_t *n;
    int k;
    if (!c)
        return -1;
    n = *(const uint32_t *const *)(c + 28);
    for (k = 0; k < 16 && n && (const uint8_t *)n != c + 20; k++) {
        const uint32_t *v = (const uint32_t *)n[2];
        if (v && v[0] == VT_TRIG_VIEW)
            return *(const int32_t *)((const uint8_t *)v + 0x8c);
        n = (const uint32_t *)n[0];
    }
    return -1;
}
#define K_PAGE 0x84
#define K_LEFT 0x40
#define K_RIGHT 0x41
#define K_UP 0x42
#define K_DOWN 0x43
#define K_YES 0x44
#define K_NO 0x45
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))
#define DOWN_BIT 1

/* GCC calls memcpy for large struct copies even when freestanding, and core exports none by
 * that name: a plain one, kept from being turned back into a memcpy call. */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void *memcpy(void *dst, const void *src, unsigned long n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

#define NTRACKS 13                      /* 12 voices and FX */
#define FX_TRACK 12
#define SEED_MAX 9999                   /* SEED's range (pages.c) */

enum { A_NONE, A_TAKE, A_TOOL };

static volatile int open, func_held, trig_down, swallow_trig;
static int page = PG_TAKE, lane, cursor;
static int auditioning;
static const char *msg;
static unsigned msg_until, ticks;
static char status[40];
static void say(const char *m);
#define RT_SHIFT ((void (*)(unsigned, int, int))0x400c1d7a)   /* (track, +-1, length or -1) */

static const char *const names[NTRACKS] = {
    "BD", "SD", "RS", "CP", "BT", "LT", "MT", "HT", "CH", "OH", "CY", "CB", "FX"
};
static const char *track_name(int t)
{
    return t >= 0 && t < NTRACKS ? names[t] : "?";
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* ---- the device (design section 10): the Rytm's struct seed_dev in device/rytm_ar2.h ---- */

#define dev (rt_seed_device())

static unsigned track_obj(int t)
{
    return rt_seed_obj(t);
}

static uint16_t track_fields(int t)
{
    return (uint16_t)rt_seed_fields(t);
}

/* what the audition covers: its tracks, and the pattern and track it was started on */
static unsigned aud_mask, aud_pat;
static int aud_track;

/* ---- the recipe store: one per track (V5) of the pattern in play ---- */

static struct seed_recipe recipe[SEED_TRACKS], last;
static uint16_t recipe_set;            /* a bit per track */
static uint16_t turned = 0xffff;        /* a knob turned since the track's last YES (a bit per track) */
static int have_last;
static unsigned recipe_pat;
static struct seed_shared shared = { 128, 128, 0 };
static struct seed_tool tool = { SEED_TOOL_ROTATE_ALL, 1 };
static struct seed_track orig, take_out, shown;
static struct seed_work work;
static uint8_t shown_defvel;

static void lane_parms(const struct seed_recipe *r, int16_t *parm)
{
    int i;
    for (i = 0; i < SEED_LANES; i++)
        parm[i] = r ? r->lane[i].parm : dev->caps->lane_parm[i];
}

/* the track's recipe; a new one when there is none for this pattern yet (V6, A7) */
static struct seed_recipe *recipe_of(int n)
{
    unsigned obj, pat;
    int16_t parm[SEED_LANES];
    rt_current(&obj, &pat);
    if (pat != recipe_pat) {
        if (recipe_pat)
            recipe_set = 0;             /* another pattern: new recipes (a first call keeps any set) */
        recipe_pat = pat;
    }
    n = clampi(n, 0, SEED_TRACKS - 1);
    if (!(recipe_set & (1u << n))) {
        struct seed_recipe *r = &recipe[n];
        lane_parms(0, parm);
        if (dev->read(n, parm, &shown) == SEED_OK)
            seed_recipe_init(r, &shown, have_last ? &last : 0, dev->caps);
        else
            seed_recipe_defaults(r, dev->caps);
        if (!pg_src_built(r->src))
            r->src = SEED_SRC_TRACK;    /* an empty track and no generator built yet */
        recipe_set |= (uint16_t)(1u << n);
        turned |= (uint16_t)(1u << n);
    }
    return &recipe[n];
}

static void page_ctx(struct pg_ctx *c, int n)
{
    c->r = recipe_of(n);
    c->shared = &shared;
    c->tool = &tool;
    c->cursor = &cursor;
    c->caps = dev->caps;
    c->lane = lane;
    c->fields = track_fields(n);
    c->length = clampi(RT_TRACK_LENGTH(track_obj(n)), 1, SEED_STEPS);
    c->ngrooves = N_GROOVES;
    c->track_name = track_name;
}

/* ---- actions ---- */

static void say(const char *m)
{
    msg = m;
    msg_until = ticks + 40;            /* two seconds at the Rytm's 20 Hz tick */
}

static void keep(void);

/* Start or continue an audition of kind `kind` over the tracks in `mask`: the first call
 * snapshots them, later ones put the originals back first. An audition of another kind, or over
 * other tracks, is kept first (its result becomes the new original). */
static int audition(int kind, unsigned mask)
{
    if (auditioning && (auditioning != kind || aud_mask != mask))
        keep();
    if (auditioning) {
        dev->restore();
        return 1;
    }
    if (dev->snapshot((uint16_t)mask) < 0) {
        say("NO TRACK DATA");
        return 0;
    }
    {
        unsigned obj;
        aud_track = rt_current(&obj, &aud_pat);
    }
    aud_mask = mask;
    auditioning = kind;
    return 1;
}

static const char *error_text(int rc)
{
    switch (rc) {
    case SEED_E_NA:   return "NOT AVAILABLE YET";
    case SEED_E_ROOM: return "NO LOCK ROWS LEFT";  /* the pattern's 72: a new lock would be lost at the save */
    case SEED_E_ARG:  return "SETTING OUT OF RANGE";
    default:          return "FAILED";
    }
}

/* YES: the track's recipe rendered from the original (V2) */
static void take(void)
{
    unsigned obj;
    int n = rt_current(&obj, 0), rc, fresh = !auditioning;
    uint16_t bit = (uint16_t)(1u << n);
    int16_t parm[SEED_LANES];
    struct seed_ctx ctx;
    struct seed_recipe *r;
    if (n == FX_TRACK) {
        say("NOT ON THE FX TRACK");
        return;
    }
    if (rt_euclid(obj)) {
        say("EUCLID IS ON: TURN IT OFF");
        return;
    }
    r = recipe_of(n);
    if (!(turned & bit))
        r->seed = r->seed < SEED_MAX ? r->seed + 1 : 0;     /* YES again: the next seed */
    turned &= (uint16_t)~bit;
    if (!audition(A_TAKE, bit))
        return;
    lane_parms(r, parm);
    rc = dev->read(n, parm, &orig);     /* the original: the snapshot, or the track just restored */
    if (rc >= 0) {
        dev->context(&ctx, n);
        ctx.shared = shared;
        ctx.grooves = grooves;
        ctx.ngrooves = N_GROOVES;
        rc = seed_render(&take_out, &orig, r, &ctx, &work);
    }
    if (rc >= 0)
        rc = dev->write(n, parm, &take_out);
    if (rc < 0) {
        if (fresh) {                    /* nothing was written: no audition */
            dev->drop();
            auditioning = A_NONE;
        }
        say(error_text(rc));
        return;
    }
    last = *r;
    have_last = 1;
    core_project_edited();
}

/* TOOLS, ROTATE ALL: every track's whole steps by STEPS (Euclid tracks are skipped), through
 * stock's trig shift, so their locks move with them and the swing bits stay */
static void tool_run(void)
{
    unsigned cur, pat;
    int t, done = 0;
    char *o;
    rt_current(&cur, &pat);
    if (!audition(A_TOOL, (1u << NTRACKS) - 1))
        return;
    for (t = 0; t < NTRACKS; t++) {
        unsigned obj = RT_TRACK_OBJ(pat, t);
        int len = RT_TRACK_LENGTH(obj), k, i;
        if (rt_euclid(obj) || len < 1 || len > 64)
            continue;
        k = ((tool.steps % len) + len) % len;
        if (k > len / 2)
            for (i = 0; i < len - k; i++)
                RT_SHIFT(obj, -1, -1);
        else
            for (i = 0; i < k; i++)
                RT_SHIFT(obj, 1, -1);
        rt_changed(obj);
        done++;
    }
    o = pg_put(status, "ROTATED ");
    o = pg_num(o, done);
    o = pg_put(o, " TRACKS");
    *o = 0;
    say(status);
    core_project_edited();
}

static void revert(void)
{
    if (auditioning) {
        dev->restore();
        core_project_edited();
        say("BACK TO THE ORIGINAL");
    }
    auditioning = A_NONE;
    dev->drop();
}

static void keep(void)
{
    if (auditioning)
        say("KEPT");
    auditioning = A_NONE;
    dev->drop();
}

/* knob k (0..7) turned one value step d on the page in view; FUNC held: the low end */
static void turn(int k, int d)
{
    struct pg_ctx c;
    const struct pg_row *w;
    int n = dev->current();
    page_ctx(&c, n);
    w = pg_row_at(&c, page, k);
    if (!w || !pg_turn(&c, w, d, func_held))
        return;
    if (!(w->flags & PR_KEEP))
        turned |= (uint16_t)(1u << clampi(n, 0, SEED_TRACKS - 1));
    if (w->fmt == PF_GROOVE) {
        say(c.r->grv > 0 && c.r->grv <= N_GROOVES ? grooves[c.r->grv - 1].name : "NO GROOVE");
    } else if (w->kind == PK_RANGE) {
        /* a range too wide for its cell shows here in full */
        char *o = pg_put(status, w->label);
        *o++ = ' ';
        pg_text(&c, w, o, 0);
        if (core_text_width(CORE_FONT_MAIN, o) > 30)
            say(status);
    }
}

/* ---- events ---- */

int gr_key(void *brain, void *ev)
{
    int k = EV_INT(ev, 12), down = EV_INT(ev, 16) & DOWN_BIT;
    (void)brain;
    if (k == K_FUNC)
        func_held = down;                       /* FUNC always goes on to stock too */
    if (k == K_TRIG) {
        trig_down = down;
        if (!down && swallow_trig) {            /* the release of the press that opened seedr */
            swallow_trig = 0;
            return 1;
        }
        if (down && !open && trig_subpage() == 1) {
            open = 1;                           /* the third TRIG page */
            swallow_trig = 1;
            return 1;
        }
        if (down && open) {
            open = 0;                           /* and stock goes on from EUCLIDEAN to TRIG */
            keep();
        }
        return 0;
    }
    if (k == K_PAGE && trig_down) {           /* the shortcut: opens, never closes */
        if (down)
            open = 1;
        return 1;
    }
    if (open && down) {
        unsigned i;
        for (i = 0; i < sizeof leave_keys; i++)
            if (k == leave_keys[i]) {
                open = 0;                       /* another page or a menu: seedr closes */
                keep();
                return 0;
            }
    }
    if (!open)
        return 0;
    switch (k) {
    case K_LEFT: case K_RIGHT:
        if (down) {
            struct pg_ctx c;
            page_ctx(&c, dev->current());
            page = pg_page_step(&c, page, k == K_LEFT ? -1 : 1);
        }
        return 1;
    case K_UP: case K_DOWN:
        if (down && page == PG_LOCK)
            lane = clampi(lane + (k == K_UP ? -1 : 1), 0, dev->caps->lanes - 1);
        return 1;
    case K_YES:
        if (down) {
            if (func_held)
                keep();
            else if (page == PG_TOOLS)
                tool_run();
            else
                take();
        }
        return 1;
    case K_NO:
        if (down) {
            if (auditioning)
                revert();
            else
                open = 0;
        }
        return 1;
    default:
        return 0;                               /* PAGE, the TRIG keys, transport: stock's */
    }
}

/* The Rytm's knobs as stock takes them (analog-emu's measurements, OS 1.73): ev_enc brings raw
 * counts (knob 0..7 = A..H at ev+12, not 1..8 as on the Digitone); a knob idle for about 2 s is
 * asleep and its first two turn events only wake it; then 4 counts make one value step, so a
 * faster turn moves further. While seedr is open every knob is its own (stock's TRIG page knobs
 * would change the trig defaults unseen). */
#define KNOB_AWAKE_TICKS 40               /* 2 s at the 20 Hz tick: stock keeps a knob awake 2..4 s */
static unsigned knob_last[8];
static int knob_wake[8], knob_acc[8];

int gr_enc(void *brain, void *ev)
{
    int e = EV_INT(ev, 12), d = EV_INT(ev, 16), steps;
    (void)brain;
    if (!open)
        return 0;
    if (e < 0 || e > 7 || !d)
        return 1;
    if (!knob_last[e] || ticks - knob_last[e] > KNOB_AWAKE_TICKS) {
        knob_wake[e] = 2;                 /* asleep: the next two turns wake it */
        knob_acc[e] = 0;
    }
    knob_last[e] = ticks ? ticks : 1;
    if (knob_wake[e] > 0) {
        knob_wake[e]--;
        return 1;
    }
    if ((knob_acc[e] > 0 && d < 0) || (knob_acc[e] < 0 && d > 0))
        knob_acc[e] = 0;                  /* a change of direction starts afresh */
    knob_acc[e] += d;
    steps = knob_acc[e] / 4;
    knob_acc[e] -= steps * 4;
    while (steps > 0) { turn(e, 1); steps--; }
    while (steps < 0) { turn(e, -1); steps++; }
    return 1;
}

void gr_tick(void *ctrl)
{
    ticks++;
    ui_ctrl = ctrl;
    if (auditioning) {
        unsigned obj, pat;
        int n = rt_current(&obj, &pat);
        if (pat != aud_pat || (auditioning == A_TAKE && n != aud_track))
            keep();                             /* another track or pattern: the result stays */
    }
    if (open)
        *((volatile uint8_t *)ctrl + 0x20) = 1;
}

/* ---- drawing (elekloader examples/screen-check-ar2). Rows count from the top, 0..63:
 * the title bar 0..9, the knob cells 12..47 (two rows of four), the strip 51..63. ---- */

#define STRIP_TOP 51
#define STRIP_BOTTOM 63

static uint8_t *px(void *bmp)
{
    return *(uint8_t **)((uint8_t *)bmp + 16);
}

static void dot(uint8_t *p, int x, int r)
{
    if (x >= 0 && x < 128 && r >= 0 && r < 64)
        p[8 * x + 7 - r / 8] |= (uint8_t)(1 << (r % 8));
}

static void undot(uint8_t *p, int x, int r)
{
    if (x >= 0 && x < 128 && r >= 0 && r < 64)
        p[8 * x + 7 - r / 8] &= (uint8_t)~(1 << (r % 8));
}

static void box(uint8_t *p, int x0, int r0, int x1, int r1, int fill)
{
    int x, r;
    for (x = x0; x <= x1; x++)
        for (r = r0; r <= r1; r++)
            if (fill || x == x0 || x == x1 || r == r0 || r == r1)
                dot(p, x, r);
}

static void text_at(void *bmp, int x, int top, const char *s, int flags)
{
    int h = core_font_height(CORE_FONT_MAIN);
    core_text_draw(bmp, CORE_FONT_MAIN, x, 64 - h - top, CORE_ALIGN_LEFT | flags, s);
}

static void text_right(void *bmp, int x, int top, const char *s, int flags)
{
    int h = core_font_height(CORE_FONT_MAIN);
    core_text_draw(bmp, CORE_FONT_MAIN, x, 64 - h - top, CORE_ALIGN_RIGHT | flags, s);
}

/* The title bar: the page (and LOCK's lane), the track; at the right the source, TRK or S and
 * the seed (V6), after AUD while a take is auditioned; along its foot a mark per page shown, the
 * page in view the long one. */
static void draw_title(void *bmp, uint8_t *p, const struct pg_ctx *c, int n)
{
    char line[40], *o = line;
    int i, shown_pages = 0, x;
    box(p, 0, 0, 127, 9, 1);
    o = pg_put(o, pg_names[page]);
    if (page == PG_LOCK && dev->caps->lanes > 1) {
        *o++ = ' ';
        o = pg_num(o, lane + 1);
    }
    *o++ = ' ';
    o = pg_put(o, track_name(n));
    *o = 0;
    text_at(bmp, 2, 1, line, CORE_TEXT_INVERT);
    o = line;
    if (auditioning)
        o = pg_put(o, "AUD ");
    if (c->r->src == SEED_SRC_TRACK)
        o = pg_put(o, "TRK");
    else {
        *o++ = 'S';
        o = pg_num(o, (int)c->r->seed);
    }
    *o = 0;
    text_right(bmp, 126, 1, line, CORE_TEXT_INVERT);
    for (i = 0; i < PG_N; i++)
        shown_pages += pg_page_shown(c, i);
    x = 64 - (shown_pages * 5) / 2;
    for (i = 0; i < PG_N; i++) {
        if (!pg_page_shown(c, i))
            continue;
        undot(p, x + 1, 8);
        undot(p, x + 2, 8);
        if (i == page) {
            undot(p, x, 8);
            undot(p, x + 3, 8);
        }
        x += 5;
    }
}

/* one knob's label and value, in a 32-pixel column, two rows of four */
static void cell(void *bmp, int k, const char *label, const char *value)
{
    int x = 1 + 32 * (k & 3), top = k < 4 ? 12 : 33;
    text_at(bmp, x, top, label, 0);
    text_at(bmp, x, top + 10, value, 0);
}

static void draw_cells(void *bmp, const struct pg_ctx *c)
{
    char v[32];
    int k;
    for (k = 0; k < 8; k++) {
        const struct pg_row *w = pg_row_at(c, page, k);
        if (!w)
            continue;
        pg_text(c, w, v, 0);
        if (core_text_width(CORE_FONT_MAIN, v) > 30)
            pg_text(c, w, v, 1);
        cell(bmp, k, w->label, v);
    }
}

static char *val(char *o, const char *label, int v)
{
    o = pg_put(o, label);
    return v == SEED_DEFAULT ? pg_put(o, "-") : pg_num(o, v);
}

/* VIEW: the 16 steps around the cursor and the cursor step's fields, read-only */
static void draw_view(void *bmp, uint8_t *p, const struct seed_track *t, int parm0)
{
    char line[48], *o;
    const struct seed_step *s;
    int i, pg;
    if (cursor >= t->length)
        cursor = t->length - 1;
    pg = cursor / 16 * 16;
    for (i = 0; i < 16; i++) {
        int x = 2 + 8 * i, st = pg + i;
        if (st >= t->length)
            break;
        s = &t->s[st];
        if (s->flags & SEED_TRIG) {
            if (s->flags & 0x0380)
                box(p, x, 14, x + 5, 20, 1);
            else
                box(p, x + 2, 16, x + 3, 18, 1);
        }
        if (s->flags & SEED_ACCENT)
            box(p, x, 22, x + 5, 22, 1);
        if (s->flags & SEED_RETRIG)
            box(p, x + 2, 12, x + 3, 12, 1);
        if (st == cursor) {
            box(p, x + 2, 24, x + 3, 24, 1);
            box(p, x + 1, 25, x + 4, 25, 1);
        }
    }
    s = &t->s[cursor];
    o = pg_put(line, "S");
    o = pg_num(o, cursor + 1);
    o = val(o, " N", s->note);
    o = val(o, " V", s->vel);
    o = val(o, " L", s->len);
    o = pg_put(o, " U");
    o = pg_snum(o, s->utime);
    *o = 0;
    text_at(bmp, 2, 28, line, 0);
    o = pg_put(line, "C");
    o = pg_cond(o, s->cond);
    o = pg_put(o, " R");
    if (s->flags & SEED_RETRIG) {
        o = pg_rate(o, s->rrate);
        *o++ = ' ';
        o = pg_num(o, s->rvel);
    } else
        o = pg_put(o, "-");
    if (s->lane[0] != SEED_NO_LOCK) {
        *o++ = ' ';
        o = pg_put(o, dev->caps->params[clampi(parm0, 0, dev->caps->nparams - 1)].name);
        o = pg_num(o, s->lane[0]);
    }
    *o = 0;
    text_at(bmp, 2, 38, line, 0);
    {
        /* the flag word and the track defaults on the right of the same line; the defaults drop
         * out when a long retrig text leaves no room */
        char right[24];
        int lw = core_text_width(CORE_FONT_MAIN, line);
        o = pg_put(right, "F");
        for (i = 12; i >= 0; i -= 4)
            *o++ = "0123456789ABCDEF"[(s->flags >> i) & 15];
        o = pg_put(o, " DN");
        o = pg_num(o, t->default_note);
        o = pg_put(o, " DV");
        o = pg_num(o, shown_defvel);
        *o = 0;
        if (2 + lw + 6 + core_text_width(CORE_FONT_MAIN, right) > 127)
            right[5] = 0;
        text_right(bmp, 126, 38, right, 0);
    }
}

/* The strip: a message for two seconds, or the track as it is now, 2 pixels a step, bar height
 * = velocity (a dot on every 4th step marks the beat). A placeholder for the preview (task F10:
 * the take as it would be written, from the recipe on every knob change). */
static void draw_strip(void *bmp, uint8_t *p, const struct seed_track *t)
{
    int i;
    if (msg && (int)(msg_until - ticks) > 0) {
        text_at(bmp, 1, STRIP_TOP + 3, msg, 0);
        return;
    }
    for (i = 0; i < t->length && i < SEED_STEPS; i++) {
        const struct seed_step *s = &t->s[i];
        int x = 2 * i;
        if (s->flags & SEED_TRIG) {
            int v = s->vel == SEED_DEFAULT ? shown_defvel : s->vel;
            int h = 1 + clampi(v, 1, 127) * (STRIP_BOTTOM - STRIP_TOP - 1) / 127;
            box(p, x, STRIP_BOTTOM - h + 1, x, STRIP_BOTTOM, 1);
        } else if (!(i & 3))
            dot(p, x, STRIP_BOTTOM);
        if (page == PG_VIEW && i == cursor)
            dot(p, x, STRIP_TOP - 1);
    }
}

void gr_draw(void *bmp, void *ctrl)
{
    struct pg_ctx c;
    uint8_t *p;
    int16_t parm[SEED_LANES];
    int n, i, ok;
    (void)ctrl;
    if (!open || core_stock_overlay())
        return;
    n = dev->current();
    page_ctx(&c, n);
    if (!pg_page_shown(&c, page))
        page = pg_page_step(&c, page, 1);
    if (lane >= dev->caps->lanes)
        lane = c.lane = 0;
    lane_parms(c.r, parm);
    ok = dev->read(n, parm, &shown) == SEED_OK;
    shown_defvel = shown.default_vel;
    p = px(bmp);
    for (i = 0; i < 1024; i++)
        p[i] = 0;
    draw_title(bmp, p, &c, n);
    if (page == PG_VIEW) {
        if (ok)
            draw_view(bmp, p, &shown, parm[0]);
    } else
        draw_cells(bmp, &c);
    if (ok)
        draw_strip(bmp, p, &shown);
}
