/* SPDX-License-Identifier: GPL-2.0-or-later */
/* livetest: a test mod for TURING's live page (docs/plan-turing.md task T0a), built by
 * tools/tests/45-live.sh. Never released. While the pattern plays it rewrites, from the UI task,
 * the steps of one track the playhead has passed, and checks in the sequencer's interrupt that
 * every step the clock makes plays what the track holds at that moment.
 *
 * TEMPO (swallowed) runs lt.cmd with lt.arg:
 *   1  set up track arg (0: BD): a start pattern through the device layer (trigs on steps 0, 4, 8,
 *      12, a lane-1 lock on step 4)
 *   2  start: snapshot track arg, then every step the playhead has passed is rewritten: its trig
 *      flipped and its velocity set to 20 + the number of its rewrites (mod 100)
 *   3  stop and restore the snapshot (NO)
 *   4  stop and keep (FUNC + YES)
 *   5  rewrite step arg once, on its own (the cost measurement breaks on lt_rewrite_one)
 *
 * Interrupt side (ev_seq_step): only counters and reads, no device calls. Tick side (ev_tick):
 * the playhead polled with core_track_step, the passed steps rewritten in order. */
#include <stdint.h>
#include "../../device/rytm_ar2.h"
#include "analog-rytm-mk2/seq.h"

#define K_TEMPO 0x83
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))

struct lt {
    int32_t marker[2];
    int32_t cmd, arg, res;
    int32_t live, track;
    /* the interrupt: ev_seq_step calls for the track, the first-step calls, steps it checked,
     * those whose trig matched the record, mismatches, matches on rewritten steps, polled step
     * at the last call */
    int32_t irq_calls, irq_first, irq_checked, irq_match, irq_miss, irq_match_rw, irq_step;
    /* the tick: calls while live, polled-step changes, the largest batch of passed steps in one
     * tick, rewrites, refused writes, loops seen, the last polled step, the write path's result */
    int32_t ticks, polls, max_batch, rewrites, refused, loops, last, wres;
    int32_t steps_disagree;             /* ticks where the polled step and the interrupt's differ by > 1 */
    uint32_t rec;                       /* the track's record, for the interrupt's reads */
    int32_t cnt[64];                    /* rewrites per step */
    /* DMA timer 0's counter (DTCN0, which core-ar2 starts at boot): at the last tick, and per
     * step rewrite in the tick (the last, the largest, the sum and the count) */
    uint32_t dt_now, dt_last, dt_max, dt_sum, dt_n;
};
struct lt lt = { { 0x5eed0a45, 0x7e570a45 }, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1,
                 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, { 0 }, 0, 0, 0, 0, 0 };
#define DTCN0 (*(volatile uint32_t *)0xfc07000c)

static struct seed_track lt_t;

static int16_t *lt_parm(int16_t *p)
{
    p[0] = rt_seed_device()->caps->lane_parm[0];
    p[1] = rt_seed_device()->caps->lane_parm[1];
    return p;
}

/* one step's rewrite through the device layer: read, change step s, rt_seed_write_one (only s is
 * written, with its locks from the snapshot, and one change notice). noinline: the
 * cost measurement breaks on its entry and on lt_rewrite_done. */
__attribute__((noinline)) void lt_rewrite_done(int r) { lt.wres = r; }

__attribute__((noinline)) int lt_rewrite_one(int track, int s)
{
    const struct seed_dev *d = rt_seed_device();
    int16_t parm[SEED_LANES];
    int r = d->read(track, lt_parm(parm), &lt_t);
    if (r == SEED_OK && s >= 0 && s < lt_t.length) {
        lt_t.s[s].flags ^= SEED_TRIG;
        lt.cnt[s]++;
        lt_t.s[s].vel = (uint8_t)(20 + lt.cnt[s] % 100);
        r = rt_seed_write_one(track, parm, &lt_t, s);
    }
    lt_rewrite_done(r);
    return r;
}

