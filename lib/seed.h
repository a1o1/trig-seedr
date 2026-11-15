/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seed: the version 1 contract (docs/design-v1.md). Steps, tracks, the recipe, device
 * capabilities, every enum version 1 needs, and the prototypes of every stage.
 *
 * Owner: the lead. Frozen after task 0.1; a change goes through a request in docs/status/.
 * Each function says which file implements it and which task owns that file
 * (docs/plan-v1.md section 4). Until its owner lands, a function is a stub that returns
 * SEED_E_NA, and the file's `seed_have_<name>` flag is 0.
 *
 * Device-neutral C99, freestanding, integer only, no C library (P6). Values use the Analog Rytm
 * MKII's encodings, which the Syntakt and Analog Four share or map onto; a device layer
 * translates anything else:
 *   flags  the step's flag word; the SEED_* bits below, other bits are kept as they are
 *   note   absolute MIDI note, or SEED_DEFAULT (the track's default note)
 *   vel    1..127, or SEED_DEFAULT
 *   len    stepped length code 0..127 (14 = 1/16, 127 = INF), or SEED_DEFAULT
 *   utime  micro timing in 1/24 of a step, inside the capabilities' range (Rytm: -23..+23)
 *   cond   trig condition code 0..64 (0..21 = 1%..100%, 22 FILL, 28 1ST, 30..64 A:B), SEED_NO_COND
 *   rlen, rrate, rvel  retrig length 0..127, rate 0..16 (9 = 1/16), velocity curve -128..127
 *   lane   a lock lane's value in its parameter's units (struct seed_param), or SEED_NO_LOCK
 */
#ifndef SEED_H
#define SEED_H

#include <stdint.h>

/* ---- sizes ---- */

#define SEED_STEPS  64              /* steps per track, at most */
#define SEED_PAGE   16              /* steps per page (PHRS works in pages) */
#define SEED_TRACKS 16              /* tracks per pattern, at most */
#define SEED_LANES  2               /* lock lanes in version 1 */
#define SEED_NAME   5               /* a parameter's screen name: 4 characters and a 0 */

/* ---- results. Every stage returns SEED_OK or one of the negative codes. ---- */

#define SEED_OK     0
#define SEED_E_NA   (-1)            /* not available: the feature is not built or the device lacks it */
#define SEED_E_ARG  (-2)            /* a setting outside its range */
#define SEED_E_ROOM (-3)            /* a lock cannot be stored (lock rows full): nothing written (I8) */

/* ---- steps ---- */

#define SEED_TRIG   0x0001u
#define SEED_RETRIG 0x0002u
#define SEED_MUTE   0x0004u
#define SEED_ACCENT 0x0008u         /* the accent flag (DYNAMICS ACC) */
#define SEED_SWING  0x0010u         /* stays with the position, never moved */
#define SEED_SLIDE  0x0020u
/* the bits a generating source clears; any other bit is kept */
#define SEED_KNOWN  (SEED_TRIG | SEED_RETRIG | SEED_MUTE | SEED_ACCENT | SEED_SLIDE)

#define SEED_DEFAULT   0xffu        /* note, vel, len: the track's default */
#define SEED_NO_COND   (-1)
#define SEED_NO_LOCK   (-32768)     /* lane: no lock on this step */
#define SEED_NO_ORIGIN (-1)         /* origin: a generated step */

/* the values a new trig gets on the Rytm (its place-trig routine) */
#define SEED_NEW_RLEN  46
#define SEED_NEW_RRATE 9

/* One step as the chain sees it. `origin` is the original step whose data this step carries
 * (design section 4): the device writes step i as a copy of the original step `origin`, all
 * parameter locks included, then applies the fields and lanes. Two steps may share an origin
 * (a copy); an original step no step refers to is overwritten. A step read from the device has
 * origin == its own index. */
struct seed_step {
    uint16_t flags;
    uint8_t note, vel, len;
    int8_t utime, cond;
    uint8_t rlen, rrate;
    int8_t rvel;
    int8_t origin;                    /* 0..63, or SEED_NO_ORIGIN */
    uint8_t spare;                    /* 0; makes the layout padding-free, so tracks compare bytewise */
    int16_t lane[SEED_LANES];         /* the lanes' parameters' locks on this step */
};

/* which per-trig fields a device or track has (struct seed_caps, struct seed_track) */
#define SEED_F_NOTE   0x0001u
#define SEED_F_VEL    0x0002u
#define SEED_F_LEN    0x0004u
#define SEED_F_UTIME  0x0008u
#define SEED_F_COND   0x0010u
#define SEED_F_RETRIG 0x0020u
#define SEED_F_ACCENT 0x0040u
#define SEED_F_SLIDE  0x0080u
#define SEED_F_LOCKS  0x0100u

struct seed_track {
    int16_t length;                   /* steps in play, 1..64 */
    uint16_t fields;                  /* SEED_F_*: what this track has (the Rytm's FX track: no note) */
    uint8_t default_note, default_vel;
    struct seed_step s[SEED_STEPS];   /* steps at and past `length` are never changed (I6) */
};

/* ---- the recipe: one per track (V5). Every knob is an int16_t so that a page table can point
 * at any of them (mods/seedr-ar2/pages.c); the seed alone is wider. SEED_DEF in a value's `lo`
 * is the knob's DEF: that field is left as it is (V4). ---- */

#define SEED_DEF (-32768)

enum seed_src     { SEED_SRC_TRACK, SEED_SRC_RANDOM, SEED_SRC_EUCLID, SEED_SRC_MAP, SEED_SRC_COPY, SEED_SRC_N };
enum seed_role    { SEED_ROLE_METER, SEED_ROLE_KICK, SEED_ROLE_BACK, SEED_ROLE_HAT, SEED_ROLE_PERC, SEED_ROLE_N };
enum seed_metr    { SEED_METR_4_4, SEED_METR_3_4, SEED_METR_6_8, SEED_METR_12_16, SEED_METR_5_8, SEED_METR_7_8, SEED_METR_N };
enum seed_phrs    { SEED_PHRS_OFF, SEED_PHRS_AAAA, SEED_PHRS_AAAB, SEED_PHRS_ABAB, SEED_PHRS_ABAC, SEED_PHRS_ABCD, SEED_PHRS_N };
enum seed_cvar    { SEED_CVAR_OFF, SEED_CVAR_1_2, SEED_CVAR_AAAB, SEED_CVAR_N };
enum seed_mask    { SEED_MASK_OFF, SEED_MASK_AVOID, SEED_MASK_ONLY, SEED_MASK_N };
/* a field rotated against the trigs (PLACE FLD), as 0.3's ROTATE */
enum seed_field   { SEED_FLD_OFF, SEED_FLD_VEL, SEED_FLD_NOTE, SEED_FLD_LEN, SEED_FLD_UTIME, SEED_FLD_COND,
                    SEED_FLD_RTRG,              /* retrig on / off, length, rate, curve together */
                    SEED_FLD_ACC,               /* the accent flag, by step */
                    SEED_FLD_LANE1, SEED_FLD_LANE2,
                    SEED_FLD_ALL,               /* whole steps, swing stays (ROTATE ALL) */
                    SEED_FLD_N };
enum seed_rmode   { SEED_RMODE_TRIG,            /* values move between the trigs, in order */
                    SEED_RMODE_GRID,            /* values move by steps; one landing on no trig is dropped */
                    SEED_RMODE_N };
enum seed_shape   { SEED_SHAPE_FLAT, SEED_SHAPE_STRONG, SEED_SHAPE_WEAK, SEED_SHAPE_OFF, SEED_SHAPE_NOT1,
                    SEED_SHAPE_NOTBAR, SEED_SHAPE_RANK, SEED_SHAPE_NOISE, SEED_SHAPE_EVRY, SEED_SHAPE_MAP,
                    SEED_SHAPE_N };
enum seed_amod    { SEED_AMOD_RAND, SEED_AMOD_TOP, SEED_AMOD_N };
enum seed_lmod    { SEED_LMOD_RANGE, SEED_LMOD_LEGATO, SEED_LMOD_N };
/* where a per-trig value comes from (V9: every value field takes the same sources; note pools
 * and pattern lists are later values of this enum) */
enum seed_vsrc    { SEED_VSRC_RANGE, SEED_VSRC_LIST, SEED_VSRC_NOISE, SEED_VSRC_N };
enum seed_order   { SEED_ORDR_UP, SEED_ORDR_DOWN, SEED_ORDR_PING, SEED_ORDR_PNG2, SEED_ORDR_ODD, SEED_ORDR_RAND,
                    SEED_ORDR_N };
enum seed_ctyp    { SEED_CTYP_PROB, SEED_CTYP_AB, SEED_CTYP_1ST, SEED_CTYP_FILL, SEED_CTYP_MIX, SEED_CTYP_N };
enum seed_tool_id { SEED_TOOL_ROTATE_ALL, SEED_TOOL_N };

#define SEED_LIST_MAX 16            /* the longest value list */

/* a per-trig value source */
struct seed_values {
    int16_t lo, hi;                   /* the range, either order; lo == SEED_DEF: leave the field */
    int16_t src;                      /* enum seed_vsrc */
    int16_t order;                    /* enum seed_order (LIST) */
    int16_t len;                      /* list length 1..SEED_LIST_MAX (LIST) */
    int16_t zoom;                     /* steps per noise cell 1..64 (NOISE) */
};

struct seed_lane {
    int16_t parm;                     /* index into the capabilities' params */
    int16_t pct;                      /* 0..100 % of trigs that get a lock; 0: the lane is left */
    struct seed_values val;
};

struct seed_recipe {
    /* TAKE */
    uint32_t seed;                    /* the take number n */
    int16_t vary;                     /* 0..100 % of take n's keys that take n + 1 keeps (P3) */
    int16_t phrs;                     /* enum seed_phrs */
    int16_t cvar;                     /* enum seed_cvar */
    int16_t metr;                     /* enum seed_metr (per track, A7) */
    int16_t beat;                     /* steps per beat 1..16 */
    int16_t qual;                     /* candidates scored per take, 1..8 (1: no scoring) */
    /* RHYTHM */
    int16_t src;                      /* enum seed_src */
    int16_t grid;                     /* RANDOM: 1, 2, 4 or 8 steps */
    int16_t dens;                     /* RANDOM: 0..100 % of grid positions */
    int16_t bias;                     /* RANDOM: -100..100, rank against chance in the keys */
    int16_t role;                     /* RANDOM: enum seed_role, the rank table */
    int16_t eu_steps, eu_puls;        /* EUCLID: 1..64 steps, 0..steps pulses */
    int16_t map_part;                 /* MAP: 0..2 */
    int16_t map_dens;                 /* MAP: 0..255; a step fires when its key exceeds 255 - map_dens */
    int16_t copy_trk;                 /* COPY: the track whose trig positions are taken */
    /* PLACE: ROT -> SYNC -> MASK -> SKIP; the field rotation runs last in the chain */
    int16_t rot;                      /* -63..63 steps, a plain rotation after the source */
    int16_t sync;                     /* 0..100 % of eligible trigs moved one grid step earlier */
    int16_t mask;                     /* enum seed_mask */
    int16_t mtrk;                     /* MASK: the other track */
    int16_t skip;                     /* 0..64: the first `skip` steps are cleared */
    int16_t fld;                      /* enum seed_field, SEED_FLD_OFF..SEED_FLD_LANE2 */
    int16_t frot;                     /* the field's rotation, -63..63 */
    int16_t fmod;                     /* enum seed_rmode */
    /* DYNAMICS */
    struct seed_values vel;           /* DEF or a range 1..127 */
    int16_t shape;                    /* enum seed_shape */
    int16_t amt;                      /* 0..100 % of the shape */
    int16_t n;                        /* EVRY: every n-th trig; NOISE: zoom */
    int16_t acc;                      /* 0..100 % of trigs with the accent flag */
    int16_t amod;                     /* enum seed_amod */
    int16_t hum;                      /* 0..100: random velocity spread */
    /* NOTE */
    struct seed_values len;           /* DEF or length codes 0..127 */
    int16_t lmod;                     /* enum seed_lmod */
    struct seed_values note;          /* DEF or -24..+24 from the track's default note */
    /* LOCK */
    struct seed_lane lane[SEED_LANES];
    /* TRIG */
    int16_t cond;                     /* 0..100 % of trigs with a condition */
    int16_t ctyp;                     /* enum seed_ctyp */
    struct seed_values cval;          /* PROB: codes 0..21; AB: the cycle B 2..8 (lo) */
    int16_t rtrg;                     /* 0..100 % of trigs with a retrig */
    struct seed_values rate;          /* retrig rate codes 0..16 */
    int16_t rcur;                     /* retrig velocity curve -128..127 */
    /* TIME */
    int16_t grv;                      /* groove 1..ngrooves, 0: none */
    int16_t timing;                   /* -100..100 %: share of the groove's micro timing */
    int16_t gvel;                     /* -100..100 %: the groove's velocity, applied after DYNAMICS */
    int16_t quant;                    /* 0..100 %: existing micro timing pulled to 0 first */
    int16_t rand;                     /* 0..100 % of the device's micro-timing range, random */
    int16_t drft;                     /* 0..100: correlated drift */
    int16_t shft;                     /* the groove moved this many steps later (negative: earlier) */
};

/* settings shared by a pattern's tracks (A1) */
struct seed_shared {
    int16_t map_x, map_y;             /* MAP: 0..255 */
    int16_t map_chaos;                /* MAP: 0..255 */
};

/* TOOLS: pattern-wide operations with their own YES (V7) */
struct seed_tool {
    int16_t tool;                     /* enum seed_tool_id */
    int16_t steps;                    /* ROTATE ALL: -63..63 */
};

/* a groove (TIME GRV), as tools/groove2h.py writes them: per step the micro timing at 100 % in 1/24
 * step and the groove's velocity there (0: no groove note, the step's velocity stays) */
#define SEED_GROOVE_NAME 21
struct seed_groove {
    char name[SEED_GROOVE_NAME];
    uint8_t len;                      /* steps in one cycle, 1..64 */
    uint8_t vmax;                     /* the loudest velocity in vel[] (0: no velocity data) */
    int8_t off[SEED_STEPS];
    uint8_t vel[SEED_STEPS];
};

/* ---- device capabilities (design section 10, V8). A page or knob whose field, lane or
 * feature the device lacks is not shown. ---- */

struct seed_param {
    char name[SEED_NAME];             /* screen name, upper case */
    uint8_t id;                       /* the device's own number (Rytm: the sound index) */
    int16_t lo, hi;                   /* the lane's value range */
};

struct seed_caps {
    uint8_t tracks;                   /* tracks per pattern, <= SEED_TRACKS */
    uint8_t steps;                    /* steps per track, <= SEED_STEPS */
    uint16_t fields;                  /* SEED_F_* the device has on some track */
    int8_t utime_lo, utime_hi;        /* micro-timing range */
    uint8_t lanes;                    /* lock lanes, <= SEED_LANES */
    uint8_t nparams;
    const struct seed_param *params;  /* the lockable parameters */
    int16_t lane_parm[SEED_LANES];    /* each lane's default parameter (Rytm lane 1: sample slot) */
};

/* What the render sees of the pattern besides the track in scope (I2: same recipe, original
 * and context give the same result). The device fills it (struct seed_dev). */
struct seed_ctx {
    const struct seed_caps *caps;
    int track;                        /* the track in scope; mixed into its takes' numbers */
    int tracks;                       /* entries used below */
    uint8_t length[SEED_TRACKS];
    uint8_t trig[SEED_TRACKS][SEED_STEPS / 8];   /* each track's trigs as stored, bit i of byte i / 8 */
    struct seed_shared shared;
    const struct seed_groove *grooves;
    int ngrooves;
};

static inline int seed_ctx_trig(const struct seed_ctx *c, int track, int step)
{
    if (track < 0 || track >= c->tracks || step < 0 || step >= c->length[track])
        return 0;
    return (c->trig[track][step >> 3] >> (step & 7)) & 1;
}

/* The device layer (design section 10). The mod calls these; lib/ never does. `parm` is the
 * recipe's lane parameters (SEED_LANES entries). Keys and drawing stay in the device layer and
 * the mod. */
struct seed_dev {
    const struct seed_caps *caps;
    int (*current)(void);             /* the track in scope */
    /* read any track (also for MASK and COPY): fields, lanes, origin = index; SEED_OK or < 0 */
    int (*read)(int track, const int16_t *parm, struct seed_track *t);
    /* SEED_OK when every lock in t can be stored, else SEED_E_ROOM; nothing is written (I8) */
    int (*room)(int track, const int16_t *parm, const struct seed_track *t);
    /* write the track in scope: origins as whole-step copies from the snapshot (all locks), then
     * fields and lanes; only steps that differ are touched. Calls room first. */
    int (*write)(int track, const int16_t *parm, const struct seed_track *t);
    void (*context)(struct seed_ctx *c, int track);
    int (*snapshot)(uint16_t tracks);  /* the audition's undo: one bit per track in scope */
    void (*restore)(void);            /* back to the snapshot, byte-identical (I7) */
    void (*drop)(void);               /* keep: forget the snapshot */
    void (*changed)(int track);       /* the stock change notice */
};

/* ---- random numbers: lib/seed_rng.c (lead). P1-P3. ---- */

enum seed_layer { SEED_L_TAKE, SEED_L_RHYTHM, SEED_L_PLACE, SEED_L_DYN, SEED_L_NOTE, SEED_L_LOCK, SEED_L_TRIG,
                  SEED_L_TIME, SEED_L_FIELDS, SEED_L_N };

/* What a random number is for. A slot (design section 3) is (layer, purpose, index); the index is
 * the step unless a purpose says otherwise. Every version 1 use has its own value here, so no
 * two modules share numbers (P1). */
enum seed_purpose {
    SEED_P_REROLL,                    /* seed_rng.c only: the VARY re-roll test */
    SEED_P_DENS,                      /* RHYTHM: the RANDOM source's keys */
    SEED_P_CHAOS,                     /* RHYTHM: MAP chaos, index = part * 256 + pass */
    SEED_P_SYNC,                      /* PLACE: SYNC keys */
    SEED_P_VEL,                       /* DYNAMICS: VEL value source */
    SEED_P_ACC,                       /* DYNAMICS: ACC keys */
    SEED_P_HUM,                       /* DYNAMICS: HUM */
    SEED_P_SHAPE,                     /* DYNAMICS: SHAPE = NOISE */
    SEED_P_LEN,                       /* NOTE: LEN value source */
    SEED_P_NOTE,                      /* NOTE: NOTE value source */
    SEED_P_LOCK_KEY,                  /* LOCK: % keys; + lane */
    SEED_P_LOCK_VAL = SEED_P_LOCK_KEY + SEED_LANES,   /* LOCK: value source; + lane */
    SEED_P_COND_KEY = SEED_P_LOCK_VAL + SEED_LANES,   /* TRIG: COND % keys */
    SEED_P_COND_TYPE,                 /* TRIG: CTYP = MIX */
    SEED_P_COND_VAL,                  /* TRIG: CVAL value source */
    SEED_P_RTRG_KEY,                  /* TRIG: RTRG % keys */
    SEED_P_RTRG_RATE,                 /* TRIG: RATE value source */
    SEED_P_UTIME,                     /* TIME: RAND */
    SEED_P_GROOVE,                    /* TIME: the groove's random part */
    SEED_P_DRIFT,                     /* TIME: DRFT */
    SEED_P_N
};

#define SEED_SLOT(layer, purpose, index) \
    ((uint32_t)(layer) << 24 | (uint32_t)(purpose) << 16 | ((uint32_t)(index) & 0xffffu))

/* how far back a slot looks for its latest re-roll (design section 3) */
#define SEED_VARY_WALK 64

/* a take: (seed n, recipe) plus what is mixed into every number */
struct seed_take {
    uint32_t seed;                    /* n */
    uint32_t salt;                    /* the track (seed_render sets it from the context) */
    int vary;                         /* 0..100 */
    int cand;                         /* QUAL's candidate, 0 .. qual - 1 */
    int length;                       /* the track's length, 1..64 (seed_render sets it; NOISE wraps at it) */
};

void seed_take_init(struct seed_take *k, uint32_t seed, uint32_t salt, int vary, int cand);

uint32_t seed_hash(uint32_t x);       /* a 32-bit integer hash (bijective) */

/* The slot's random number at this take (P3). VARY 0: every take draws afresh. Otherwise the
 * number comes from the latest take k <= n at which the slot re-rolled (hash(k, slot) mod 100 >=
 * VARY), looking back SEED_VARY_WALK takes; with no re-roll among them, from the slot's anchor,
 * which is the same for every n (so VARY 100 gives one take for every seed). */
uint32_t seed_rand(const struct seed_take *k, uint32_t slot);
int seed_rand_range(const struct seed_take *k, uint32_t slot, int lo, int hi);  /* lo..hi, either order */

#define SEED_RANK_MAX 65535           /* ranks are 0..SEED_RANK_MAX, higher = stronger step */

/* Keys for a share knob (P2): key = |bias| * rank + (100 - |bias|) * random, rank reversed for a
 * negative bias; rank NULL is all 0. Ties go to the earlier step, so keys are distinct. Index i's
 * random number comes from slot + i. */
void seed_keys(uint32_t *key, int n, const struct seed_take *k, uint32_t slot, const uint16_t *rank, int bias);

/* The steps in key order, highest first, among those with eligible[i] != 0 (eligible NULL: all).
 * Returns how many there are. The first m of them are the top m (I4: they nest). */
int seed_order(uint8_t *order, const uint32_t *key, const uint8_t *eligible, int n);

/* round(pct * n / 100), pct clamped to 0..100 */
int seed_share(int pct, int n);

/* ---- the chain: lib/seed_render.c (lead) ---- */

/* scratch for seed_render, so the device can keep it out of the stack */
struct seed_work {
    struct seed_track a, b, c, page;
};

/* A recipe at its defaults: every layer at 0, SOURCE = TRACK. */
void seed_recipe_defaults(struct seed_recipe *r, const struct seed_caps *caps);
/* A new recipe for track t (V6): SOURCE = TRACK when t has trigs, RANDOM when it is empty;
 * the meter (METR, BEAT) from `last`, the recipe used last (A7), when not NULL. */
void seed_recipe_init(struct seed_recipe *r, const struct seed_track *t, const struct seed_recipe *last,
                      const struct seed_caps *caps);
/* 1 when the layer would change something (V4: a layer at 0 does not touch its fields) */
int seed_layer_active(const struct seed_recipe *r, enum seed_layer layer);

/* Renders the whole chain from the original (V2): out = the take. Steps at and past the length
 * are copied unchanged. SEED_OK, or the first stage's error (SEED_E_NA: a layer is active whose
 * feature is not available). */
int seed_render(struct seed_track *out, const struct seed_track *orig, const struct seed_recipe *r,
                const struct seed_ctx *c, struct seed_work *w);

/* the step a generating source starts from: no trig, fields at their defaults, swing, unknown
 * flags and lanes kept, origin none (P4: untouched means the default) */
void seed_step_clear(struct seed_step *s);
void seed_track_copy(struct seed_track *to, const struct seed_track *from);

/* Every stage has this shape: it works on t (the take so far) in place, below t->length. It is
 * called only when its part of the recipe is active, and returns SEED_OK or < 0. */
typedef int seed_stage(struct seed_track *t, const struct seed_recipe *r, const struct seed_take *k,
                       const struct seed_ctx *c);

/* RHYTHM: the only layer that creates trigs. Each sets SEED_TRIG on the steps it picks of a
 * track seed_render has cleared (seed_step_clear). */
seed_stage seed_src_random;           /* src_random.c (0.2): GRID, DENS, BIAS, ROLE; exact count (I5) */
seed_stage seed_src_euclid;           /* src_euclid.c (F2) */
seed_stage seed_src_map;              /* src_map.c (F3) */
seed_stage seed_src_copy;             /* src_copy.c (F4) */

/* PLACE, in this order. Moves carry the whole step, origin included; swing stays. */
seed_stage seed_place_rot;            /* place.c (0.2) */
seed_stage seed_place_sync;           /* place_sync.c (F5) */
seed_stage seed_place_mask;           /* place_mask.c (F4): only removes (I5) */
seed_stage seed_place_skip;           /* place.c (0.2): only removes (I5) */

seed_stage seed_dyn;                  /* dyn.c (0.2): VEL, SHAPE, AMT, N, ACC, AMOD, HUM */
seed_stage seed_note;                 /* values.c (0.2): LEN, LMOD, NOTE */
seed_stage seed_lock;                 /* values.c (0.2): both lanes */
seed_stage seed_trig;                 /* trig.c (0.2): COND, CTYP, CVAL, RTRG, RATE, RCUR */
seed_stage seed_time;                 /* timing.c (0.2): GRV, TIME, GVEL, QUANT, RAND, SHFT; calls DRFT */
seed_stage seed_place_fields;         /* place.c (0.2): FLD, FROT, FMOD, last in the chain */

/* ---- helpers the stages call ---- */

/* meter.c (F1): the rank of step i of n for the meter and role, 0..SEED_RANK_MAX, or SEED_E_NA */
int seed_rank(int metr, int beat, int role, int step, int n);
/* meter.c (F1): the five-level metric weight of step i in the meter, 0..4 (bar, a grouping above
 * the counted beat, a counted beat, its first division, finer), or SEED_E_ARG; in 4/4 with an
 * even BEAT, seed_weight's */
int seed_meter_weight(int metr, int beat, int step);
/* dyn.c (0.2): 0.3's five-level metric weight, 0..4 (bar, half bar, beat, "&", other) */
int seed_weight(int step, int beat);
/* the level of a step in a velocity shape, 0 (VEL's low end) .. 255 (its high end), or SEED_E_NA */
int seed_shape_rank(const struct seed_recipe *r, int step, int n);                         /* dyn_rank.c (F1) */
int seed_shape_noise(const struct seed_recipe *r, const struct seed_take *k, int step);   /* dyn_noise.c (F7) */
/* dyn_rank.c (F1): ACC with AMOD = TOP, the highest-ranked trigs get the accent flag */
seed_stage seed_acc_top;

/* src_map.c (F3): the map level of step i for part, X and Y, 0..255, or SEED_E_NA */
int seed_map_level(int part, int x, int y, int step);

/* A field's value for the trig at `step`, the index-th trig of the track, from v (values.c, 0.2).
 * `slot` is SEED_SLOT(layer, purpose, 0): RANGE draws slot + step, LIST its item i from
 * slot + 0x100 + i, NOISE its corner c from slot + 0x200 + c (the contour wraps at k->length). These
 * three return the value and cannot
 * report an error (a value may be negative, so SEED_E_NA is a valid value): a stage checks the
 * source is built (seed_have_list, seed_have_noise) and refuses before it calls them. */
int seed_value(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int step, int index);
int seed_value_list(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int index);  /* values_list.c (F6) */
int seed_value_noise(const struct seed_values *v, const struct seed_take *k, uint32_t slot, int step);  /* noise.c (F7) */
/* values_list.c (F6): LMOD = LEGATO, the length code reaching the next trig after `step` */
int seed_len_legato(const struct seed_track *t, int step);
/* noise.c (F7): integer value noise at x with `zoom` steps per cell, 0..255 */
int seed_noise(uint32_t seed, int x, int zoom);

/* trig.c (0.2): the condition code for A:B */
int seed_cond_ab(int a, int b);

/* timing_drift.c (F8): DRFT, called by seed_time */
seed_stage seed_time_drift;

/* vary.c (F9): the take offset (0..3) PHRS gives page p, or SEED_E_NA */
int seed_phrase_take(int phrs, int page);
/* vary.c (F9): CVAR, takes a (n) and b (n + 1) rendered into one track with conditions */
int seed_cvar_merge(struct seed_track *out, const struct seed_track *a, const struct seed_track *b,
                    const struct seed_recipe *r);

/* score.c (F11): a candidate's score, >= 0 (higher is better), or SEED_E_NA */
int seed_score(const struct seed_track *t, const struct seed_recipe *r, const struct seed_ctx *c);

/* place.c (0.2): rotate steps [from, from + n) of t by k (positive: later) inside that range,
 * one field (enum seed_field) by mode (enum seed_rmode). FLD and ROTATE ALL. */
int seed_rotate(struct seed_track *t, int from, int n, int k, int what, int mode);

/* ---- what is built: each file sets its flag to 1 when its owner lands. The mod hides a page,
 * knob or value whose flag is 0. ---- */

extern const uint8_t seed_have_random;  /* src_random.c (0.2) */
extern const uint8_t seed_have_place;   /* place.c (0.2): ROT, SKIP, FLD */
extern const uint8_t seed_have_dyn;     /* dyn.c (0.2) */
extern const uint8_t seed_have_values;  /* values.c (0.2): NOTE, LOCK with RANGE */
extern const uint8_t seed_have_trig;    /* trig.c (0.2) */
extern const uint8_t seed_have_timing;  /* timing.c (0.2) */
extern const uint8_t seed_have_rank;    /* meter.c (F1): ROLE, METR, SHAPE = RANK, AMOD = TOP */
extern const uint8_t seed_have_euclid;  /* src_euclid.c (F2) */
extern const uint8_t seed_have_map;     /* src_map.c (F3): MAP, SHAPE = MAP */
extern const uint8_t seed_have_copy;    /* src_copy.c (F4) */
extern const uint8_t seed_have_mask;    /* place_mask.c (F4) */
extern const uint8_t seed_have_sync;    /* place_sync.c (F5) */
extern const uint8_t seed_have_list;    /* values_list.c (F6): LIST, LMOD = LEGATO */
extern const uint8_t seed_have_noise;   /* noise.c (F7): NOISE values, SHAPE = NOISE */
extern const uint8_t seed_have_drift;   /* timing_drift.c (F8) */
extern const uint8_t seed_have_vary;    /* vary.c (F9): PHRS, CVAR */
extern const uint8_t seed_have_score;   /* score.c (F11): QUAL */

#endif
