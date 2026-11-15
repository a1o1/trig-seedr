/* SPDX-License-Identifier: GPL-2.0-or-later */
/* lrtest: a test mod for the pattern's stored lock rows (task D1, docs/status/D1.md), built by
 * tools/tests/12-lock-rows.sh into build/. Never released. TEMPO (swallowed) runs the command in
 * dt.cmd with dt.arg. dt.res is its result (1 while a command that runs over ticks is not done);
 * dt.used, the rows in use, is updated every tick.
 *   1  measure: for each sound index 0..41 a lock through stock's lock set on step 0 of BD (fine
 *      flag 0), MT (fine 1) and FX (fine 0), then cleared; dt.map records the rows stock made
 *   2  the same on FX with fine 1
 *   3  fill: coarse locks on step 0 of MT .. CB (sound indexes 17..41) until arg rows are used
 *   4  write BD through the device layer: a trig on step 0, lane 1 on parameter arg (an RT_PARAMS
 *      index) at 50; refused with nothing written when its row does not fit (I8)
 *   5  stock's lock set on BD step 0, sound index arg, at 50 << 8, with no room check
 *   6  BD's fine flag of sound index arg on, through a lock set with fine 1 and its clear in one
 *      call (stock makes no row: its rows follow the locks a tick later)
 * Stock makes a lock's row, and frees it, from a notice the lock set posts, by the next UI tick,
 * so the commands that count rows run from ev_tick. */
#include <stdint.h>
#include "rytm_ar2.h"

#define K_TEMPO 0x83
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))
#define DT_WAIT 6                       /* ticks to wait for stock's rows */
#define DT_MT 6                         /* a voice track with no locks in the default pattern */

/* map[pass][k], passes BD fine 0, MT fine 1, FX fine 0, FX fine 1: [0] rows that went from free to
 * used; [1..4] the first two new rows' (type, track); [5] rows in use after the clear minus before
 * the set; [6] the fine-flag byte before the set; [7] the device layer's RT_LOCK_TYPE(track, k);
 * [8] that row there after the set; [9], [10] ticks waited after the set and after the clear */
#define DT_E 12
struct dt {
    int32_t marker[2];
    int32_t cmd, arg, res, used;
    uint32_t rows;                      /* the row table's address */
    uint8_t map[4][RT_SOUND_LOCKS][DT_E];
};
struct dt dt = { { 0x5eed0d01, 0x7e570d01 }, 0, 0, 1, -1, 0, { { { 0 } } } };

static struct seed_track dt_t;
/* the command running over ticks: 1 measure, 3 fill */
static struct {
    int run, pass, last, k, t, phase, ticks, used0;
    uint8_t before[RT_ROWS];
} dm;

static unsigned dt_obj(int track)
{
    return RT_TRACK_OBJ(RT_CURRENT_PAT(RT_APP()), track);
}

static int dt_used(void)
{
    int have;
    return rt_rows(dt_obj(0), 0, &have);
}

static int dt_pass_track(int pass)
{
    return pass == 0 ? 0 : pass == 1 ? DT_MT : RT_FX_TRACK;
}

static int dt_row_used(const uint8_t *m, int j)
{
    return m[RT_ROW * j] != 0xff && m[RT_ROW * j + 1] != 0xff;
}

