/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The ColdFire build check for rytm_ar2.h (make -C device coldfire): one unit that takes every
 * entry of the device layer, so that all of it is compiled with the mod's
 * flags and its size can be read. Not part of any mod. */
#include "rytm_ar2.h"

const struct seed_dev *cf_device(void)
{
    return rt_seed_device();
}

int cf_rest(void)
{
    unsigned obj, pat;
    static struct rt_snap s;
    rt_current(&obj, &pat);
    if (!rt_lock_room(obj, RT_SAMPLE_SLOT) || !rt_snap_take(&s, obj, pat, 0))
        return 0;
    rt_snap_restore(&s);
    return rt_euclid(obj) + rt_seed_written();
}
