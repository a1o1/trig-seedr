/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Rytm's capabilities as data: the lockable parameters a lock lane can take, the lanes'
 * defaults, and the fields each kind of track has. No stock routines, so the host reference
 * renderer (tools/seedref.c) uses the same table as the device layer (rytm_ar2.h).
 *
 * The parameters, voice tracks, by page as the Rytm's knobs A..H show them. `id` is the sound
 * index; a lane's value is the stored value's whole part (value >> 8), 0..127 (SMP: the sample
 * slot, 0 = OFF). Left out until their ranges are measured: the SYNTH page (9..16, a different
 * parameter per machine), and the stepped settings LOP (23), filter TYP (32), LFO MUL, WAV, MOD
 * and DST (1..4). */
#ifndef RYTM_PARAMS_H
#define RYTM_PARAMS_H

#include "../lib/seed.h"

#define RT_P_SMP  3                    /* lane 1's default: the sample slot */
#define RT_P_FRQ 12                    /* lane 2's default: filter frequency */
#define RT_NPARAMS 26

#define RT_PARAMS { \
    /* SMPL */ \
    { "TUN", 17, 0, 127 }, { "FIN", 18, 0, 127 }, { "BR", 20, 0, 127 }, { "SMP", 19, 0, 127 }, \
    { "STA", 21, 0, 127 }, { "END", 22, 0, 127 }, { "SLEV", 24, 0, 127 }, \
    /* FLTR */ \
    { "FATK", 26, 0, 127 }, { "FDEC", 27, 0, 127 }, { "FSUS", 28, 0, 127 }, { "FREL", 29, 0, 127 }, \
    { "FENV", 25, 0, 127 }, { "FRQ", 30, 0, 127 }, { "RES", 31, 0, 127 }, \
    /* AMP */ \
    { "AATK", 33, 0, 127 }, { "AHLD", 34, 0, 127 }, { "ADEC", 35, 0, 127 }, { "OVR", 36, 0, 127 }, \
    { "DEL", 40, 0, 127 }, { "REV", 41, 0, 127 }, { "PAN", 38, 0, 127 }, { "VOL", 37, 0, 127 }, \
    /* LFO */ \
    { "LSPD", 5, 0, 127 }, { "LFAD", 6, 0, 127 }, { "LSPH", 7, 0, 127 }, { "LDEP", 8, 0, 127 }, \
}

/* the track's fields: the FX track has no note, accent or slide and no lanes (its sound
 * indexes are the FX machine's) */
#define RT_VOICE_FIELDS (SEED_F_NOTE | SEED_F_VEL | SEED_F_LEN | SEED_F_UTIME | SEED_F_COND | SEED_F_RETRIG | \
                         SEED_F_ACCENT | SEED_F_SLIDE | SEED_F_LOCKS)
#define RT_FX_FIELDS    (SEED_F_UTIME | SEED_F_COND)

/* what each track plays, for TOOLS YOLO's ranges (0.7): BD; SD, RS, CP; BT and the toms; CH, OH, CY;
 * CB; the FX track none */
#define RT_TRACK_ROLES { SEED_ROLE_KICK, SEED_ROLE_BACK, SEED_ROLE_BACK, SEED_ROLE_BACK, \
                         SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_PERC, SEED_ROLE_PERC, \
                         SEED_ROLE_HAT, SEED_ROLE_HAT, SEED_ROLE_HAT, SEED_ROLE_PERC, SEED_ROLE_METER }

#endif