/* one tick of the measurement: set, wait for the rows to change, record and clear, wait, next */
static void dt_measure_tick(void)
{
    int track = dt_pass_track(dm.pass), j, used, have;
    unsigned obj = dt_obj(track);
    const uint8_t *m = rt_row_table(obj), *l = rt_locks(obj);
    uint8_t *e;
    if (!m || !l) {
        dm.run = 0;
        dt.res = SEED_E_NA;
        return;
    }
    dt.rows = (uint32_t)m;
    if (dm.k < 0) {                     /* a pass starts: a trig on step 0 */
        RT_SET_TRIG(obj, 0, 1);
        dm.k = 0;
        return;
    }
    e = dt.map[dm.pass][dm.k];
    used = rt_rows(obj, dm.k, &have);
    switch (dm.phase) {
    case 0:
        for (dm.used0 = j = 0; j < RT_ROWS; j++)
            dm.used0 += dm.before[j] = (uint8_t)dt_row_used(m, j);
        e[1] = e[2] = e[3] = e[4] = 0xff;
        e[6] = l[RT_LOCK_FINE + dm.k];
        e[7] = (uint8_t)RT_LOCK_TYPE(track, dm.k);
        RT_LOCK_SET(obj, 0, dm.k, 64 << 8, dm.pass & 1);
        dm.phase = 1;
        dm.ticks = 0;
        break;
    case 1:
        if (used == dm.used0 && ++dm.ticks < DT_WAIT)
            break;
        for (e[0] = 0, j = 0; j < RT_ROWS; j++)
            if (!dm.before[j] && dt_row_used(m, j)) {
                if (e[0] < 2) {
                    e[1 + 2 * e[0]] = m[RT_ROW * j];
                    e[2 + 2 * e[0]] = m[RT_ROW * j + 1];
                }
                e[0]++;
            }
        e[8] = (uint8_t)have;
        e[9] = (uint8_t)dm.ticks;
        RT_LOCK_CLEAR(obj, 0, dm.k);
        dm.phase = 2;
        dm.ticks = 0;
        break;
    case 2:
        if (used != dm.used0 && ++dm.ticks < DT_WAIT)
            break;
        e[5] = (uint8_t)(used - dm.used0);
        e[10] = (uint8_t)dm.ticks;
        dm.phase = 0;
        if (++dm.k == RT_SOUND_LOCKS) {
            dm.k = -1;
            if (++dm.pass > dm.last) {
                dm.run = 0;
                dt.res = SEED_OK;
            }
        }
        break;
    }
}

/* one tick of the fill: a lock, then wait until its row is there */
static void dt_fill_tick(void)
{
    int used = dt_used();
    if (dm.phase && used == dm.used0 && ++dm.ticks < DT_WAIT)
        return;
    if (dm.phase && used == dm.used0) {
        dm.run = 0;
        dt.res = SEED_E_ROOM;           /* stock made no row */
        return;
    }
    if (used < 0 || used >= dt.arg) {
        dm.run = 0;
        dt.res = used < 0 ? SEED_E_NA : SEED_OK;
        return;
    }
    if (++dm.k == RT_SOUND_LOCKS) {
        dm.k = 17;
        if (++dm.t == RT_FX_TRACK) {
            dm.run = 0;
            dt.res = SEED_E_ROOM;
            return;
        }
    }
    dm.used0 = used;
    dm.phase = 1;
    dm.ticks = 0;
    RT_LOCK_SET(dt_obj(dm.t), 0, dm.k, 64 << 8, 0);
}

void dt_tick(void *ctrl)
{
    (void)ctrl;
    if (dm.run == 1)
        dt_measure_tick();
    else if (dm.run == 3)
        dt_fill_tick();
    dt.used = dt_used();
}

int dt_key(void *brain, void *ev)
{
    const struct seed_dev *d = rt_seed_device();
    int k = EV_INT(ev, 12), down = EV_INT(ev, 16) & 1;
    int16_t parm[SEED_LANES];
    (void)brain;
    if (k != K_TEMPO)
        return 0;
    if (!down || dm.run)
        return 1;
    switch (dt.cmd) {
    case 1:
    case 2:
        dm.run = 1;
        dm.pass = dm.last = dt.cmd == 1 ? 0 : 3;
        if (dt.cmd == 1)
            dm.last = 2;
        dm.k = -1;
        dm.phase = 0;
        dt.res = 1;
        break;
    case 3:
        dm.run = 3;
        dm.t = DT_MT;
        dm.k = 16;
        dm.phase = 0;
        dt.res = 1;
        break;
    case 4:
        parm[0] = (int16_t)dt.arg;
        parm[1] = d->caps->lane_parm[1];
        if ((dt.res = d->read(0, parm, &dt_t)) == SEED_OK) {
            dt_t.s[0].flags |= SEED_TRIG;
            dt_t.s[0].lane[0] = 50;
            dt.res = d->write(0, parm, &dt_t);
        }
        break;
    case 5:
        dt.res = RT_LOCK_SET(dt_obj(0), 0, dt.arg, 50 << 8, 0);
        break;
    case 6:
        RT_LOCK_SET(dt_obj(0), 0, dt.arg, 50 << 8, 1);
        dt.res = RT_LOCK_CLEAR(dt_obj(0), 0, dt.arg);
        break;
    default:
        dt.res = SEED_E_ARG;
    }
    return 1;
}
