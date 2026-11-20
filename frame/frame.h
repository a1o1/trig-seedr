/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The frame (frame.c) and the device part it needs (docs/plan-syntakt.md, task S1.1).
 *
 * The frame is everything seedr does that does not depend on the instrument: the keys and knobs,
 * the pages, the recipe store, generate and apply (the audition), YOLO and ROTATE ALL, the screen.
 * A device's mod has one source file, mods/seedr-<device>/seedr.c, that includes this header,
 * defines what it lists below, and then includes frame.c, so the two are one translation unit and
 * the device's inline functions (device/rytm_ar2.h's) stay inline. pages.c, preview.c and view.c
 * are compiled on their own; they reach the device only through sf_text and sf_text_width
 * (preview.h), which frame.c defines.
 *
 * The device part defines, before including frame.c:
 *
 *   the groove tables      grooves[] and N_GROOVES (tools/groove2h.py)
 *   SD_TRACKS              tracks per pattern, at most SEED_TRACKS
 *   SD_FX_TRACK            the track seedr does not write (NOT ON THE FX TRACK), or -1
 *   SD_TICK_HZ             ev_tick's rate (messages last 2 s, knobs sleep after 2 s)
 *   SD_K_FUNC .. SD_K_NO   the key codes of ev_key: FUNC, TRIG, PAGE, LEFT, RIGHT, UP, DOWN, YES, NO
 *   SD_KEY(ev), SD_DOWN(ev)   the key code of an ev_key event, and whether it is a press
 *   SD_ENC(ev), SD_TURN(ev)   an ev_enc event's knob (0..7 = A..H) and its signed count
 *   SD_KNOB_COUNTS         counts of a turn that make one value step
 *   SD_KNOB_WAKE           turn events that only wake a knob asleep
 *   sd_leave_keys[]        keys that leave stock's TRIG page: seedr closes on them
 *   SD_LIVE                (optional) the device gives the playhead: TURING's live page (0.9, T3), with
 *                          sd_playhead and sd_write_one below; without it the page is not shown
 *
 * and the functions declared below (static: one translation unit).
 */
#ifndef SEEDR_FRAME_H
#define SEEDR_FRAME_H

#include <stdint.h>
#include "../lib/seed.h"
#include "pages.h"
#include "preview.h"

/* the device layer (design section 10) */
static const struct seed_dev *sd_device(void);
/* the current track (0..SD_TRACKS - 1); *pat, when pat is not null: the pattern in play, as a
 * number that changes when the pattern does */
static int sd_current(unsigned *pat);
/* track t's length in steps, as stock has it (not clamped) */
static int sd_track_length(int t);
/* the SEED_F_* fields track t has */
static uint16_t sd_track_fields(int t);
/* track t's name for the title bar ("BD"); "?" for none */
static const char *sd_track_name(int t);
/* what track t plays, for YOLO's ranges (SEED_ROLE_*) */
static int sd_track_role(int t);
/* 1 while grid recording is on (generating needs it) */
static int sd_grid_recording(void);
/* 1 while the current track is in stock's Euclidean mode (seedr does not write it) */
static int sd_euclid(void);
/* ROTATE ALL on track t of the pattern in play: its whole steps by `steps`, through stock's own
 * shift, so locks move with them; 1 when it was rotated, 0 when it is skipped (Euclidean mode, a
 * length outside 1..64) */
static int sd_rotate(int t, int steps);
/* the project has changed (stock's modified mark) */
static void sd_edited(void);
/* stock's sub-page of the TRIG page in view (0 the first, 1 the second), or -1 when it is not in
 * view; ctrl: the view controller ev_tick brings */
static int sd_trig_subpage(void *ctrl);
/* ask stock for a frame (from ev_tick, with its ctrl) */
static void sd_request_frame(void *ctrl);
/* 1 while one of stock's overlays (a popup, a menu) is over the screen: seedr does not draw */
static int sd_stock_overlay(void);
/* the bitmap's pixels: 128 x 64, 8 bytes a column, pv_dot's layout */
static uint8_t *sd_pixels(void *bmp);
/* the main font: text at x, y (y from the bottom, as the core takes it), align SF_* (preview.h) */
static void sd_text_draw(void *bmp, int x, int y, int align, const char *s);
static int sd_text_width(const char *s);
static int sd_font_height(void);
#ifdef SD_LIVE
/* the step track t plays now, 0 .. *len - 1, and the length it wraps at; -1 while stopped. Steps
 * before it in the loop have played and may be rewritten (docs/status/T0a.md) */
static int sd_playhead(int t, int *len);
/* write step i of take on track t alone, as the device's write writes it (the audition's snapshot
 * is the source of its locks); every other step is left alone */
static int sd_write_one(int t, const int16_t *parm, const struct seed_track *take, int i);
#endif

#endif
