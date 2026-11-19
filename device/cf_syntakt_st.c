/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The ColdFire build check for syntakt_st.h (make -C device coldfire): one unit that takes every
 * entry of the device layer, so that all of it is compiled with the mod's flags and its size can be
 * read. Not part of any mod. */
#include "syntakt_st.h"

const struct seed_dev *cf_st_device(void *mem, unsigned size)
{
    if (!st_seed_attach(mem, size))
        return 0;
    st_seed_set_machine(0, 0);
    return st_seed_device();
}

int cf_st_rest(void)
{
    unsigned obj, pat;
    st_current(&obj, &pat);
    return st_euclid(obj) + st_grid_recording() + st_seed_written() + st_cond_to_seed(35) + st_cond_from_seed(64);
}
