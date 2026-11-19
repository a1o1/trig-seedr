/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Fuzzing the chain (plan task M2.2): random originals, contexts and recipes over every source,
 * layer and value source (props.h), checked against the invariants that hold on the host
 * (design section 11): I1 identity, I2 determinism, I3 independence, I4 monotonic shares, I5
 * counts, I6 scope, I10 VARY; and that a take is something the device can write (every value in
 * its field's range, origins inside the track, swing where it was, no refusal of a valid recipe).
 * The device's side (I7 to I9) is replayed in the emulator: tools/tests/41-fuzz-sample.sh.
 *
 *   ./test_fuzz                      the short run inside `make test`: a fixed set of cases
 *   FUZZ_SECONDS=N ./test_fuzz       the long run: cases from FUZZ_SEED (default: the time) for N
 *                                    seconds (tools/fuzz.sh runs it with ASan and UBSan)
 *   FUZZ_CASE=0xHEX ./test_fuzz      one case again, printed (the case number a failure reports)
 *   ./test_fuzz --sample N SEED      N recipes for the emulator (A01 BD, the Rytm), one line each
 *   ./test_fuzz --sample-st N SEED TYPE TRACK
 *                                    N recipes for the Syntakt (tools/tests-st/41-fuzz-sample.sh): lanes over
 *                                    the whole list of TRACK with the machine TYPE, in each parameter's range
 *                                    in tools/seedref's KEY=VALUE form
 *
 * A failure prints its invariant and its case number; FUZZ_CASE replays it. Host only. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "props.h"
#ifndef PROP_DEVICE_ST
#include "../device/st_params.h"
#endif

static int fails, checks;
static long nfail[16];
static const char *const inv_name[16] = { "I1", "I2", "I3", "I4", "I5", "I6", "I10", "range", "refused", "const",
                                          "vary100", "lanes" };
enum { F_I1, F_I2, F_I3, F_I4, F_I5, F_I6, F_I10, F_RANGE, F_REFUSED, F_CONST, F_VARY100, F_LANES };

static uint32_t this_case;
static int verbose, shown;

#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void fail(int what, const char *detail)
{
    nfail[what]++;
    if (shown < 20 || verbose) {
        printf("FAIL fuzz %s: case 0x%08x: %s (FUZZ_CASE=0x%08x)\n", inv_name[what], (unsigned)this_case, detail,
               (unsigned)this_case);
        shown++;
    }
}

/* ---- a second device: 16 tracks, a narrower micro-timing range, lanes with other ranges ---- */

static const struct seed_param odd_params[3] = { { "TUNE", 1, -64, 63 }, { "SW", 2, 0, 1 }, { "WIDE", 3, -1000, 1000 } };
static const struct seed_caps odd_caps = { 16, 64, 0x1ff, -11, 11, SEED_LANES, 3, odd_params, { 0, 2 } };
static const struct seed_caps one_lane = { 8, 64, 0x1ff & ~SEED_F_ACCENT, -23, 23, 1, 3, odd_params, { 0, 0 } };

/* ---- recipes as tools/seedref's keys ---- */

#define K(name, field) { name, offsetof(struct seed_recipe, field) }
#define KV(name, field) K(name ".lo", field.lo), K(name ".hi", field.hi), K(name ".src", field.src), \
    K(name ".order", field.order), K(name ".len", field.len), K(name ".zoom", field.zoom)
static const struct { const char *name; size_t off; } keys[] = {
    K("vary", vary), K("phrs", phrs), K("cvar", cvar), K("metr", metr), K("beat", beat), K("qual", qual),
    K("src", src), K("grid", grid), K("dens", dens), K("bias", bias), K("role", role), K("eu_steps", eu_steps),
    K("eu_puls", eu_puls), K("map_part", map_part), K("map_dens", map_dens), K("copy_trk", copy_trk),
    K("rot", rot), K("sync", sync), K("mask", mask), K("mtrk", mtrk), K("skip", skip), K("fld", fld),
    K("frot", frot), K("fmod", fmod), KV("vel", vel), K("shape", shape), K("amt", amt), K("n", n), K("acc", acc),
    K("amod", amod), K("hum", hum), KV("len", len), K("lmod", lmod), KV("note", note),
    K("lane1.parm", lane[0].parm), K("lane1.pct", lane[0].pct), KV("lane1", lane[0].val),
    K("lane2.parm", lane[1].parm), K("lane2.pct", lane[1].pct), KV("lane2", lane[1].val),
    K("cond", cond), K("ctyp", ctyp), KV("cval", cval), K("rtrg", rtrg), KV("rate", rate), K("rcur", rcur),
    K("grv", grv), K("timing", timing), K("gvel", gvel), K("quant", quant), K("rand", rand), K("drft", drft),
    K("shft", shft),
};
#define NKEYS ((int)(sizeof keys / sizeof keys[0]))

static int16_t *kfield(struct seed_recipe *r, int i) { return (int16_t *)((char *)r + keys[i].off); }

/* the keys that differ from the defaults, as KEY=VALUE (SEED_DEF as def) */
static void print_recipe(FILE *f, const struct seed_recipe *r)
{
    struct seed_recipe d, x = *r;
    int i;
    seed_recipe_defaults(&d, prop_cap);
    fprintf(f, "seed=%u", (unsigned)r->seed);
    for (i = 0; i < NKEYS; i++)
        if (*kfield(&x, i) != *kfield(&d, i)) {
            if (*kfield(&x, i) == SEED_DEF)
                fprintf(f, " %s=def", keys[i].name);
            else
                fprintf(f, " %s=%d", keys[i].name, *kfield(&x, i));
        }
    fprintf(f, "\n");
}

static void print_track(const char *what, const struct seed_track *t)
{
    int i;
    printf("%s: length %d fields 0x%x default note %d vel %d\n", what, t->length, t->fields, t->default_note,
           t->default_vel);
    for (i = 0; i < t->length; i++) {
        const struct seed_step *s = &t->s[i];
        printf("  %2d %04x note %3d vel %3d len %3d ut %3d cond %3d rtrg %3d %2d %4d lane %6d %6d origin %3d\n", i,
               s->flags, s->note, s->vel, s->len, s->utime, s->cond, s->rlen, s->rrate, s->rvel, s->lane[0],
               s->lane[1], s->origin);
    }
}

/* ---- one case ---- */

struct fcase {
    struct seed_track orig;
    struct seed_ctx c;
    struct seed_recipe r;
    int wild;                         /* a knob set outside its range: only I2, I6 and the sanitizers */
};

static struct seed_track out, out2;
static struct seed_work work2;

static int render(struct seed_track *o, const struct fcase *f, const struct seed_recipe *r)
{
    return seed_render(o, &f->orig, r, &f->c, &prop_work);
}

static int trig(const struct seed_track *t, int i) { return (t->s[i].flags & SEED_TRIG) != 0; }

static int ntrigs(const struct seed_track *t)
{
    int i, m = 0;
    for (i = 0; i < t->length; i++)
        m += trig(t, i);
    return m;
}

/* a value outside the knobs' ranges now and then */
static int16_t wild_value(struct prop_rng *g)
{
    static const int16_t v[] = { -32768, -32767, -1000, -129, -128, -101, -100, -65, -64, -63, -2, -1, 0, 1, 2, 7,
                                 8, 9, 15, 16, 17, 21, 22, 63, 64, 65, 99, 100, 101, 126, 127, 128, 255, 256, 1000,
                                 32767 };
    return prop_chance(g, 80) ? v[prop_int(g, 0, (int)(sizeof v / sizeof v[0]) - 1)] : (int16_t)prop_next(g);
}

static void make_case(struct fcase *f, uint32_t id)
{
    struct prop_rng g;
    int len, i, l, k = prop_int(&(struct prop_rng){ seed_hash(id ^ 0x77u) | 1u }, 0, 99);
    g.s = seed_hash(id) | 1u;
    prop_cap = k < 80 ? &prop_caps : k < 92 ? &odd_caps : &one_lane;
    len = prop_chance(&g, 30) ? 16 : prop_chance(&g, 30) ? 64 : prop_int(&g, 1, SEED_STEPS);
    prop_track(&f->orig, &g, len, prop_int(&g, 0, 100));
    if (prop_chance(&g, 10))
        f->orig.fields = prop_chance(&g, 50) ? RT_FX_FIELDS : (uint16_t)(prop_next(&g) & 0x1ffu);
    f->orig.default_note = (uint8_t)prop_int(&g, 0, 126);
    f->orig.default_vel = (uint8_t)prop_int(&g, 1, 127);
    for (i = 0; i < SEED_STEPS; i++) {             /* retrigs and their settings */
        struct seed_step *s = &f->orig.s[i];
        if (prop_chance(&g, 15))
            s->flags |= SEED_RETRIG;
        if (prop_chance(&g, 20)) {
            s->rlen = (uint8_t)prop_int(&g, 0, 127);
            s->rrate = (uint8_t)prop_int(&g, 0, 16);
            s->rvel = (int8_t)prop_int(&g, -128, 127);
        }
        if (prop_chance(&g, 5))
            s->flags |= SEED_MUTE | SEED_SLIDE;
    }
    prop_ctx_full(&f->c, &f->orig, prop_int(&g, 0, prop_cap->tracks - 1), &g);
    prop_recipe(&f->r, &g, prop_int(&g, 20, 90));
    f->wild = prop_chance(&g, 6);
    if (f->wild) {
        int n = prop_int(&g, 1, 3);
        while (n--)
            *kfield(&f->r, prop_int(&g, 0, NKEYS - 1)) = wild_value(&g);
    }
    /* the original's lanes, in the recipe's parameters' ranges, as the device reads them (a lane on
     * no parameter has no locks) */
    for (l = 0; l < SEED_LANES; l++) {
        int parm = f->r.lane[l].parm, share = prop_int(&g, 0, 60);
        const struct seed_param *p = parm >= 0 && parm < prop_cap->nparams ? &prop_cap->params[parm] : 0;
        for (i = 0; i < SEED_STEPS; i++)
            f->orig.s[i].lane[l] = p && prop_chance(&g, share) ? (int16_t)prop_int(&g, p->lo, p->hi) : SEED_NO_LOCK;
    }
}

/* every value of the take is one the device can store, given an original that is */
static int well_formed(const struct seed_track *t, const struct fcase *f, char *why, size_t n)
{
    const struct seed_caps *cp = f->c.caps;
    int i, l;
    for (i = 0; i < t->length; i++) {
        const struct seed_step *s = &t->s[i];
        const char *bad = 0;
        if (s->origin < SEED_NO_ORIGIN || s->origin >= t->length) bad = "origin";
        else if ((s->flags ^ f->orig.s[i].flags) & SEED_SWING) bad = "swing moved";
        else if (s->vel != SEED_DEFAULT && (s->vel < 1 || s->vel > 127)) bad = "vel";
        else if (s->note != SEED_DEFAULT && s->note > 127) bad = "note";
        else if (s->len != SEED_DEFAULT && s->len > 127) bad = "len";
        else if (s->utime < cp->utime_lo || s->utime > cp->utime_hi) bad = "utime";
        else if (s->cond != SEED_NO_COND && (s->cond < 0 || s->cond > 64)) bad = "cond";
        else if (s->rlen > 127 || s->rrate > 16) bad = "retrig";
        else if (s->spare) bad = "spare";
        for (l = 0; !bad && l < cp->lanes && l < SEED_LANES; l++) {
            int parm = f->r.lane[l].parm;
            const struct seed_param *p = &cp->params[parm < 0 || parm >= cp->nparams ? 0 : parm];
            if (parm < 0 || parm >= cp->nparams)
                continue;                   /* the device writes no lane on no parameter */
            if (s->lane[l] != SEED_NO_LOCK && (s->lane[l] < p->lo || s->lane[l] > p->hi))
                bad = l ? "lane 2" : "lane 1";
        }
        if (bad) {
            snprintf(why, n, "step %d: %s", i, bad);
            return 0;
        }
    }
    return 1;
}

/* the recipe with every layer at 0 (V4), the TAKE knobs and every other setting kept */
static void all_off(struct seed_recipe *r, struct prop_rng *g)
{
    int l;
    r->src = SEED_SRC_TRACK;
    r->rot = r->sync = r->skip = 0;
    r->mask = SEED_MASK_OFF;
    r->vel.lo = SEED_DEF;
    if (prop_chance(g, 50)) r->shape = SEED_SHAPE_FLAT; else r->amt = 0;
    r->acc = r->hum = 0;
    r->len.lo = r->note.lo = SEED_DEF;
    r->lmod = SEED_LMOD_RANGE;
    for (l = 0; l < SEED_LANES; l++)
        r->lane[l].pct = 0;
    r->cond = r->rtrg = 0;
    r->grv = r->quant = r->rand = r->drft = 0;
    if (prop_chance(g, 50)) r->fld = SEED_FLD_OFF; else r->frot = 0;
}

/* I4: the steps a share knob has chosen at a lower value are chosen at a higher one */
enum { S_DENS, S_MAP, S_SYNC, S_ACC, S_COND, S_RTRG, S_LANE1, S_LANE2, S_N };
static const char *const share_name[S_N] = { "DENS", "MAP DENS", "SYNC", "ACC", "COND", "RTRG", "LANE1 %", "LANE2 %" };

static void set_share(struct seed_recipe *r, int knob, int v)
{
    switch (knob) {
    case S_DENS: r->dens = (int16_t)v; break;
    case S_MAP: r->map_dens = (int16_t)v; break;
    case S_SYNC: r->sync = (int16_t)v; break;
    case S_ACC: r->acc = (int16_t)v; break;
    case S_COND: r->cond = (int16_t)v; break;
    case S_RTRG: r->rtrg = (int16_t)v; break;
    case S_LANE1: r->lane[0].pct = (int16_t)v; break;
    default: r->lane[1].pct = (int16_t)v; break;
    }
}

/* step i is chosen: a trig (DENS), a step SYNC emptied (against the take at SYNC 0), or a trig with
 * the knob's mark */
static int chosen(const struct seed_track *t, const struct seed_track *base, int knob, int i)
{
    const struct seed_step *s = &t->s[i];
    switch (knob) {
    case S_DENS: case S_MAP: return trig(t, i);
    case S_SYNC: return trig(base, i) && !trig(t, i);
    case S_ACC: return trig(t, i) && (s->flags & SEED_ACCENT);
    case S_COND: return trig(t, i) && s->cond != SEED_NO_COND;
    case S_RTRG: return trig(t, i) && (s->flags & SEED_RETRIG);
    case S_LANE1: return trig(t, i) && s->lane[0] != SEED_NO_LOCK;
    default: return trig(t, i) && s->lane[1] != SEED_NO_LOCK;
    }
}

static long i4_grew[S_N], i4_tried[S_N];

static void check_i4(const struct fcase *f, struct prop_rng *g)
{
    static struct seed_track a, b, base;
    struct seed_recipe r = f->r;
    int knob = prop_int(g, 0, S_N - 1), lo, hi, v, prev = -1, i;
    char why[96];
    r.qual = 1;                                 /* another share can change which candidate wins */
    switch (knob) {
    case S_DENS: r.src = SEED_SRC_RANDOM; r.sync = 0; lo = 0; hi = 100; break;
    case S_MAP: r.src = SEED_SRC_MAP; r.sync = 0; lo = 0; hi = 255; break;
    case S_SYNC: r.cvar = SEED_CVAR_OFF; lo = 0; hi = 100; break;  /* CVAR: two takes' moves on one track */
    case S_LANE2: if (f->c.caps->lanes < 2) return; /* fall through */
    default: lo = 1; hi = 100; break;           /* above 0 the layer owns its field */
    }
    set_share(&r, knob, 0);
    if (render(&base, f, &r) != SEED_OK)
        return;
    i4_tried[knob]++;
    for (v = lo; v <= hi; v += prop_int(g, 1, hi / 8)) {
        int m = 0;
        set_share(&r, knob, v);
        if (render(&b, f, &r) != SEED_OK) {
            snprintf(why, sizeof why, "%s %d refused", share_name[knob], v);
            fail(F_I4, why);
            return;
        }
        for (i = 0; i < b.length; i++) {
            if (prev >= 0 && chosen(&a, &base, knob, i) && !chosen(&b, &base, knob, i)) {
                snprintf(why, sizeof why, "%s %d dropped step %d", share_name[knob], v, i);
                fail(F_I4, why);
                return;
            }
            m += chosen(&b, &base, knob, i);
        }
        i4_grew[knob] += prev >= 0 && m > prev;
        prev = m;
        a = b;
    }
}

/* I3 within a layer (P1, V4): each part of a layer at 0 leaves the fields the layer's other parts
 * own; and across RHYTHM, a trig that is there with another DENS keeps the values drawn for its
 * step */
static void check_parts(const struct fcase *f, struct prop_rng *g)
{
    static struct seed_track a, b;
    static const unsigned owns[] = { PROP_VEL, PROP_FLAGS, PROP_LEN, PROP_NOTE, PROP_LANE1, PROP_LANE2, PROP_COND,
                                     PROP_RTRG | PROP_FLAGS, PROP_UTIME, PROP_VEL };
    static const char *const name[] = { "DYN velocity", "ACC", "LEN", "NOTE", "lane 1", "lane 2", "COND", "RTRG",
                                        "TIME micro timing", "GVEL" };
    struct seed_recipe r = f->r;
    int part = prop_int(g, 0, 9), i;
    unsigned d;
    char why[96];
    if (render(&a, f, &r) != SEED_OK)
        return;
    switch (part) {
    case 0: r.vel.lo = SEED_DEF; r.amt = 0; r.hum = 0; break;
    case 1: r.acc = 0; break;
    case 2: r.len.lo = SEED_DEF; r.lmod = SEED_LMOD_RANGE; break;
    case 3: r.note.lo = SEED_DEF; break;
    case 4: r.lane[0].pct = 0; break;
    case 5: r.lane[1].pct = 0; break;
    case 6: r.cond = 0; break;
    case 7: r.rtrg = 0; break;
    case 8: r.quant = r.rand = r.drft = 0; r.timing = 0; break;
    default: r.gvel = 0; break;
    }
    if (render(&b, f, &r) == SEED_OK) {
        d = prop_diff(&a, &b, f->orig.length);
        if (part == 1 || part == 7) {             /* the accent and the retrig flag: which bit moved */
            for (i = 0; i < f->orig.length; i++)
                if ((a.s[i].flags ^ b.s[i].flags) & (part == 1 ? SEED_KNOWN & ~SEED_ACCENT : SEED_KNOWN & ~SEED_RETRIG))
                    d |= PROP_TRIG;
        } else if ((d & PROP_FLAGS) && !(owns[part] & PROP_FLAGS)) {
            d |= PROP_TRIG;
        }
        if (d & ~owns[part]) {
            snprintf(why, sizeof why, "%s at 0 changed fields 0x%x", name[part], d & ~owns[part]);
            fail(F_I3, why);
        }
    }
    /* across RHYTHM: values drawn by step (RANGE, NOISE; not LIST, EVRY, LEGATO or FLD, which go by
     * the trigs' order) stay with a trig that is there with another DENS */
    r = f->r;
    r.src = SEED_SRC_RANDOM;
    r.sync = 0;
    r.cvar = SEED_CVAR_OFF;
    r.qual = 1;
    r.fld = SEED_FLD_OFF;
    r.lmod = SEED_LMOD_RANGE;
    if (r.shape == SEED_SHAPE_EVRY)
        r.shape = SEED_SHAPE_STRONG;
    if (r.vel.src == SEED_VSRC_LIST) r.vel.src = SEED_VSRC_RANGE;
    if (r.len.src == SEED_VSRC_LIST) r.len.src = SEED_VSRC_RANGE;
    if (r.note.src == SEED_VSRC_LIST) r.note.src = SEED_VSRC_RANGE;
    if (render(&a, f, &r) != SEED_OK)
        return;
    r.dens = (int16_t)prop_int(g, 0, 100);
    if (render(&b, f, &r) != SEED_OK)
        return;
    for (i = 0; i < a.length; i++)
        if (trig(&a, i) && trig(&b, i)
            && (a.s[i].vel != b.s[i].vel || a.s[i].len != b.s[i].len || a.s[i].note != b.s[i].note
                || a.s[i].utime != b.s[i].utime)) {
            snprintf(why, sizeof why, "DENS %d: step %d's values changed", r.dens, i);
            fail(F_I3, why);
            return;
        }
}

/* I5: RANDOM's DENS and EUCLID give their exact counts at the source, SYNC keeps the count, MASK
 * and SKIP only remove */
static void check_i5(const struct fcase *f, struct prop_rng *g)
{
    static struct seed_track a, b;
    struct seed_recipe r = f->r;
    int i, n = f->orig.length, want, got;
    char why[96];
    r.phrs = SEED_PHRS_OFF;
    r.cvar = SEED_CVAR_OFF;
    r.mask = SEED_MASK_OFF;
    r.skip = 0;
    if (prop_chance(g, 50)) {
        r.src = SEED_SRC_RANDOM;
        want = seed_share(r.dens, (n + (r.grid == 2 || r.grid == 4 || r.grid == 8 ? r.grid : 1) - 1)
                                  / (r.grid == 2 || r.grid == 4 || r.grid == 8 ? r.grid : 1));
    } else {
        r.src = SEED_SRC_EUCLID;
        r.eu_steps = (int16_t)n;
        r.eu_puls = (int16_t)prop_int(g, 0, n);
        want = r.eu_puls;
    }
    if (render(&a, f, &r) != SEED_OK)
        return;
    got = ntrigs(&a);
    if (got != want) {
        snprintf(why, sizeof why, "%s: %d trigs, wanted %d", r.src == SEED_SRC_RANDOM ? "DENS" : "EUCLID", got, want);
        fail(F_I5, why);
    }
    /* SYNC keeps the count (with SKIP: no trig crosses it). Not with MASK after it (a moved trig
     * meets the other track at its new step), nor on a last, shorter page of PHRS (the page is a
     * 16-step phrase, and it plays the phrase's start) */
    r = f->r;
    r.cvar = SEED_CVAR_OFF;
    r.mask = SEED_MASK_OFF;
    r.qual = 1;
    r.sync = 0;
    if (n > SEED_PAGE && n % SEED_PAGE)
        r.phrs = SEED_PHRS_OFF;
    if (render(&a, f, &r) == SEED_OK) {
        r.sync = prop_pct(g, 0);
        if (render(&b, f, &r) == SEED_OK && ntrigs(&a) != ntrigs(&b)) {
            snprintf(why, sizeof why, "SYNC %d: %d trigs, %d at SYNC 0", r.sync, ntrigs(&b), ntrigs(&a));
            fail(F_I5, why);
        }
    }
    /* MASK and SKIP only remove: the trigs with them are among those without (SYNC off: it moves
     * no trig across SKIP, so SKIP changes which trigs it moves) */
    r = f->r;
    r.qual = 1;
    r.sync = 0;
    if (r.mask == SEED_MASK_OFF && r.skip == 0)
        return;
    if (render(&b, f, &r) != SEED_OK)
        return;
    r.mask = SEED_MASK_OFF;
    r.skip = 0;
    if (render(&a, f, &r) != SEED_OK)
        return;
    for (i = 0; i < n; i++)
        if (trig(&b, i) && !trig(&a, i)) {
            snprintf(why, sizeof why, "MASK / SKIP added a trig at step %d", i);
            fail(F_I5, why);
            return;
        }
}

/* I10 on the slots: the share of slots take n + 1 keeps from take n, by VARY */
static long kept_same[11], kept_all[11];

static void check_i10_slots(struct prop_rng *g)
{
    struct seed_take a, b;
    int vary = prop_chance(g, 20) ? (prop_chance(g, 50) ? 0 : 100) : prop_int(g, 0, 100), s;
    uint32_t n = prop_chance(g, 10) ? (uint32_t)prop_int(g, -70, 70) : prop_next(g);
    uint32_t salt = (uint32_t)prop_int(g, 0, 15);
    int cand = prop_int(g, 0, 7);
    seed_take_init(&a, n, salt, vary, cand);
    seed_take_init(&b, n + 1u, salt, vary, cand);
    for (s = 0; s < 32; s++) {
        uint32_t slot = SEED_SLOT(prop_int(g, 0, SEED_L_N - 1), prop_int(g, 0, SEED_P_N - 1), prop_int(g, 0, 0xffff));
        int same = seed_rand(&a, slot) == seed_rand(&b, slot);
        if ((vary == 100 && !same) || (vary == 0 && same && seed_rand(&a, slot + 1) == seed_rand(&b, slot + 1))) {
            char why[96];
            snprintf(why, sizeof why, "VARY %d, take %u, slot 0x%08x: kept %d", vary, (unsigned)n, (unsigned)slot, same);
            fail(F_I10, why);
        }
        if (vary > 0 && vary < 100) {
            kept_same[vary / 10] += same;
            kept_all[vary / 10]++;
        }
    }
}

static long n_src[SEED_SRC_N], n_shape[SEED_SHAPE_N], n_vsrc[SEED_VSRC_N], n_phrs, n_cvar, n_qual, n_wild,
            n_refused, n_cases, n_sync, n_mask, n_fld, n_grv, n_legato, n_top, n_lane2;

static void count(const struct seed_recipe *r)
{
    const struct seed_values *v[] = { &r->vel, &r->len, &r->note, &r->lane[0].val, &r->lane[1].val, &r->cval,
                                      &r->rate };
    unsigned i;
    if (r->src >= 0 && r->src < SEED_SRC_N) n_src[r->src]++;
    if (r->shape >= 0 && r->shape < SEED_SHAPE_N && r->amt > 0) n_shape[r->shape]++;
    for (i = 0; i < sizeof v / sizeof v[0]; i++)
        if (v[i]->lo != SEED_DEF && v[i]->src >= 0 && v[i]->src < SEED_VSRC_N) n_vsrc[v[i]->src]++;
    n_phrs += r->phrs != SEED_PHRS_OFF;
    n_cvar += r->cvar != SEED_CVAR_OFF;
    n_qual += r->qual > 1;
    n_sync += r->sync > 0;
    n_mask += r->mask != SEED_MASK_OFF;
    n_fld += r->fld != SEED_FLD_OFF && r->frot != 0;
    n_grv += r->grv > 0;
    n_legato += r->lmod == SEED_LMOD_LEGATO;
    n_top += r->acc > 0 && r->amod == SEED_AMOD_TOP;
    n_lane2 += r->lane[1].pct > 0;
}

static void run_case(uint32_t id)
{
    static struct fcase f, keep;
    struct prop_rng g;
    char why[96];
    int rc, rc2, L;
    unsigned ch;

    this_case = id;
    make_case(&f, id);
    keep = f;
    g.s = seed_hash(id ^ 0xc0ffee11u) | 1u;
    n_cases++;
    if (verbose) {
        printf("case 0x%08x: track %d of %d, caps %s, %swild\n  ", (unsigned)id, f.c.track, f.c.tracks,
               prop_cap == &prop_caps ? "Rytm" : prop_cap == &odd_caps ? "odd" : "one lane", f.wild ? "" : "not ");
        print_recipe(stdout, &f.r);
        printf("  map_x=%d map_y=%d map_chaos=%d\n", f.c.shared.map_x, f.c.shared.map_y, f.c.shared.map_chaos);
        print_track("original", &f.orig);
    }

    /* I2: twice, from scratch buffers full of something else, the same bytes and result */
    memset(&out, 0x5a, sizeof out);
    rc = render(&out, &f, &f.r);
    memset(&work2, (int)(prop_next(&g) & 0xff), sizeof work2);
    memset(&out2, 0xa5, sizeof out2);
    rc2 = seed_render(&out2, &f.orig, &f.r, &f.c, &work2);
    if (verbose) {
        printf("result %d\n", rc);
        if (rc == SEED_OK)
            print_track("take", &out);
    }
    if (rc != rc2 || (rc == SEED_OK && memcmp(&out, &out2, sizeof out))) {
        snprintf(why, sizeof why, "results %d and %d", rc, rc2);
        fail(F_I2, why);
    }
    if (memcmp(&keep, &f, sizeof f))
        fail(F_CONST, "the original, context or recipe changed");
    /* the context is its tracks' trigs below their lengths: bits past them change nothing */
    {
        struct fcase h = f;
        int j, s;
        for (j = 0; j < SEED_TRACKS; j++)
            for (s = j < h.c.tracks ? h.c.length[j] : 0; s < SEED_STEPS; s++)
                if (prop_chance(&g, 50))
                    h.c.trig[j][s >> 3] |= (uint8_t)(1u << (s & 7));
        rc2 = render(&out2, &h, &f.r);
        if (rc != rc2 || (rc == SEED_OK && memcmp(&out, &out2, sizeof out)))
            fail(F_I2, "trig bits past the other tracks' lengths changed the take");
    }
    if (rc != SEED_OK) {
        n_refused++;
        /* a valid recipe is refused only when the track lacks CVAR's conditions */
        if (!f.wild && !(f.r.cvar != SEED_CVAR_OFF && !(f.orig.fields & SEED_F_COND))) {
            snprintf(why, sizeof why, "result %d", rc);
            fail(F_REFUSED, why);
        }
    } else {
        int i;
        /* I6: the length respected; steps at and past it and the track's settings unchanged */
        for (i = f.orig.length; i < SEED_STEPS; i++)
            if (memcmp(&out.s[i], &f.orig.s[i], sizeof out.s[i])) {
                snprintf(why, sizeof why, "step %d past the length %d changed", i, f.orig.length);
                fail(F_I6, why);
                break;
            }
        if (out.length != f.orig.length || out.fields != f.orig.fields || out.default_note != f.orig.default_note
            || out.default_vel != f.orig.default_vel)
            fail(F_I6, "the track's length, fields or defaults changed");
        if (!well_formed(&out, &f, why, sizeof why))
            fail(F_RANGE, why);
        /* I10: at VARY 100 every take is the same */
        if (!f.wild) {
            struct seed_recipe r = f.r;
            r.vary = 100;
            if (render(&out, &f, &r) == SEED_OK) {
                r.seed++;
                if (render(&out2, &f, &r) != SEED_OK || memcmp(&out, &out2, sizeof out))
                    fail(F_VARY100, "VARY 100: take n + 1 differs from take n");
            }
        }
    }
    if (f.wild) {
        n_wild++;
        return;
    }
    count(&f.r);

    /* I1: every layer at 0, whatever the TAKE knobs and the other knobs say */
    {
        struct seed_recipe r = f.r;
        all_off(&r, &g);
        for (L = SEED_L_RHYTHM; L < SEED_L_N; L++)
            if (seed_layer_active(&r, (enum seed_layer)L)) {
                snprintf(why, sizeof why, "layer %d active with its knobs at 0", L);
                fail(F_I1, why);
            }
        rc = render(&out, &f, &r);
        if (rc != SEED_OK && (f.orig.fields & SEED_F_COND)) {
            snprintf(why, sizeof why, "every layer at 0: result %d", rc);
            fail(F_I1, why);
        } else if (rc == SEED_OK && memcmp(&out, &f.orig, sizeof out)) {
            fail(F_I1, "every layer at 0: the track changed");
        }
    }
    /* I3: each layer drawn again changes only its own fields */
    for (L = SEED_L_PLACE; L < SEED_L_N; L++)
        if (!prop_i3(&f.orig, &f.r, &f.c, &g, (enum seed_layer)L, &ch)) {
            snprintf(why, sizeof why, "layer %d changed fields 0x%x", L, ch & ~prop_owned((enum seed_layer)L));
            fail(F_I3, why);
        }
    /* two lanes on one parameter are one lock: a take is refused, or its two lanes agree (the device
     * writes the parameter once, from one of them) */
    if (f.c.caps->lanes >= 2 && prop_chance(&g, 25)) {
        struct seed_recipe r = f.r;
        int i;
        r.lane[1].parm = r.lane[0].parm;
        for (i = 0; i < SEED_STEPS; i++)
            f.orig.s[i].lane[1] = f.orig.s[i].lane[0];
        if (render(&out, &f, &r) == SEED_OK)
            for (i = 0; i < out.length; i++)
                if (out.s[i].lane[0] != out.s[i].lane[1]) {
                    snprintf(why, sizeof why, "lanes 1 and 2 on parameter %d differ at step %d", r.lane[0].parm, i);
                    fail(F_LANES, why);
                    break;
                }
        f.orig = keep.orig;
    }
    check_parts(&f, &g);
    check_i4(&f, &g);
    check_i5(&f, &g);
    check_i10_slots(&g);
}

/* ---- failures the fuzzer found, reduced (docs/status/M2.2.md, findings) ---- */

/* Finding 1: two lanes on one parameter. The device stores one lock per parameter and step and
 * takes the later lane's value (device/rytm_ar2.h rt_seed_target_lock), so lane 1 at 100 % on SMP
 * with lane 2 left on SMP wrote lane 2's (the original's) locks back: the take on the device was
 * not the host's. Now LOCK with either lane set, and FLD on either lane, refuse (SEED_E_ARG). */
static void test_found_lanes(void)
{
    static struct seed_track t, o;
    struct prop_rng g = { 1 };
    struct seed_recipe r;
    struct seed_ctx c;
    int i;
    prop_cap = &prop_caps;
    prop_track(&t, &g, 16, 50);
    for (i = 0; i < SEED_STEPS; i++)
        t.s[i].lane[1] = t.s[i].lane[0];        /* one parameter, read twice */
    prop_ctx(&c, &t, 0);
    seed_recipe_defaults(&r, &prop_caps);
    r.lane[0].pct = 100;
    r.lane[0].val.lo = 10;
    r.lane[0].val.hi = 20;
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_OK);          /* SMP and FRQ */
    r.lane[1].parm = r.lane[0].parm;
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_E_ARG);       /* both on SMP, lane 1 set */
    r.lane[0].pct = 0;
    r.lane[1].pct = 40;
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_E_ARG);       /* lane 2 set */
    r.lane[1].pct = 0;
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_OK);          /* neither: they move together */
    CHECK(memcmp(&o, &t, sizeof o) == 0);
    r.fld = SEED_FLD_LANE1;
    r.frot = 3;
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_E_ARG);       /* FLD on one of them */
    r.fld = SEED_FLD_VEL;
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_OK);
    /* a device with one lane: lane 2's parameter is not there */
    prop_cap = &one_lane;
    prop_ctx(&c, &t, 0);
    seed_recipe_defaults(&r, &one_lane);
    r.lane[0].pct = 100;
    CHECK(r.lane[1].parm == r.lane[0].parm);
    CHECK(seed_render(&o, &t, &r, &c, &prop_work) == SEED_OK);
    prop_cap = &prop_caps;
}

