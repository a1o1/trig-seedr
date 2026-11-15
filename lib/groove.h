/* SPDX-License-Identifier: GPL-2.0-or-later */
/* groove: for groove tables made by tools/groove2h.py before 0.4, which include this header and
 * name the table type struct gen_groove. That type is struct seed_groove now (lib/seed.h); tables
 * made since include seed.h directly. Regenerate a local grooves-live.h to drop this header. */
#ifndef GROOVE_H
#define GROOVE_H

#include "seed.h"

#define gen_groove seed_groove

#endif
