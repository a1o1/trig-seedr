/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The preview strip (preview.c) and VIEW (view.c): task F10 (docs/plan-v1.md section 6), called
 * from the frame's gr_draw (frame.c). The pixel helpers are the ones the frame uses, for the core's bitmap. */
#ifndef SEEDR_PREVIEW_H
#define SEEDR_PREVIEW_H

#include <stdint.h>
#include "../lib/seed.h"

/* What a YES on this track would render (frame.c take()): the recipe from the original. */
struct pv_in {
    const struct seed_dev *device;    /* for the context (the pattern's trigs, for MASK and COPY) */
    int track;
    const struct seed_recipe *r;
    const struct seed_shared *shared;
    const struct seed_groove *grooves;
    int ngrooves;
    const struct seed_track *orig;    /* the original: the audition's snapshot of the track, or the track as it is */
};

/* The preview, rows top..bottom: the take as YES would write it, 2 pixels a step, bar height =
 * velocity, a dot on every 4th empty step. Renders only when an input differs from the last
 * render's (the recipe, the shared settings, the original, the pattern's trigs); otherwise it
 * draws the last one. SEED_OK, or seed_render's error, with nothing drawn. */
int pv_draw(uint8_t *p, const struct pv_in *in, int top, int bottom);

/* VIEW, rows 12..46: the 16 steps around the cursor and the cursor step's fields, read-only.
 * parm: the recipe's lane parameters (the lanes t was read with). */
void view_draw(void *bmp, uint8_t *p, const struct seed_track *t, int *cursor, const struct seed_caps *caps,
               const int16_t *parm);

/* ---- pixels: rows count from the top, 0..63 ---- */

static inline void pv_dot(uint8_t *p, int x, int r)
{
    if (x >= 0 && x < 128 && r >= 0 && r < 64)
        p[8 * x + 7 - r / 8] |= (uint8_t)(1 << (r % 8));
}

static inline void pv_box(uint8_t *p, int x0, int r0, int x1, int r1, int fill)
{
    int x, r;
    for (x = x0; x <= x1; x++)
        for (r = r0; r <= r1; r++)
            if (fill || x == x0 || x == x1 || r == r0 || r == r1)
                pv_dot(p, x, r);
}

/* ---- text, through the device (frame.c, frame.h): the main font ---- */

#define SF_LEFT   0                     /* x is where the text starts */
#define SF_RIGHT  2                     /* x is where it ends */
#define SF_INVERT 0x10                  /* the text's box inverted */

/* text whose top is on row `top`; align SF_LEFT or SF_RIGHT, | SF_INVERT */
void sf_text(void *bmp, int x, int top, const char *s, int align);
/* the text's width in pixels */
int sf_text_width(const char *s);

static inline void pv_text(void *bmp, int x, int top, const char *s, int align)
{
    sf_text(bmp, x, top, s, align);
}

#endif
