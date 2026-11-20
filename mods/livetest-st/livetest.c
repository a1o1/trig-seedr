/* SPDX-License-Identifier: GPL-2.0-or-later */
/* livetest-st: writing behind the Syntakt's playhead while the pattern plays (docs/plan-turing.md task T0b), for
 * tools/tests-st/45-live.sh. A test build only. The script pokes a command into `lt`; the next ev_tick (the UI
 * task) runs it, sets lt.res and clears lt.cmd. While live, every tick reads the track's playhead (st_playhead)
 * and writes the steps it passed since the last read (st_steps_passed, st_seed_write_steps) from the target.
 *
 *   1  attach: the layer's memory from the native heap
 *   2  live on the current track: snapshot it, the target is the original with a trig exactly on the steps
 *      lt.arg names (bit i: step i); lt.track, lt.len
 *   3  live off
 *   4  restore (while the pattern plays, too)
 *   5  drop
 *   6  the cost: lt.arg times, every step of the track written, alternately the target and its inverse (so
 *      each write changes every step); the script times the tick with dtboot --watch on lt.ticks and lt.cmd
 *   7  the same loop without the writes (the baseline) */
#include <stdint.h>
#include "../../device/syntakt_st.h"

#define FW_MALLOC ((void *(*)(uint32_t))0x401036d0)    /* native allocator, task context (core-st fw.s) */

struct livetest {
    uint32_t magic;                    /* 'LVST' once the first tick ran */
    int32_t cmd, arg, res;
    uint32_t mem, ticks, track, len;
    uint32_t live, polls, writes, steps, maxper, stopped;  /* steps: steps written; maxper: most in one tick */
    int32_t prev, now;
};
struct livetest lt;
static struct seed_track tgt;
static const int16_t lanes[SEED_LANES] = { -1, -1 };
static uint8_t mask[SEED_STEPS / 8];

static struct seed_track inv;

/* a struct copy without memcpy (the build links no C library) */
static void copy(void *to, const void *from, unsigned n)
{
    uint8_t *d = to;
    const uint8_t *s = from;
    while (n--)
        *d++ = *s++;
}

static int start(void)
{
    const struct seed_dev *d = st_seed_device();
    int n = d->current(), i, rc;
    lt.track = (uint32_t)n;
    if ((rc = d->snapshot((uint16_t)(1u << n))) < 0 || (rc = d->read(n, lanes, &tgt)) < 0)
        return rc;
    lt.len = (uint32_t)tgt.length;
    for (i = 0; i < tgt.length; i++) {
        if (lt.arg >> i & 1)
            tgt.s[i].flags |= SEED_TRIG;
        else
            tgt.s[i].flags &= (uint16_t)~SEED_TRIG;
    }
    lt.prev = -1;
    lt.live = 1;
    return SEED_OK;
}

static void live(void)
{
    int now = st_playhead((int)lt.track), n, i, c;
    lt.polls++;
    lt.now = now;
    if (now < 0) {
        lt.stopped++;
        lt.prev = -1;
        return;
    }
    n = st_steps_passed(lt.prev, now, (int)lt.len, mask);
    lt.prev = now;
    if (!n)
        return;
    if ((uint32_t)n > lt.maxper)
        lt.maxper = (uint32_t)n;
    if (st_seed_write_steps((int)lt.track, lanes, &tgt, mask) == SEED_OK) {
        lt.writes++;
        for (i = 0, c = 0; i < (int)lt.len; i++)
            c += mask[i >> 3] >> (i & 7) & 1;
        lt.steps += (uint32_t)c;
    }
}

void livetest_tick(void *ctrl)
{
    const struct seed_dev *d = st_seed_device();
    void *m;
    int i;
    (void)ctrl;
    lt.ticks++;
    lt.magic = 0x4c565354u;
    if (lt.live)
        live();
    switch (lt.cmd) {
    case 0:
        return;
    case 1:
        m = FW_MALLOC(ST_SEED_STATE_SIZE);
        lt.mem = (uint32_t)m;
        lt.res = (uint32_t)m >= 0x40000000u && st_seed_attach(m, ST_SEED_STATE_SIZE) ? 0 : -1;
        break;
    case 2:
        lt.res = start();
        break;
    case 3:
        lt.live = 0;
        lt.res = 0;
        break;
    case 4:
        d->restore();
        lt.res = 0;
        break;
    case 5:
        d->drop();
        lt.res = 0;
        break;
    case 6:
    case 7:
        for (i = 0; i < SEED_STEPS / 8; i++)
            mask[i] = 0xff;
        copy(&inv, &tgt, sizeof inv);
        for (i = 0; i < inv.length; i++)
            inv.s[i].flags ^= SEED_TRIG;
        lt.res = 0;
        for (i = 0; i < lt.arg; i++)
            if (lt.cmd == 6)
                lt.res |= st_seed_write_steps((int)lt.track, lanes, i & 1 ? &inv : &tgt, mask);
            else
                lt.res |= st_steps_passed(-1, i % (int)lt.len, (int)lt.len, mask) - 1;
        break;
    default:
        lt.res = -9;
    }
    lt.cmd = 0;
}