/* ---- the emulator's sample ---- */

/* N recipes for A01 BD on the Rytm (tools/tests/41-fuzz-sample.sh): lane 1 on the sample slot (the
 * dump shows it), lane 2 on another parameter, grooves 1..3 of the mod's, a seed the mod's knob can
 * show; the shared MAP settings as seedref's map_x, map_y, map_chaos */
static void sample(int n, uint32_t seed)
{
    struct prop_rng g = { seed | 1u };
    struct seed_recipe r;
    prop_cap = &prop_caps;
    while (n-- > 0) {
        prop_recipe(&r, &g, prop_int(&g, 40, 90));
        r.seed %= 10000u;
        r.lane[0].parm = RT_P_SMP;
        if (r.lane[1].parm == RT_P_SMP)
            r.lane[1].parm = RT_P_FRQ;
        if (r.lane[0].val.lo != SEED_DEF) {
            if (r.lane[0].val.lo < 0) r.lane[0].val.lo = 0;
            if (r.lane[0].val.hi < 0) r.lane[0].val.hi = 0;
        }
        printf("map_x=%d map_y=%d map_chaos=%d ", prop_int(&g, 0, 255), prop_int(&g, 0, 255),
               prop_chance(&g, 50) ? 0 : prop_int(&g, 0, 255));
        print_recipe(stdout, &r);
    }
}

