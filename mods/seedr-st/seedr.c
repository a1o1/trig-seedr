/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seedr (trig-seedr) on the Syntakt (OS 1.42): the device part of the frame (frame/frame.h; task S3 in
 * docs/plan-syntakt.md), as mods/seedr-ar2/seedr.c is the Rytm's. The keys, pages, generate and apply and
 * the screen are the frame's (frame/frame.c, included at the end); this file gives the Syntakt's side:
 * its keys and knobs, tracks, the TRIG page's sub-pages, text drawn from the OS's own font, and the device
 * layer (device/syntakt_st.h), whose writes go through the stock step routines.
 *
 * It runs on the hook-bus core (tools/st_build.sh --core mini: tick, draw, key, encoder). The device
 * layer's memory (the 13 snapshots, about 140 KB) comes from the native heap at the first tick (plan
 * SD2); until then every device entry says SEED_E_NA and seedr does not open. Findings: status/S3.md.
 */
#include <stddef.h>
#include <stdint.h>
#include "../../device/syntakt_st.h"
#include "../../frame/frame.h"
/* the groove tables: tools/groove2h.py, kept with the Rytm's mod. grooves-live.h (from your own
 * Ableton Live grooves) is kept out of the repository; without it the swing-only grooves-swing.h */
#if __has_include("../seedr-ar2/grooves-live.h")
#include "../seedr-ar2/grooves-live.h"
#else
#include "../seedr-ar2/grooves-swing.h"
#endif

#define SD_TRACKS ST_TRACKS             /* 12 and the FX track */
#define SD_FX_TRACK ST_FX_TRACK
#define SD_TICK_HZ 30                   /* ev_tick: 300 in 10 s of analog-emu's clock [measured] */

/* keys (ev_key: the native KeyEvent's ID at ev+12, its flags at ev+16: press 1, repeat 9, release
 * 0x10; Synfact core-st's README, the rest measured with a probe mod) */
#define SD_K_FUNC 1
#define SD_K_TRIG 19
#define SD_K_PAGE 18
#define SD_K_LEFT 16
#define SD_K_RIGHT 17
#define SD_K_UP 14
#define SD_K_DOWN 15
#define SD_K_YES 12
#define SD_K_NO 13
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))
#define SD_KEY(ev) EV_INT(ev, 12)
#define SD_DOWN(ev) (EV_INT(ev, 16) & 1)
/* keys that leave the TRIG page in stock: MENU 5, GLOBAL 6, FX 7, SYN 20, FLTR 21, AMP 22, LFO 23 */
static const uint8_t sd_leave_keys[] = { 5, 6, 7, 20, 21, 22, 23 };

/* The knobs: ev_enc's ID is 1..8 for A..H (LEVEL 9) at ev+12 and the signed count at ev+16 (core-st's
 * README; measured). analog-emu takes the Syntakt's counts per detent as the Rytm's 8 [inferred, its
 * docs/SYNTAKT-PORT.md], so the Rytm's settings: two turns wake a knob, then 4 counts a step. */
#define SD_ENC(ev) (EV_INT(ev, 12) - 1)
#define SD_TURN(ev) EV_INT(ev, 16)
#define SD_KNOB_COUNTS 4
#define SD_KNOB_WAKE 2

#define FW_MALLOC ((void *(*)(uint32_t))0x401036d0u)   /* the native allocator, task context (core-st fw.s) */
#define SD_FRAME_HEAP                   /* the take's and the screen's buffers from the heap too */
static int st_ready;                    /* the heap's memory is in place (st_attach) */

static const struct seed_dev *sd_device(void)
{
    return st_seed_device();
}

/* the current track, and the device's caps pointed at its list (st_seed_current) */
static int sd_current(unsigned *pat)
{
    unsigned obj;
    int n = st_current(&obj, pat);
    if (st_seed_state())
        st_seed_select(n);
    return n;
}

static int sd_track_length(int t)
{
    unsigned obj = st_seed_obj(t);
    return obj ? ST_TRACK_LENGTH(obj) : 16;
}

static uint16_t sd_track_fields(int t)
{
    return (uint16_t)st_seed_fields(t);
}

static const char *const names[SD_TRACKS] = {
    "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "T9", "T10", "T11", "T12", "FX"
};
static const char *sd_track_name(int t)
{
    return t >= 0 && t < SD_TRACKS ? names[t] : "?";
}

/* What a track plays, for YOLO's ranges. The Syntakt's tracks have no fixed instrument; the machine's
 * role (device/st_params.h) once the layer knows it, else a role by the default kit's places [inferred]:
 * a kick on track 1, a snare on 2, hats on 3 and on the cymbal track. */
static int sd_track_role(int t)
{
    static const int8_t role[SD_TRACKS] = {
        SEED_ROLE_KICK, SEED_ROLE_BACK, SEED_ROLE_HAT, SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_PERC,
        SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_HAT,
        SEED_ROLE_METER
    };
    static const struct st_machine machines[ST_NMACHINES] = ST_MACHINES;
    const struct st_machine *m;
    if (t < 0 || t >= SD_TRACKS)
        return SEED_ROLE_METER;
    m = st_seed_state() && st_seed_state()->list[t].machine >= 0 ?
        st_machine_of(machines, ST_NMACHINES, st_seed_state()->list[t].machine) : 0;
    return m ? m->role : role[t];
}

static int sd_grid_recording(void)
{
    return st_grid_recording();
}

static int sd_euclid(void)
{
    unsigned obj;
    st_current(&obj, 0);
    return st_euclid(obj);
}

