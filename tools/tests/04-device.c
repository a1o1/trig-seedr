/* SPDX-License-Identifier: GPL-2.0-or-later */
/* devtest: a test mod for the device layer's version 1 interface (device/rytm_ar2.h,
 * rt_seed_device()), built by tools/tests/04-device.sh into build/. Never released.
 * Four keys, each on the current track (BD in the test), swallowed:
 *   TEMPO   move: every step one step later (origins i - 1), no audition
 *   CHAIN   audition: snapshot, then a take from it: every step three later, lane 1 (sample slot)
 *           set to step + 1 on each trig, and the accent flag flipped on step 4
 *   SONG    restore the snapshot (NO)
 *   FILL    a lock no row can hold (RT_ROWS_MAX 0 for this write): lane 1 on TUN at step 0;
 *           must be refused with nothing written (I8) */
#include <stdint.h>
#define RT_ROWS_MAX dt_rows_max
static int dt_rows_max = 71;
#include "rytm_ar2.h"

#define K_TEMPO 0x83
#define K_CHAIN 0x82
#define K_SONG  0x81
#define K_FILL  0xb2
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))

static struct seed_track dt_t, dt_o;
/* two marker words, then the last result of TEMPO, CHAIN, SONG and FILL (1: not run yet); the
 * test finds the marker with dtpeek find */
int32_t dt_res[6] = { 0x5eed0404, 0x7e570404, 1, 1, 1, 1 };
#define dt_result (dt_res + 2)

static void dt_rotate(struct seed_track *t, int k)
{
    int i, n = t->length;
    RT_MEMCPY(&dt_o, t, sizeof dt_o);    /* a mod links no libc: stock's memcpy */
    for (i = 0; i < n; i++)
        RT_MEMCPY(&t->s[i], &dt_o.s[((i - k) % n + n) % n], sizeof t->s[i]);
}

int dt_key(void *brain, void *ev)
{
    const struct seed_dev *d = rt_seed_device();
    int k = EV_INT(ev, 12), down = EV_INT(ev, 16) & 1, n, i;
    int16_t parm[SEED_LANES];
    (void)brain;
    if (k != K_TEMPO && k != K_CHAIN && k != K_SONG && k != K_FILL)
        return 0;
    if (!down)
        return 1;
    n = d->current();
    parm[0] = d->caps->lane_parm[0];
    parm[1] = d->caps->lane_parm[1];
    switch (k) {
    case K_TEMPO:
        if ((dt_result[0] = d->read(n, parm, &dt_t)) == SEED_OK) {
            dt_rotate(&dt_t, 1);
            dt_result[0] = d->write(n, parm, &dt_t);
        }
        break;
    case K_CHAIN:
        if ((dt_result[1] = d->snapshot((uint16_t)(1u << n))) == SEED_OK &&
            (dt_result[1] = d->read(n, parm, &dt_t)) == SEED_OK) {
            dt_rotate(&dt_t, 3);
            for (i = 0; i < dt_t.length; i++)
                if (dt_t.s[i].flags & SEED_TRIG)
                    dt_t.s[i].lane[0] = (int16_t)(i + 1);
            dt_t.s[4].flags ^= SEED_ACCENT;
            dt_result[1] = d->write(n, parm, &dt_t);
        }
        break;
    case K_SONG:
        d->restore();
        d->drop();
        dt_result[2] = SEED_OK;
        break;
    case K_FILL:
        parm[0] = 0;                   /* TUN, sound index 17: no row for it yet */
        if ((dt_result[3] = d->read(n, parm, &dt_t)) == SEED_OK) {
            dt_t.s[0].lane[0] = 50;
            dt_rows_max = 0;
            dt_result[3] = d->write(n, parm, &dt_t);
            dt_rows_max = 71;
        }
        break;
    }
    return 1;
}