/* N recipes for a Syntakt track (tools/tests-st/41-fuzz-sample.sh; task P1): the track's own list for the
 * machine TYPE, as st_track_params builds it for the device (SD4), so both lanes run over its SYN controls and
 * pages and take values in each parameter's range; the rest as sample()'s. A lane on a parameter the other
 * lane has, which the engine refuses, stays in the sample: the test checks the refusal writes nothing. */
static void sample_st(int n, uint32_t seed, int type, int track)
{
    static const struct st_machine m[ST_NMACHINES] = ST_MACHINES;
    static struct seed_param params[ST_TRACK_PARAMS_MAX];
    static struct seed_caps caps;
    uint16_t ids[ST_TRACK_PARAMS_MAX];
    struct prop_rng g = { seed | 1u };
    struct seed_recipe r;
    caps.tracks = ST_TRACKS;
    caps.steps = SEED_STEPS;
    caps.fields = track == ST_FX_TRACK ? ST_FX_FIELDS : ST_VOICE_FIELDS;
    caps.utime_lo = ST_UTIME_LO;
    caps.utime_hi = ST_UTIME_HI;
    caps.lanes = SEED_LANES;
    caps.params = params;
    caps.nparams = (uint8_t)st_track_params(st_machine_of(m, ST_NMACHINES, type), track, params, ids, caps.lane_parm);
    prop_cap = &caps;
    while (n-- > 0) {
        prop_recipe(&r, &g, prop_int(&g, 40, 90));
        r.seed %= 10000u;
        printf("map_x=%d map_y=%d map_chaos=%d ", prop_int(&g, 0, 255), prop_int(&g, 0, 255),
               prop_chance(&g, 50) ? 0 : prop_int(&g, 0, 255));
        print_recipe(stdout, &r);
    }
}

