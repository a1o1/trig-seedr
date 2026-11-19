/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The frame of seedr (trig-seedr): an offline pattern generator on the current track, for any
 * device whose part (frame.h) defines the instrument's side. Design: docs/design-v1.md; split out of
 * mods/seedr-ar2/seedr.c by task S1.1 (docs/plan-syntakt.md). A device's seedr.c includes this file
 * after its own definitions (frame.h): one translation unit.
 *
 *   TRIG              stock: TRIG page, TRIG again: EUCLIDEAN; TRIG on the EUCLIDEAN page opens
 *                     seedr, as a third TRIG page; TRIG on seedr closes it and stock goes back to
 *                     its TRIG page. A key that leaves stock's TRIG page (sd_leave_keys: the
 *                     parameter pages, FX, the menus) closes it too.
 *   TRIG held + PAGE  opens it from anywhere (a shortcut; stock does nothing with it)
 *   LEFT / RIGHT      the previous / next page: TAKE RHYTHM PLACE DYNAMICS NOTE LOCK TRIG TIME VIEW
 *                     TOOLS. A page whose features are not built, or that the device lacks, is skipped
 *   UP / DOWN         LOCK: lane 1 / lane 2
 *   knobs A..H        the page's settings (pages.c); FUNC + knob: a range's low end, or 10 (SEED:
 *                     100) a step. VIEW: knob A moves the step cursor
 *   YES               generate (needs grid recording; otherwise RECORD TO GENERATE): renders the
 *                     track's recipe from the original (V2) and writes it to be heard. The first YES
 *                     snapshots the track, every YES after it starts again from that original. A
 *                     YES with no knob turned takes the next seed only while the track's take is
 *                     playing or just after it was applied; after NO or NOT APPLIED it brings the
 *                     same take back, so FUNC + YES applies the take that was heard (M2.5)
 *                     TOOLS: runs the tool: ROTATE ALL over every track, the same way; YOLO rolls a
 *                     recipe for this track (WILD: how far) and generates it. NO puts the track and
 *                     the recipe back as they were before the first roll
 *   NO                with a take pending: back to the original; otherwise: close
 *   FUNC + YES        apply: the take stays, the original is dropped. The only way a take stays
 *   PAGE, TRIG keys   stock's: PAGE picks which 16 steps the TRIG keys show
 *
 * One recipe per track (V5), in RAM, for the pattern in play; a new one starts on SOURCE = TRACK
 * when the track has trigs (V6). A pending take is not applied (the original comes back) when the
 * track or pattern changes, seedr closes, or grid recording ends (Alan, 2026-11-16: generate and
 * apply, unlike stock's Euclidean mode, which plays from its own table and never writes trigs).
 * Writes go through the device layer (struct seed_dev), which uses stock's step routines.
 */
#include <stddef.h>
#include <stdint.h>
#include "frame.h"

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

#define SEED_MAX 9999                   /* SEED's range (pages.c) */

enum { A_NONE, A_TAKE, A_TOOL };

static void *ui_ctrl;                   /* the view controller, from ev_tick (sd_trig_subpage) */

static volatile int open, func_held, trig_down, swallow_trig;
static int page = PG_TAKE, lane, cursor;
static int auditioning;
static const char *msg;
static unsigned msg_until, ticks;
static char status[40];
static int scr_dirty = 1;               /* an event may have changed the screen: draw it in full (M2.3) */
static void say(const char *m);

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* ---- the device (design section 10): its struct seed_dev, from the device part (frame.h) ---- */

#define dev (sd_device())

/* what the audition covers: its tracks, and the pattern and track it was started on */
static unsigned aud_mask, aud_pat;
static int aud_track;

/* ---- the recipe store: one per track (V5) of the pattern in play ---- */

static struct seed_recipe recipe[SEED_TRACKS], last;
static unsigned recipe_set;             /* a bit per track (an int: tools/emu.sh reads it) */
static uint16_t turned = 0xffff;        /* a knob turned since the track's last YES (a bit per track) */
static uint16_t kept;                   /* the track's last take was applied (FUNC + YES): YES moves on */
static int have_last;
static unsigned recipe_pat;
static struct seed_shared shared = { 128, 128, 0 };
static struct seed_tool tool = { SEED_TOOL_ROTATE_ALL, 1, 50, 0 };
/* YOLO's undo (0.7): the track's recipe before its first roll; kept until FUNC + YES, dropped with
 * the take otherwise (NO, NOT APPLIED put it back) */
static struct seed_recipe yolo_undo;
static int yolo_track = -1, yolo_used = -1;
/* The take's buffers. A device part that defines SD_FRAME_HEAP gives them from its heap instead
 * (frame_heap_size, frame_heap_attach), when its mods' area is too small for them (the Syntakt). */
#ifdef SD_FRAME_HEAP
struct frame_bufs {
    struct seed_track orig, take_out, shown;
    struct seed_work work;
};
static struct frame_bufs *frame_bufs;
#define f_orig (frame_bufs->orig)
#define f_take_out (frame_bufs->take_out)
#define f_shown (frame_bufs->shown)
#define f_work (frame_bufs->work)
#else
static struct seed_track f_orig, f_take_out, f_shown;
static struct seed_work f_work;
#endif

static void lane_parms(const struct seed_recipe *r, int16_t *parm)
{
    int i;
    for (i = 0; i < SEED_LANES; i++)
        parm[i] = r ? r->lane[i].parm : dev->caps->lane_parm[i];
}

/* the track's recipe; a new one when there is none for this pattern yet (V6, A7) */
static struct seed_recipe *recipe_of(int n)
{
    unsigned pat;
    int16_t parm[SEED_LANES];
    sd_current(&pat);
    if (pat != recipe_pat) {
        if (recipe_pat)
            recipe_set = 0;             /* another pattern: new recipes (a first call keeps any set) */
        recipe_pat = pat;
    }
    n = clampi(n, 0, SEED_TRACKS - 1);
    if (!(recipe_set & (1u << n))) {
        struct seed_recipe *r = &recipe[n];
        lane_parms(0, parm);
        if (dev->read(n, parm, &f_shown) == SEED_OK)
            seed_recipe_init(r, &f_shown, have_last ? &last : 0, dev->caps);
        else
            seed_recipe_defaults(r, dev->caps);
        if (!pg_src_built(r->src))
            r->src = SEED_SRC_TRACK;    /* an empty track and no generator built yet */
        recipe_set |= 1u << n;
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
    c->fields = sd_track_fields(n);
    c->length = clampi(sd_track_length(n), 1, SEED_STEPS);
    c->ngrooves = N_GROOVES;
    c->track_name = sd_track_name;
}

/* ---- actions ---- */

static void say(const char *m)
{
    msg = m;
    msg_until = ticks + 2 * SD_TICK_HZ;    /* two seconds */
    scr_dirty = 1;
}

static void not_applied(void);

/* Start or continue an audition of kind `kind` over the tracks in `mask`: the first call
 * snapshots them, later ones put the originals back first. A take of another kind, or over other
 * tracks, still pending is not applied: its originals come back first. */
static int audition(int kind, unsigned mask)
{
    if (auditioning && (auditioning != kind || aud_mask != mask))
        not_applied();
    if (auditioning) {
        dev->restore();
        return 1;
    }
    if (dev->snapshot((uint16_t)mask) < 0) {
        say("NO TRACK DATA");
        return 0;
    }
    aud_track = sd_current(&aud_pat);
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

/* YES (generate): the track's recipe rendered from the original (V2) and written so it can be
 * heard; the original is kept until FUNC + YES applies the take. Needs grid recording. */
static int take(void)
{
    int n = sd_current(0), rc, fresh = !auditioning;
    uint16_t bit = (uint16_t)(1u << n);
    int16_t parm[SEED_LANES];
    struct seed_ctx ctx;
    struct seed_recipe *r;
    if (n == SD_FX_TRACK) {
        say("NOT ON THE FX TRACK");
        return 0;
    }
    if (!sd_grid_recording()) {
        say("RECORD TO GENERATE");      /* generate and apply need grid recording, as editing does */
        return 0;
    }
    if (sd_euclid()) {
        say("EUCLID IS ON: TURN IT OFF");
        return 0;
    }
    r = recipe_of(n);
    /* YES again on the take being heard, or after applying it: the next seed. A take that was put
     * back (NO, NOT APPLIED) comes back as it was (Alan, M2.5) */
    if (!(turned & bit) && ((auditioning == A_TAKE && aud_mask == bit) || (kept & bit)))
        r->seed = r->seed < SEED_MAX ? r->seed + 1 : 0;
    turned &= (uint16_t)~bit;
    kept &= (uint16_t)~bit;
    if (!audition(A_TAKE, bit))
        return 0;
    lane_parms(r, parm);
    rc = dev->read(n, parm, &f_orig);     /* the original: the snapshot, or the track just restored */
    if (rc >= 0) {
        dev->context(&ctx, n);
        ctx.shared = shared;
        ctx.grooves = grooves;
        ctx.ngrooves = N_GROOVES;
        rc = seed_render(&f_take_out, &f_orig, r, &ctx, &f_work);
    }
    if (rc >= 0)
        rc = dev->write(n, parm, &f_take_out);
    if (rc < 0) {
        if (fresh) {                    /* nothing was written: no audition */
            dev->drop();
            auditioning = A_NONE;
        }
        say(error_text(rc));
        return 0;
    }
    last = *r;
    have_last = 1;
    sd_edited();
    return 1;
}

/* TOOLS, YOLO (0.7): roll a recipe for this track and generate it, as YES on a page would. YES with
 * ROLL as it was after the last roll takes the next roll (as SEED); after NO or NOT APPLIED, or with ROLL
 * turned, YES plays the roll ROLL shows. The first roll keeps the recipe it
 * replaces: NO, or the take not applied, puts it back with the track; FUNC + YES keeps the roll. */
static void yolo_run(void)
{
    int n = sd_current(0), i;
    int16_t parm[SEED_LANES];
    struct seed_recipe *r;
    struct seed_yolo_in in;
    char *o;
    if (n == SD_FX_TRACK) {
        say("NOT ON THE FX TRACK");
        return;
    }
    if (!sd_grid_recording()) {
        say("RECORD TO GENERATE");
        return;
    }
    if (auditioning && !(auditioning == A_TAKE && aud_mask == (1u << n)))
        not_applied();                          /* ROTATE ALL pending: put back first, not mid-roll */
    r = recipe_of(n);
    if (yolo_track != n) {
        yolo_undo = *r;                         /* the recipe before the first roll: NO's */
        yolo_track = n;
    }
    if (tool.roll == yolo_used)
        tool.roll = (int16_t)(tool.roll < 9999 ? tool.roll + 1 : 0);
    yolo_used = tool.roll;
    lane_parms(r, parm);
    if (dev->read(n, parm, &f_shown) != SEED_OK)
        f_shown.length = 0;
    in.caps = dev->caps;
    in.role = sd_track_role(n);
    in.length = clampi(sd_track_length(n), 1, SEED_STEPS);
    in.fields = sd_track_fields(n);
    in.ngrooves = N_GROOVES;
    for (in.any = 0, i = 0; i < f_shown.length && i < SEED_STEPS; i++)
        in.any |= f_shown.s[i].flags & SEED_TRIG;
    seed_yolo(r, &yolo_undo, tool.roll, tool.wild, &in);
    turned |= (uint16_t)(1u << n);              /* take() writes the rolled seed, not the next */
    if (!take()) {                              /* not written (the screen says why): the recipe back */
        recipe[n] = yolo_undo;
        if (!auditioning)
            yolo_track = -1;
        return;
    }
    o = pg_put(status, "ROLL ");
    o = pg_num(o, tool.roll);
    *o = 0;
    say(status);
}

/* TOOLS, ROTATE ALL: every track's whole steps by STEPS (Euclid tracks are skipped), through
 * stock's trig shift, so their locks move with them and the swing bits stay (sd_rotate) */
static void tool_run(void)
{
    int t, done = 0;
    char *o;
    if (!sd_grid_recording()) {
        say("RECORD TO GENERATE");
        return;
    }
    if (!audition(A_TOOL, (1u << SD_TRACKS) - 1))
        return;
    for (t = 0; t < SD_TRACKS; t++)
        done += sd_rotate(t, tool.steps);
    o = pg_put(status, "ROTATED ");
    o = pg_num(o, done);
    o = pg_put(o, " TRACKS");
    *o = 0;
    say(status);
    sd_edited();
}

/* the originals back, the take dropped; `m` says why */
static void back(const char *m)
{
    if (yolo_track >= 0) {                      /* a roll not applied: the recipe as it was too */
        recipe[yolo_track] = yolo_undo;
        turned |= (uint16_t)(1u << yolo_track);
        yolo_track = -1;
        yolo_used = -1;                         /* YES on YOLO brings the roll that was heard back (M2.5) */
    }
    if (auditioning) {
        dev->restore();
        sd_edited();
        say(m);
    }
    auditioning = A_NONE;
    dev->drop();
}

/* NO */
static void revert(void)
{
    back("BACK TO THE ORIGINAL");
}

/* leaving the track or pattern, closing seedr, turning RECORD off: a take only stays when it is
 * applied (FUNC + YES), so a pending one is not */
static void not_applied(void)
{
    back("NOT APPLIED");
}

/* FUNC + YES: the take stays, the original is dropped */
static void apply(void)
{
    yolo_track = -1;                            /* the rolled recipe stays with its take */
    if (auditioning == A_TAKE)
        kept |= (uint16_t)aud_mask;
    if (auditioning)
        say("APPLIED");
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
    if (w->base == PB_TOOL && w->off == offsetof(struct seed_tool, roll))
        yolo_used = -1;                         /* ROLL turned: YES plays the roll it shows */
    if (w->fmt == PF_GROOVE) {
        say(c.r->grv > 0 && c.r->grv <= N_GROOVES ? grooves[c.r->grv - 1].name : "NO GROOVE");
    } else if (w->kind == PK_RANGE) {
        /* a range too wide for its cell shows here in full */
        char *o = pg_put(status, w->label);
        *o++ = ' ';
        pg_text(&c, w, o, 0);
        if (sd_text_width(o) > 30)
            say(status);
    }
}

/* ---- events ---- */

int gr_key(void *brain, void *ev)
{
    int k = SD_KEY(ev), down = SD_DOWN(ev);
    (void)brain;
    scr_dirty = 1;                              /* any key may change what the screen shows */
    if (k == SD_K_FUNC)
        func_held = down;                       /* FUNC always goes on to stock too */
    if (k == SD_K_TRIG) {
        trig_down = down;
        if (!down && swallow_trig) {            /* the release of the press that opened seedr */
            swallow_trig = 0;
            return 1;
        }
        if (down && !open && sd_trig_subpage(ui_ctrl) == 1) {
            open = 1;                           /* the third TRIG page */
            swallow_trig = 1;
            return 1;
        }
        if (down && open) {
            open = 0;                           /* and stock goes on from EUCLIDEAN to TRIG */
            not_applied();
        }
        return 0;
    }
    if (k == SD_K_PAGE && trig_down) {           /* the shortcut: opens, never closes */
        if (down)
            open = 1;
        return 1;
    }
    if (open && down) {
        unsigned i;
        for (i = 0; i < sizeof sd_leave_keys; i++)
            if (k == sd_leave_keys[i]) {
                open = 0;                       /* another page or a menu: seedr closes */
                not_applied();
                return 0;
            }
    }
    if (!open)
        return 0;
    switch (k) {
    case SD_K_LEFT: case SD_K_RIGHT:
        if (down) {
            struct pg_ctx c;
            page_ctx(&c, dev->current());
            page = pg_page_step(&c, page, k == SD_K_LEFT ? -1 : 1);
        }
        return 1;
    case SD_K_UP: case SD_K_DOWN:
        if (down && page == PG_LOCK)
            lane = clampi(lane + (k == SD_K_UP ? -1 : 1), 0, dev->caps->lanes - 1);
        return 1;
    case SD_K_YES:
        if (down) {
            if (func_held)
                apply();
            else if (page == PG_TOOLS && tool.tool == SEED_TOOL_YOLO)
                yolo_run();
            else if (page == PG_TOOLS)
                tool_run();
            else
                take();
        }
        return 1;
    case SD_K_NO:
        if (down) {
            if (auditioning)
                revert();
            else
                open = 0;                       /* nothing pending: NO closes */
        }
        return 1;
    default:
        return 0;                               /* PAGE, the TRIG keys, transport: stock's */
    }
}

/* The knobs as stock takes them (the device part's SD_KNOB_*; the Rytm's from analog-emu's
 * measurements, OS 1.73): ev_enc brings raw counts; a knob idle for about 2 s is asleep and its
 * first SD_KNOB_WAKE turn events only wake it; then SD_KNOB_COUNTS counts make one value step, so a
 * faster turn moves further. While seedr is open every knob is its own (stock's TRIG page knobs
 * would change the trig defaults unseen). */
#define KNOB_AWAKE_TICKS (2 * SD_TICK_HZ) /* 2 s: stock keeps a knob awake 2..4 s */
static unsigned knob_last[8];
static int knob_wake[8], knob_acc[8];

int gr_enc(void *brain, void *ev)
{
    int e = SD_ENC(ev), d = SD_TURN(ev), steps;
    (void)brain;
    if (!open)
        return 0;
    scr_dirty = 1;
    if (e < 0 || e > 7 || !d)
        return 1;
    if (!knob_last[e] || ticks - knob_last[e] > KNOB_AWAKE_TICKS) {
        knob_wake[e] = SD_KNOB_WAKE;      /* asleep: the next turns wake it */
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
    steps = knob_acc[e] / SD_KNOB_COUNTS;
    knob_acc[e] -= steps * SD_KNOB_COUNTS;
    while (steps > 0) { turn(e, 1); steps--; }
    while (steps < 0) { turn(e, -1); steps++; }
    return 1;
}

static int scr_changed(void);

void gr_tick(void *ctrl)
{
    ticks++;
    ui_ctrl = ctrl;
    if (auditioning) {
        unsigned pat;
        int n = sd_current(&pat);
        if (pat != aud_pat || (auditioning == A_TAKE && n != aud_track) || !sd_grid_recording())
            not_applied();                      /* another track or pattern, or RECORD off */
    }
    if (open && scr_changed())
        sd_request_frame(ctrl);                 /* stock draws a frame, and seedr's screen over it */
}

/* ---- drawing (elekloader examples/screen-check-ar2). Rows count from the top, 0..63:
 * the title bar 0..9, the knob cells 12..47 (two rows of four), the strip 51..63. ---- */

#define STRIP_TOP 51
#define STRIP_BOTTOM 63

static uint8_t *px(void *bmp)
{
    return sd_pixels(bmp);
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
    int h = sd_font_height();
    sd_text_draw(bmp, x, 64 - h - top, SF_LEFT | flags, s);
}

static void text_right(void *bmp, int x, int top, const char *s, int flags)
{
    int h = sd_font_height();
    sd_text_draw(bmp, x, 64 - h - top, SF_RIGHT | flags, s);
}

/* preview.c's and view.c's text (preview.h) */
void sf_text(void *bmp, int x, int top, const char *s, int align)
{
    sd_text_draw(bmp, x, 64 - sd_font_height() - top, align, s);
}

int sf_text_width(const char *s)
{
    return sd_text_width(s);
}

/* the sprout, 7 x 7: two leaves on a stem, in the ground (the title bar and VIEW's footer) */
static const char *const sprout[7] = {
    "##...##",
    "#.#.#.#",
    ".##.##.",
    "...#...",
    "...#...",
    ".#####.",
    "#######",
};

/* The title bar: the sprout, then the page (and LOCK's lane), the track and the page's place
 * among the pages shown (3/10); at the right the source: TRK, EUC, CPY, MAP (CHAOS below 4), or S
 * and the seed (V6; the seed only for a source that draws from it), after a dot while a take is pending (not applied:
 * FUNC + YES keeps it; M2.5, was AUD). The place is left out when the two sides would not fit. */
#define DOT_W 7                                 /* the pending dot, 5 x 5, and the 2 pixels after it */
static const char dot5[5][6] = { ".###.", "#####", "#####", "#####", ".###." };
#define TITLE_X 13                              /* the sprout (2..8), a space, then the text */
static void draw_title(void *bmp, uint8_t *p, const struct pg_ctx *c, int n)
{
    char line[40], right[16], *o = line, *place;
    int i, at = 0, count = 0;
    for (i = 0; i < 128; i++) {                 /* the bar, rows 0..9: box(p, 0, 0, 127, 9, 1) a byte at a time */
        p[8 * i + 7] = 0xff;                    /* rows 0..7 */
        p[8 * i + 6] |= 0x03;                   /* rows 8, 9 */
    }
    for (i = 0; i < 7; i++)                    /* cut out of the bar, as the text is */
        for (at = 0; at < 7; at++)
            if (sprout[i][at] == '#')
                undot(p, 2 + at, 1 + i);
    at = 0;
    o = pg_put(o, pg_names[page]);
    if (page == PG_LOCK && dev->caps->lanes > 1) {
        *o++ = ' ';
        o = pg_num(o, lane + 1);
    }
    *o++ = ' ';
    o = pg_put(o, sd_track_name(n));
    place = o;
    for (i = 0; i < PG_N; i++)
        if (pg_page_shown(c, i)) {
            count++;
            if (i == page)
                at = count;
        }
    if (at) {
        *o++ = ' ';
        o = pg_num(o, at);
        *o++ = '/';
        o = pg_num(o, count);
    }
    *o = 0;
    o = right;
    if (c->r->src == SEED_SRC_TRACK)
        o = pg_put(o, "TRK");
    else if (c->r->src == SEED_SRC_EUCLID)       /* the sources that draw no random numbers */
        o = pg_put(o, "EUC");
    else if (c->r->src == SEED_SRC_COPY)
        o = pg_put(o, "CPY");
    else if (c->r->src == SEED_SRC_MAP && c->shared->map_chaos < 4)   /* CHAOS >> 2 is 0: no offset */
        o = pg_put(o, "MAP");
    else {
        *o++ = 'S';
        o = pg_num(o, (int)c->r->seed);
    }
    *o = 0;
    if (TITLE_X + sd_text_width(line) + 6 + sd_text_width(right) +
            (auditioning ? DOT_W : 0) > 126)
        *place = 0;
    text_at(bmp, TITLE_X, 1, line, SF_INVERT);
    text_right(bmp, 126, 1, right, SF_INVERT);
    if (auditioning) {                          /* cut out of the bar, as the sprout is */
        int x = 126 - sd_text_width(right) - DOT_W, y, k;
        for (y = 0; y < 5; y++)
            for (k = 0; k < 5; k++)
                if (dot5[y][k] == '#')
                    undot(p, x + k, 2 + y);
    }
}

/* VIEW's footer, as 0.3's: the name at the left, with the sprout after it */
static void draw_footer(void *bmp, uint8_t *p)
{
    int w = sd_text_width("Trig Seedr"), x = 2, r, k;
    box(p, 0, 47, 127, 47, 1);                  /* the divider */
    text_at(bmp, x, 52, "Trig Seedr", 0);
    x += w + 4;
    for (r = 0; r < 7; r++)
        for (k = 0; k < 7; k++)
            if (sprout[r][k] == '#')
                dot(p, x + k, 50 + r);
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
        if (sd_text_width(v) > 30)
            pg_text(c, w, v, 1);
        cell(bmp, k, w->label, v);
    }
}

/* The strip: a message for two seconds, else the preview (preview.c): what YES would write on
 * this track, rendered from the recipe and the original. With an audition of this track's take
 * running, the original is the one take() read after restoring the snapshot. */
static void draw_strip(void *bmp, uint8_t *p, int n, const struct seed_recipe *r)
{
    struct pv_in in;
    const char *why = 0;
    int rc;
    if (msg && (int)(msg_until - ticks) > 0) {
        text_at(bmp, 1, STRIP_TOP + 3, msg, 0);
        return;
    }
    if (n == SD_FX_TRACK)
        why = "NOT ON THE FX TRACK";
    else if (sd_euclid())
        why = "EUCLID IS ON: TURN IT OFF";
    else {
        in.device = dev;
        in.track = n;
        in.r = r;
        in.shared = &shared;
        in.grooves = grooves;
        in.ngrooves = N_GROOVES;
        in.orig = auditioning == A_TAKE && n == aud_track ? &f_orig : &f_shown;
        rc = pv_draw(p, &in, STRIP_TOP, STRIP_BOTTOM);
        if (rc < 0)
            why = error_text(rc);
    }
    if (why)
        text_at(bmp, 1, STRIP_TOP + 3, why, 0);
}

/* ---- redrawing (task M2.3) ----
 * Stock draws a frame when its view is marked dirty (the controller's +0x20) and for reasons of
 * its own; seedr draws its whole screen over every frame stock draws. Until M2.3 gr_tick marked
 * the view on every tick, so stock drew its TRIG page and seedr its screen 20 times a second
 * while nothing changed (docs/status/M2.3.md). Now a tick marks it only when the screen may
 * differ from the last frame drawn in full: after a key or knob event or a message (scr_dirty),
 * while one of stock's overlays is up (as before), or when an input of the screen differs
 * (struct scr_key: what changes outside seedr's events, such as the current track or pattern,
 * its steps and lanes as read, the pattern's trigs, Euclidean mode, a message running out; and,
 * to be safe, the settings seedr's events change). A frame stock draws when nothing differs gets
 * a copy of the last one. The screen is the same as before; only frames that would repeat the
 * last are not drawn again. */
struct scr_key {
    int n, rc, len, euclid, msg_on, page, lane, cursor, auditioning, aud_track;
    unsigned pat, rset;
    const char *msg;
    struct seed_recipe r;
    struct seed_shared shared;
    struct seed_tool tool;
    struct seed_track shown;
    struct seed_ctx ctx;
};
#ifdef SD_FRAME_HEAP
struct scr_bufs {
    struct scr_key now, drawn;          /* zeroed when attached, so their unused bytes stay equal */
    uint8_t frame[1024];
};
static struct scr_bufs *scr_bufs;
#define scr_now (scr_bufs->now)
#define scr_drawn (scr_bufs->drawn)
#define scr_frame (scr_bufs->frame)
#else
static struct scr_key scr_now, scr_drawn;   /* static: zeroed once, so their unused bytes stay equal */
static uint8_t scr_frame[1024];         /* the last frame drawn in full */
#endif
static int scr_have;                    /* scr_frame and scr_drawn hold a frame */
unsigned scr_draws, scr_full;           /* frames seedr drew, and those drawn in full (tools/tests/42-perf.sh) */
unsigned scr_always;                    /* tests: 1 marks every tick and draws every frame in full, as before M2.3 */

/* the settings the screen shows that seedr's own events change */
static void scr_state(struct scr_key *k)
{
    int t = clampi(k->n, 0, SEED_TRACKS - 1);
    k->rset = recipe_set;
    k->msg = msg;
    k->msg_on = msg && (int)(msg_until - ticks) > 0;
    k->page = page;
    k->lane = lane;
    k->cursor = cursor;
    k->auditioning = auditioning;
    k->aud_track = aud_track;
    k->r = recipe[t];
    k->shared = shared;
    k->tool = tool;
}

/* everything else the screen shows: the track and pattern, the track as gr_draw reads it, the
 * pattern's trigs as the preview's context has them */
static void scr_inputs(struct scr_key *k)
{
    int16_t parm[SEED_LANES];
    int t;
    k->n = sd_current(&k->pat);
    t = clampi(k->n, 0, SEED_TRACKS - 1);
    scr_state(k);
    k->len = sd_track_length(k->n);
    k->euclid = sd_euclid();
    lane_parms(recipe_set & (1u << t) ? &recipe[t] : 0, parm);
    k->rc = dev->read(k->n, parm, &k->shown);
    dev->context(&k->ctx, k->n);
}

static int scr_same(const struct scr_key *a, const struct scr_key *b)
{
    const uint16_t *x = (const uint16_t *)a, *y = (const uint16_t *)b;
    unsigned i;
    for (i = 0; i < sizeof *a / 2; i++)
        if (x[i] != y[i])
            return 0;
    return 1;
}

/* gr_tick: whether the screen needs a frame */
static int scr_changed(void)
{
    if (scr_always || scr_dirty || !scr_have || sd_stock_overlay())
        return 1;
    scr_inputs(&scr_now);
    return !scr_same(&scr_now, &scr_drawn);
}

static void draw_all(void *bmp);

void gr_draw(void *bmp, void *ctrl)
{
    uint8_t *p;
    int i;
    (void)ctrl;
    if (!open || sd_stock_overlay())
        return;
    scr_draws++;
    p = px(bmp);
    if (!scr_always) {
        scr_inputs(&scr_now);
        if (!scr_dirty && scr_have && scr_same(&scr_now, &scr_drawn)) {
            for (i = 0; i < 1024; i++)
                p[i] = scr_frame[i];
            return;
        }
    }
    scr_dirty = 0;
    scr_full++;
    draw_all(bmp);
    for (i = 0; i < 1024; i++)
        scr_frame[i] = p[i];
    scr_state(&scr_now);                /* as the draw left them (it may change the page, lane, recipe) */
    scr_drawn = scr_now;
    scr_have = !scr_always;
}

/* the whole screen */
static void draw_all(void *bmp)
{
    struct pg_ctx c;
    uint8_t *p;
    int16_t parm[SEED_LANES];
    int n, i, ok;
    n = dev->current();
    page_ctx(&c, n);
    if (!pg_page_shown(&c, page))
        page = pg_page_step(&c, page, 1);
    if (lane >= dev->caps->lanes)
        lane = c.lane = 0;
    lane_parms(c.r, parm);
    ok = dev->read(n, parm, &f_shown) == SEED_OK;
    p = px(bmp);
    for (i = 0; i < 1024; i++)
        p[i] = 0;
    draw_title(bmp, p, &c, n);
    if (page == PG_VIEW) {
        if (ok)
            view_draw(bmp, p, &f_shown, &cursor, dev->caps, parm);
        draw_footer(bmp, p);                    /* VIEW draws the steps itself: no strip */
    } else {
        draw_cells(bmp, &c);
        if (ok)
            draw_strip(bmp, p, n, c.r);
    }
}

#ifdef SD_FRAME_HEAP
/* the buffers' size, and their memory (frame_heap_size() bytes, aligned to 4), zeroed here */
static unsigned frame_heap_size(void)
{
    return sizeof(struct frame_bufs) + sizeof(struct scr_bufs);
}

static void frame_heap_attach(void *m)
{
    uint8_t *b = m;
    unsigned i;
    for (i = 0; i < frame_heap_size(); i++)
        b[i] = 0;
    frame_bufs = (struct frame_bufs *)m;
    scr_bufs = (struct scr_bufs *)(b + sizeof(struct frame_bufs));
}
#endif
