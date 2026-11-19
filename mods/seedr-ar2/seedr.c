/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seedr (trig-seedr) on the Analog Rytm MKII (OS 1.74): the device part of the frame
 * (frame/frame.h, task S1.1 in docs/plan-syntakt.md). The keys, pages, generate and apply and the
 * screen are the frame's (frame/frame.c, which this file includes at its end); this file gives the
 * Rytm's side: its keys and knobs, tracks, the TRIG page's sub-pages, the core's text, and the
 * device layer (device/rytm_ar2.h), whose writes go through the stock step routines.
 */
#include <stddef.h>
#include <stdint.h>
#include "analog-rytm-mk2/screen.h"
#include "analog-rytm-mk2/store.h"
#include "analog-rytm-mk2/load.h"
#include "../../device/rytm_ar2.h"
#include "../../frame/frame.h"
/* the groove tables: tools/groove2h.py. grooves-live.h (from your own Ableton Live grooves) is
 * kept out of the repository; without it the swing-only grooves-swing.h is used */
#if __has_include("grooves-live.h")
#include "grooves-live.h"
#else
#include "grooves-swing.h"
#endif

#define SD_TRACKS 13                    /* 12 voices and FX */
#define SD_FX_TRACK 12
#define SD_TICK_HZ 20                   /* ev_tick */

/* keys (ev_key: the code at ev+12, bit 0 of ev+16 set on a press) */
#define SD_K_FUNC 0x80
#define SD_K_TRIG 0x30
#define SD_K_PAGE 0x84
#define SD_K_LEFT 0x40
#define SD_K_RIGHT 0x41
#define SD_K_UP 0x42
#define SD_K_DOWN 0x43
#define SD_K_YES 0x44
#define SD_K_NO 0x45
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))
#define SD_KEY(ev) EV_INT(ev, 12)
#define SD_DOWN(ev) (EV_INT(ev, 16) & 1)
/* keys that leave the TRIG page in stock: the parameter pages, FX, the menu keys */
static const uint8_t sd_leave_keys[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x10, 0xb0, 0xb5, 0x11, 0x12, 0x13, 0x14 };

/* The knobs as stock takes them (analog-emu's measurements, OS 1.73): ev_enc brings raw counts
 * (knob 0..7 = A..H at ev+12, not 1..8 as on the Digitone, the count at ev+16); the first two turn
 * events of a knob asleep only wake it; then 4 counts make one value step. */
#define SD_ENC(ev) EV_INT(ev, 12)
#define SD_TURN(ev) EV_INT(ev, 16)
#define SD_KNOB_COUNTS 4
#define SD_KNOB_WAKE 2

#define RT_SHIFT ((void (*)(unsigned, int, int))0x400c1d7a)   /* (track, +-1, length or -1) */

static const struct seed_dev *sd_device(void)
{
    return rt_seed_device();
}

static int sd_current(unsigned *pat)
{
    unsigned obj;
    return rt_current(&obj, pat);
}

static int sd_track_length(int t)
{
    return RT_TRACK_LENGTH(rt_seed_obj(t));
}

static uint16_t sd_track_fields(int t)
{
    return (uint16_t)rt_seed_fields(t);
}

static const char *const names[SD_TRACKS] = {
    "BD", "SD", "RS", "CP", "BT", "LT", "MT", "HT", "CH", "OH", "CY", "CB", "FX"
};
static const char *sd_track_name(int t)
{
    return t >= 0 && t < SD_TRACKS ? names[t] : "?";
}

/* what a Rytm track plays, for YOLO's ranges (device/rytm_params.h) */
static int sd_track_role(int t)
{
    static const int8_t role[SD_TRACKS] = RT_TRACK_ROLES;
    return t >= 0 && t < SD_TRACKS ? role[t] : SEED_ROLE_METER;
}

static int sd_grid_recording(void)
{
    return rt_grid_recording();
}

static int sd_euclid(void)
{
    unsigned obj;
    rt_current(&obj, 0);
    return rt_euclid(obj);
}

static int sd_rotate(int t, int steps)
{
    unsigned pat, obj;
    int len, k, i;
    rt_current(&obj, &pat);
    obj = RT_TRACK_OBJ(pat, t);
    len = RT_TRACK_LENGTH(obj);
    if (rt_euclid(obj) || len < 1 || len > 64)
        return 0;
    k = ((steps % len) + len) % len;
    if (k > len / 2)
        for (i = 0; i < len - k; i++)
            RT_SHIFT(obj, -1, -1);
    else
        for (i = 0; i < k; i++)
            RT_SHIFT(obj, 1, -1);
    rt_changed(obj);
    return 1;
}

static void sd_edited(void)
{
    core_project_edited();
}

/* Stock's TRIG page view (ParametersSeqNoteView, 1.74's vtable 0x401e3148) keeps its sub-page at +0x8c:
 * 0 TRIG, 1 EUCLIDEAN (TRIG cycles it, 0x4003ab90; a TRIG from another page starts at 0). It is on
 * the view controller's list while the TRIG page shows: +28 the first node, +20 the end; a node's +0
 * the next, +8 its view (elekloader core-ar2 screen.c, sidecar-mods 0026). -1: not showing. */
#define VT_TRIG_VIEW 0x401e3148u
static int sd_trig_subpage(void *ctrl)
{
    const uint8_t *c = (const uint8_t *)ctrl;
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

static void sd_request_frame(void *ctrl)
{
    *((volatile uint8_t *)ctrl + 0x20) = 1;
}

static int sd_stock_overlay(void)
{
    return core_stock_overlay();
}

static uint8_t *sd_pixels(void *bmp)
{
    return *(uint8_t **)((uint8_t *)bmp + 16);
}

/* the frame's SF_* are the core's values */
#if SF_LEFT != CORE_ALIGN_LEFT || SF_RIGHT != CORE_ALIGN_RIGHT || SF_INVERT != CORE_TEXT_INVERT
#error "SF_* (frame/preview.h) differ from the core's CORE_ALIGN_* / CORE_TEXT_INVERT"
#endif

static void sd_text_draw(void *bmp, int x, int y, int align, const char *s)
{
    core_text_draw(bmp, CORE_FONT_MAIN, x, y, align, s);
}

static int sd_text_width(const char *s)
{
    return core_text_width(CORE_FONT_MAIN, s);
}

static int sd_font_height(void)
{
    return core_font_height(CORE_FONT_MAIN);
}

#include "../../frame/frame.c"
