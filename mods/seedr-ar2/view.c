/* SPDX-License-Identifier: GPL-2.0-or-later */
/* VIEW (design section 8): the track's steps, read-only, around a cursor on knob A. Task F10
 * (docs/plan-v1.md section 6); 0.3 drew it in seedr.c. The track is shown as it is (during an
 * audition: the take); seedr.c draws the footer below it.
 *
 *   rows 12..25  the 16 steps of the cursor's page, 8 pixels a step: a trig (tall when the device's
 *                own trig bits 0x0380 are set, hollow when muted), a mark over it for a retrig
 *                (left) and a lock on either lane (right), a line under it for the accent flag,
 *                and the cursor under that
 *   row 28       S step, N note, V velocity, L length (- = the track's default), U micro timing
 *   row 38       C condition, R retrig rate and velocity, each lane's lock (its parameter's name
 *                and value) when the step has one; at the right the flag word and the track's
 *                default note and velocity, left out when the line is full */
#include <stdint.h>
#include "preview.h"
#include "pages.h"

#define DEV_TRIG_BITS 0x0380            /* the Rytm's own bits of a step with a trig */

static char *val(char *o, const char *label, int v)
{
    o = pg_put(o, label);
    return v == SEED_DEFAULT ? pg_put(o, "-") : pg_num(o, v);
}

static int has_lock(const struct seed_step *s)
{
    int j;
    for (j = 0; j < SEED_LANES; j++)
        if (s->lane[j] != SEED_NO_LOCK)
            return 1;
    return 0;
}

static void steps(uint8_t *p, const struct seed_track *t, int cursor)
{
    int i, pg = cursor / SEED_PAGE * SEED_PAGE;
    for (i = 0; i < SEED_PAGE && pg + i < t->length; i++) {
        const struct seed_step *s = &t->s[pg + i];
        int x = 2 + 8 * i, fill = !(s->flags & SEED_MUTE);
        if (s->flags & SEED_TRIG) {
            if (s->flags & DEV_TRIG_BITS)
                pv_box(p, x, 14, x + 5, 20, fill);
            else
                pv_box(p, x + 1, 15, x + 4, 19, fill);
        }
        if (s->flags & SEED_RETRIG)
            pv_box(p, x, 12, x + 1, 12, 1);
        if (has_lock(s))
            pv_box(p, x + 4, 12, x + 5, 12, 1);
        if (s->flags & SEED_ACCENT)
            pv_box(p, x, 22, x + 5, 22, 1);
        if (pg + i == cursor) {
            pv_box(p, x + 2, 24, x + 3, 24, 1);
            pv_box(p, x + 1, 25, x + 4, 25, 1);
        }
    }
}

void view_draw(void *bmp, uint8_t *p, const struct seed_track *t, int *cursor, const struct seed_caps *caps,
               const int16_t *parm)
{
    char line[48], right[24], *o, *full;
    const struct seed_step *s;
    int i, lw;
    if (*cursor >= t->length)
        *cursor = t->length - 1;
    if (*cursor < 0)
        *cursor = 0;
    steps(p, t, *cursor);
    s = &t->s[*cursor];

    o = pg_put(line, "S");
    o = pg_num(o, *cursor + 1);
    o = val(o, " N", s->note);
    o = val(o, " V", s->vel);
    o = val(o, " L", s->len);
    o = pg_put(o, " U");
    o = pg_snum(o, s->utime);
    *o = 0;
    pv_text(bmp, 2, 28, line, CORE_ALIGN_LEFT);

    o = pg_put(line, "C");
    o = pg_cond(o, s->cond);
    o = pg_put(o, " R");
    if (s->flags & SEED_RETRIG) {
        o = pg_rate(o, s->rrate);
        *o++ = ' ';
        o = pg_num(o, s->rvel);
    } else
        o = pg_put(o, "-");
    for (i = 0; i < SEED_LANES && i < caps->lanes; i++)
        if (s->lane[i] != SEED_NO_LOCK && parm[i] >= 0 && parm[i] < caps->nparams) {
            *o++ = ' ';
            o = pg_put(o, caps->params[parm[i]].name);
            o = pg_num(o, s->lane[i]);
        }
    *o = 0;
    pv_text(bmp, 2, 38, line, CORE_ALIGN_LEFT);

    /* the flag word, then the defaults; the defaults go first when the line is full, then the flags */
    lw = 2 + core_text_width(CORE_FONT_MAIN, line) + 6;
    o = pg_put(right, "F");
    for (i = 12; i >= 0; i -= 4)
        *o++ = "0123456789ABCDEF"[(s->flags >> i) & 15];
    full = o;
    o = pg_put(o, " DN");
    o = pg_num(o, t->default_note);
    o = pg_put(o, " DV");
    o = pg_num(o, t->default_vel);
    *o = 0;
    if (lw + core_text_width(CORE_FONT_MAIN, right) > 127)
        *full = 0;
    if (lw + core_text_width(CORE_FONT_MAIN, right) <= 127)
        pv_text(bmp, 126, 38, right, CORE_ALIGN_RIGHT);
}
