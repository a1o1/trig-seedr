/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The ColdFire build check for rytm_ar2.h (make -C device coldfire): one unit that takes every
 * entry of the version 1 interface and the 0.3 one, so that all of it is compiled with the mod's
 * flags and its size can be read. Not part of any mod. */
#include "rytm_ar2.h"

const struct seed_dev *cf_device(void)
{
    return rt_seed_device();
}

int cf_gen(struct gen_track *t)
{
    unsigned obj, pat;
    static struct rt_snap s;
    rt_current(&obj, &pat);
    if (!rt_read(obj, t, 0) || !rt_lock_room(obj, RT_SAMPLE_SLOT) || !rt_snap_take(&s, obj, pat, 0))
        return 0;
    rt_snap_restore(&s);
    return rt_write(obj, t) + rt_euclid(obj) + rt_seed_written();
}