int main(int argc, char **argv)
{
    const char *secs = getenv("FUZZ_SECONDS"), *one = getenv("FUZZ_CASE"), *sd = getenv("FUZZ_SEED");
    uint32_t base = sd ? (uint32_t)strtoul(sd, 0, 0) : 0x4d322e32u, id;
    long cases = 0, limit = 4000, i;
    time_t start = time(0), report = start;
    int v;

    if (argc == 4 && !strcmp(argv[1], "--sample")) {
        sample(atoi(argv[2]), (uint32_t)strtoul(argv[3], 0, 0));
        return 0;
    }
    if (argc == 6 && !strcmp(argv[1], "--sample-st")) {
        sample_st(atoi(argv[2]), (uint32_t)strtoul(argv[3], 0, 0), atoi(argv[4]), atoi(argv[5]));
        return 0;
    }
    if (one) {
        verbose = 1;
        run_case((uint32_t)strtoul(one, 0, 0));
        for (v = 0; v < 16; v++)
            fails += (int)nfail[v];
        printf("test_fuzz: case 0x%08x, %d failed\n", (unsigned)this_case, fails);
        return fails != 0;
    }
    test_found_lanes();
    if (secs && !sd)
        base = (uint32_t)start * 2654435761u;
    if (secs)
        printf("test_fuzz: long run, %s s, FUZZ_SEED=0x%08x\n", secs, (unsigned)base);
    for (i = 0;; i++) {
        id = seed_hash(base + (uint32_t)i);
        run_case(id);
        cases++;
        if (!secs && cases >= limit)
            break;
        if (secs && (i & 63) == 0) {
            time_t now = time(0);
            if (now - start >= atol(secs))
                break;
            if (now - report >= 300) {
                long f = 0;
                for (v = 0; v < 16; v++)
                    f += nfail[v];
                printf("  %ld s: %ld cases, %ld failures\n", (long)(now - start), cases, f);
                fflush(stdout);
                report = now;
            }
        }
    }

    /* coverage: every source, shape and value source was drawn */
    printf("fuzz: %ld cases (%ld with a knob out of range, %ld refused)\n", n_cases, n_wild, n_refused);
    printf("  sources TRACK %ld RANDOM %ld EUCLID %ld MAP %ld COPY %ld; PHRS %ld CVAR %ld QUAL %ld SYNC %ld MASK %ld FLD %ld"
           " GRV %ld LEGATO %ld TOP %ld lane 2 %ld\n", n_src[0], n_src[1], n_src[2], n_src[3], n_src[4], n_phrs, n_cvar,
           n_qual, n_sync, n_mask, n_fld, n_grv, n_legato, n_top, n_lane2);
    printf("  shapes");
    for (v = 0; v < SEED_SHAPE_N; v++)
        printf(" %ld", n_shape[v]);
    printf("; value sources RANGE %ld LIST %ld NOISE %ld\n", n_vsrc[0], n_vsrc[1], n_vsrc[2]);
    for (v = 0; v < S_N; v++)
        printf("%s %s %ld/%ld", v ? "," : "  I4 grew / tried:", share_name[v], i4_grew[v], i4_tried[v]);
    printf("\n  I10 kept by VARY decile:");
    for (v = 1; v < 10; v++) {
        long pct = kept_all[v] ? (kept_same[v] * 1000 / kept_all[v] + 5) / 10 : -1;
        printf(" %ld", pct);
        /* a decile's VARY values average its middle: kept within 3 points of it */
        if (kept_all[v] > 2000 && (pct < v * 10 + 5 - 3 || pct > v * 10 + 5 + 3)) {
            char why[64];
            snprintf(why, sizeof why, "VARY %d..%d kept %ld %%", v * 10, v * 10 + 9, pct);
            fail(F_I10, why);
        }
    }
    printf(" %%\n  failures:");
    for (v = 0; v <= F_LANES; v++)
        printf(" %s %ld", inv_name[v], nfail[v]);
    printf("\n");
    for (v = 0; v < 16; v++)
        CHECK(nfail[v] == 0);
    for (v = 0; v < SEED_SRC_N; v++)
        CHECK(n_src[v] > 0);
    for (v = 0; v < SEED_SHAPE_N; v++)
        CHECK(n_shape[v] > 0);
    for (v = 0; v < SEED_VSRC_N; v++)
        CHECK(n_vsrc[v] > 0);
    CHECK(n_phrs > 0 && n_cvar > 0 && n_qual > 0 && n_sync > 0 && n_mask > 0 && n_fld > 0 && n_grv > 0
          && n_legato > 0 && n_top > 0 && n_lane2 > 0);
    printf("test_fuzz: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
