/* SPDX-License-Identifier: GPL-2.0-or-later */
/* devtest-st: the Syntakt device layer (device/syntakt_st.h) on the stock OS 1.42, for tools/st_s2.sh
 * (docs/plan-syntakt.md task S2). A test build only. tools/st_s2.sh pokes a command into `dt` with
 * dtboot --poke; the next ev_tick (the UI task) runs it, sets dt.res and clears dt.cmd.
 *
 *   1  attach: the layer's memory from the native heap (ST_SEED_STATE_SIZE bytes)
 *   2  snapshot the current track
 *   3  write the known take below on the current track; dt.res the result, dt.arg the steps changed
 *   4  restore
 *   5  drop
 *   6  read the current track: dt.res the result, dt.arg its length
 *
 * The known take, from the track as read: every step moved one later (origins i - 1 within the
 * length); step 2 NOTE 64, VEL 90, micro timing +5, condition 1:2; lane 1 (the filter's FRQ, list
 * index 4 while the machine is not known) 100 on step 1 and none on step 4. */
#include <stdint.h>
#include "../../device/syntakt_st.h"

#define FW_MALLOC ((void *(*)(uint32_t))0x401036d0)    /* native allocator, task context (core-st fw.s) */

struct devtest {
    uint32_t magic;                    /* 'DTST' once the first tick ran */
    int32_t cmd, arg, res;
    uint32_t mem, ticks, track, lane_slot;
};
struct devtest dt;
static struct seed_track t;
static struct seed_track a;
static const int16_t lanes[SEED_LANES] = { 4, -1 };

/* a struct copy without memcpy (the build links no C library) */
static void copy(void *to, const void *from, unsigned n)
{
    uint8_t *d = to;
    const uint8_t *s = from;
    while (n--)
        *d++ = *s++;
}

static int take(void)
{
    const struct seed_dev *d = st_seed_device();
    int n = d->current(), i, rc;
    dt.track = (uint32_t)n;
    dt.lane_slot = (uint32_t)st_seed_lane_k(n, lanes, 0);
    if ((rc = d->read(n, lanes, &t)) < 0)
        return rc;
    copy(&a, &t, sizeof a);
    for (i = 0; i < t.length; i++)
        copy(&t.s[i], &a.s[(i + t.length - 1) % t.length], sizeof t.s[i]);
    t.s[2].note = 64;
    t.s[2].vel = 90;
    t.s[2].utime = 5;
    t.s[2].cond = 30;
    t.s[1].lane[0] = 100;
    t.s[4].lane[0] = SEED_NO_LOCK;
    rc = d->write(n, lanes, &t);
    dt.arg = st_seed_written();
    return rc;
}

void devtest_tick(void *ctrl)
{
    const struct seed_dev *d = st_seed_device();
    void *m;
    (void)ctrl;
    dt.ticks++;
    dt.magic = 0x44545354u;
    switch (dt.cmd) {
    case 0:
        return;
    case 1:
        m = FW_MALLOC(ST_SEED_STATE_SIZE);
        dt.mem = (uint32_t)m;
        dt.res = (uint32_t)m >= 0x40000000u && st_seed_attach(m, ST_SEED_STATE_SIZE) ? 0 : -1;
        break;
    case 2:
        dt.res = d->snapshot((uint16_t)(1u << d->current()));
        break;
    case 3:
        dt.res = take();
        break;
    case 4:
        d->restore();
        dt.res = 0;
        break;
    case 5:
        d->drop();
        dt.res = 0;
        break;
    case 6:
        dt.res = d->read(d->current(), lanes, &t);
        dt.arg = t.length;
        break;
    default:
        dt.res = -9;
    }
    dt.cmd = 0;
}
