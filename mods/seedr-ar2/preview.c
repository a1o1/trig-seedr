/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The preview strip (design section 8): the take as YES would write it, rendered from the recipe
 * before YES. Task F10 (docs/plan-v1.md section 6).
 *
 * Every draw gathers what a render depends on (the track, its recipe, the shared settings, the
 * original, the pattern's trigs) and compares it with what the last render used. Only a
 * difference renders again, so a knob turn, a YES or NO, a trig recorded with stock's keys, or a
 * change on a track MASK or COPY reads each renders once, and an idle screen costs a compare.
 * The render has its own buffers: the take seedr.c writes is never touched. */
#include <stdint.h>
#include "preview.h"

struct pv_key {
    int track;
    struct seed_recipe r;
    struct seed_track orig;
    struct seed_ctx ctx;
};

/* what the last render used; the context this draw would use. Static, so zeroed once: their
 * padding (if any) stays equal and they compare bytewise. */
static struct pv_key was;
static struct seed_ctx ctx;
static int have, rc;
static struct seed_work work;
struct seed_track pv_out;             /* the last render; tools/tests/30-preview.sh reads it */
unsigned pv_renders;                  /* renders since boot (the test counts them) */

static int differ(const void *a, const void *b, unsigned n)
{
    const uint8_t *x = a, *y = b;
    while (n--)
        if (*x++ != *y++)
            return 1;
    return 0;
}

static void copy(void *to, const void *from, unsigned n)
{
    uint8_t *d = to;
    const uint8_t *s = from;
    while (n--)
        *d++ = *s++;
}

/* the context as take() makes it; the track in scope from the original (during an audition the
 * track holds the take, and YES renders from the original) */
static void context(const struct pv_in *in)
{
    int i;
    in->device->context(&ctx, in->track);
    ctx.shared = *in->shared;
    ctx.grooves = in->grooves;
    ctx.ngrooves = in->ngrooves;
    if (in->track >= 0 && in->track < ctx.tracks) {
        ctx.length[in->track] = (uint8_t)in->orig->length;
        for (i = 0; i < SEED_STEPS / 8; i++)
            ctx.trig[in->track][i] = 0;
        for (i = 0; i < in->orig->length && i < SEED_STEPS; i++)
            if (in->orig->s[i].flags & SEED_TRIG)
                ctx.trig[in->track][i >> 3] |= (uint8_t)(1 << (i & 7));
    }
}

int pv_draw(uint8_t *p, const struct pv_in *in, int top, int bottom)
{
    int i;
    context(in);
    if (!have || was.track != in->track || differ(&was.r, in->r, sizeof was.r) ||
        differ(&was.orig, in->orig, sizeof was.orig) || differ(&was.ctx, &ctx, sizeof ctx)) {
        was.track = in->track;
        copy(&was.r, in->r, sizeof was.r);
        copy(&was.orig, in->orig, sizeof was.orig);
        copy(&was.ctx, &ctx, sizeof ctx);
        rc = seed_render(&pv_out, &was.orig, &was.r, &was.ctx, &work);
        have = 1;
        pv_renders++;
    }
    if (rc < 0)
        return rc;
    for (i = 0; i < pv_out.length && i < SEED_STEPS; i++) {
        const struct seed_step *s = &pv_out.s[i];
        int x = 2 * i;
        if (s->flags & SEED_TRIG) {
            int v = s->vel == SEED_DEFAULT ? pv_out.default_vel : s->vel;
            int h = 1 + (v < 1 ? 1 : v > 127 ? 127 : v) * (bottom - top - 1) / 127;
            pv_box(p, x, bottom - h + 1, x, bottom, 1);
        } else if (!(i & 3))
            pv_dot(p, x, bottom);
    }
    return SEED_OK;
}