/* ROTATE ALL: the Syntakt's whole-step shift (with its locks) is not found yet (firmware notes,
 * section 8): every track is skipped */
static int sd_rotate(int t, int steps)
{
    (void)t;
    (void)steps;
    return 0;
}

/* the project's modified mark: not found on the Syntakt yet */
static void sd_edited(void)
{
}

/* Stock's TRIG page: the main screen's view (+0xc0) has vtable 0x401c9a9c and +0x88 = 0x13 while it
 * shows; +0x90 is the sub-page, 0 Trig Parameters, 1 Euclidean (firmware notes section 6, measured).
 * The main screen is the UI controller's (*0x444e2334) +0x40 node's +0x1c, its +8. */
#define VT_TRIG_VIEW 0x401c9a9cu
static int sd_trig_subpage(void *ctrl)
{
    const uint8_t *ui = *(const uint8_t *const *)0x444e2334u, *node, *main, *v;
    (void)ctrl;
    if (!ui || !(node = *(const uint8_t *const *)(ui + 0x40 + 0x1c)) || !(main = *(const uint8_t *const *)(node + 8)) ||
        !(v = *(const uint8_t *const *)(main + 0xc0)))
        return -1;
    if (*(const uint32_t *)v != VT_TRIG_VIEW || *(const uint32_t *)(v + 0x88) != 0x13)
        return -1;
    return *(const int32_t *)(v + 0x90);
}

static void sd_request_frame(void *ctrl)
{
    *((volatile uint8_t *)ctrl + 0x20) = 1;  /* core-st: the byte that asks for a frame */
}

/* stock's popups and menus over the screen: not found on the Syntakt yet */
static int sd_stock_overlay(void)
{
    return 0;
}

/* the bitmap (vtable, 128, 64, 2, pixels): 128 x 64, 8 bytes a column, row 0 the top in bit 0 of the
 * column's last byte, as the Rytm's [measured] */
static uint8_t *sd_pixels(void *bmp)
{
    return *(uint8_t **)((uint8_t *)bmp + 16);
}

/* Text in the OS's own 6-pixel font (the object at 0x443db220: its data at +4 holds the height, then
 * pointers to a width per character, a 16-bit offset per character and the glyphs: a byte a column, rows
 * from the top in bits 8 - height .. 7) [measured]. Drawn here, not through stock's text routine, so
 * widths are exact and SF_INVERT (the title bar) clears the text's pixels. A character's gap is 1. */
#define ST_FONT 0x443db220u
static const uint32_t *st_font(void)
{
    return (const uint32_t *)((const uint32_t *)ST_FONT)[1];
}

static int sd_font_height(void)
{
    return (int)st_font()[1];
}

static int sd_text_width(const char *s)
{
    const uint8_t *w = (const uint8_t *)st_font()[2];
    int n = 0;
    while (*s)
        n += w[(uint8_t)*s++] + 1;
    return n ? n - 1 : 0;
}

/* x the start (SF_LEFT) or end (SF_RIGHT); y the text's bottom row, counted up from the bottom */
static void sd_text_draw(void *bmp, int x, int y, int align, const char *s)
{
    const uint32_t *f = st_font();
    const uint8_t *w = (const uint8_t *)f[2], *g = (const uint8_t *)f[4];
    const uint16_t *off = (const uint16_t *)f[3];
    uint8_t *p = sd_pixels(bmp);
    int h = (int)f[1], top = 63 - y - (h - 1), inv = align & SF_INVERT, i, r;
    if (align & SF_RIGHT)
        x -= sd_text_width(s) - 1;
    for (; *s; s++) {
        uint8_t c = (uint8_t)*s;
        for (i = 0; i <= w[c]; i++, x++) {
            uint8_t col = i < w[c] ? g[off[c] + i] : 0;
            if (x < 0 || x > 127 || (i == w[c] && !s[1]))
                continue;
            for (r = 0; r < h; r++) {
                int on = (col >> (8 - h + r)) & 1, row = top + r;
                if (row < 0 || row > 63)
                    continue;
                if (on != !!inv)
                    p[8 * x + 7 - row / 8] |= (uint8_t)(1 << (row % 8));
                else if (inv)
                    p[8 * x + 7 - row / 8] &= (uint8_t)~(1 << (row % 8));
            }
        }
    }
}

#include "../../frame/frame.c"

/* The memory the 64 KiB mods' area has no room for, from the native heap at the first tick (the UI
 * task): the device layer's (its 13 snapshots) and the frame's buffers. Until then seedr stays shut. */
static void st_attach(void)
{
    uint8_t *m;
    if (st_ready)
        return;
    m = FW_MALLOC(((ST_SEED_STATE_SIZE + 3) & ~3u) + frame_heap_size());
    if ((uint32_t)m < 0x40000000u || !st_seed_attach(m, ST_SEED_STATE_SIZE))
        return;
    frame_heap_attach(m + ((ST_SEED_STATE_SIZE + 3) & ~3u));
    st_ready = 1;
}

/* the frame's events, after the device layer's memory is in place */
int st_key(void *brain, void *ev)
{
    return st_ready ? gr_key(brain, ev) : 0;
}

int st_enc(void *brain, void *ev)
{
    return st_ready ? gr_enc(brain, ev) : 0;
}

void st_tick(void *ctrl)
{
    st_attach();
    if (st_ready)
        gr_tick(ctrl);
}

void st_draw(void *bmp, void *ctrl)
{
    if (st_ready)
        gr_draw(bmp, ctrl);
}