void lt_seq_step(uint32_t steps, uint32_t trigs)
{
    int t = lt.track, len, made;
    const uint8_t *r = (const uint8_t *)lt.rec;
    uint16_t w;
    if (!(steps & (1u << t)))
        return;
    lt.irq_calls++;
    len = core_track_length(t);
    if (steps & CORE_SEQ_FIRST) {
        lt.irq_first++;
        made = 0;
    } else {
        lt.irq_step = core_track_step(t);
        made = len > 0 ? (lt.irq_step + 1) % len : 0;
    }
    if (!lt.live || !r)
        return;
    lt.irq_checked++;
    w = (uint16_t)(r[2 * made] << 8 | r[2 * made + 1]);
    if (((w & 1) != 0) == ((trigs >> t) & 1)) {
        lt.irq_match++;
        if (lt.cnt[made])
            lt.irq_match_rw++;
    } else {
        lt.irq_miss++;
    }
}

void lt_tick(void *ctrl)
{
    int t = lt.track, cur, len, k, n;
    (void)ctrl;
    lt.dt_now = DTCN0;
    if (!lt.live || !core_transport())
        return;
    lt.ticks++;
    cur = rt_playhead(t, &len);
    if (cur < 0)
        return;
    if (lt.last < 0) {
        lt.last = cur;
        return;
    }
    if (lt.irq_step >= 0) {
        int dd = (cur - lt.irq_step + len) % len;
        if (dd > 1 && dd < len - 1)
            lt.steps_disagree++;
    }
    if (cur == lt.last)
        return;
    lt.polls++;
    n = (cur - lt.last + len) % len;      /* steps passed since the last tick: last .. cur - 1 */
    if (n > lt.max_batch)
        lt.max_batch = n;
    if (cur < lt.last)
        lt.loops++;
    for (k = 0; k < n; k++) {
        int s = (lt.last + k) % len;
        uint32_t t0 = DTCN0, dt;
        int r = lt_rewrite_one(t, s);
        dt = DTCN0 - t0;
        lt.dt_last = dt;
        if (dt > lt.dt_max)
            lt.dt_max = dt;
        lt.dt_sum += dt;
        lt.dt_n++;
        if (r == SEED_OK)
            lt.rewrites++;
        else
            lt.refused++;
    }
    lt.last = cur;
}

int lt_key(void *brain, void *ev)
{
    const struct seed_dev *d = rt_seed_device();
    int k = EV_INT(ev, 12), down = EV_INT(ev, 16) & 1, i;
    int16_t parm[SEED_LANES];
    (void)brain;
    if (k != K_TEMPO)
        return 0;
    if (!down)
        return 1;
    switch (lt.cmd) {
    case 1:
        lt.track = lt.arg;
        if ((lt.res = d->read(lt.track, lt_parm(parm), &lt_t)) == SEED_OK) {
            for (i = 0; i < lt_t.length; i++) {
                lt_t.s[i].flags &= (uint16_t)~SEED_TRIG;
                if (i % 4 == 0)
                    lt_t.s[i].flags |= SEED_TRIG;
                lt_t.s[i].lane[0] = SEED_NO_LOCK;
            }
            lt_t.s[4].lane[0] = 5;
            lt.res = d->write(lt.track, parm, &lt_t);
        }
        break;
    case 2:
        lt.track = lt.arg;
        for (i = 0; i < 64; i++)
            lt.cnt[i] = 0;
        lt.irq_checked = lt.irq_match = lt.irq_miss = lt.irq_match_rw = 0;
        lt.ticks = lt.polls = lt.max_batch = lt.rewrites = lt.refused = lt.loops = 0;
        lt.steps_disagree = 0;
        lt.dt_max = lt.dt_sum = lt.dt_n = 0;
        lt.last = -1;
        lt.rec = (uint32_t)rt_record(rt_seed_obj(lt.track));
        lt.res = d->snapshot((uint16_t)(1u << lt.track));
        lt.live = lt.res == SEED_OK;
        break;
    case 3:
        lt.live = 0;
        d->restore();
        d->drop();
        lt.res = SEED_OK;
        break;
    case 4:
        lt.live = 0;
        d->drop();
        lt.res = SEED_OK;
        break;
    case 5:
        lt.res = lt_rewrite_one(lt.track, lt.arg);
        break;
    default:
        lt.res = SEED_E_ARG;
    }
    return 1;
}
