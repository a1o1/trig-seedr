/* SPDX-License-Identifier: GPL-2.0-or-later */
/* seedref: the version 1 chain (lib/seed_render.c) on the host. A recipe as key=value, the take
 * as a step dump, for comparing with the emulator (tools/emu.py dump prints the same format) and
 * for the golden vectors (tools/golden.sh).
 *
 *   seedref [KEY=VALUE ...]        render and print the take
 *   seedref --keys                 every key with its default
 *   seedref --fields KEY=VALUE...  the recipe fields those keys set, as "C-member-path value"
 *   seedref --recipe KEY=VALUE...  every recipe field, the defaults with those keys applied, in
 *                                  the same form (writing a whole recipe into the mod's store:
 *                                  tools/emu.sh emu_recipe)
 *   seedref --layout               every field's C path, host offset and size (checked against the
 *                                  cross compiler's by tools/tests/00-host.sh)
 *
 * Recipe keys: every field of struct seed_recipe (lib/seed.h) by its name. A value field (vel,
 * len, note, cval, rate, lane1, lane2) takes NAME=LO:HI, NAME=V or NAME=def, and its parts as
 * NAME.lo, .hi, .src, .order, .len, .zoom; a lane also .parm and .pct. Enums take their names
 * (src=random, shape=not1, metr=6/8) or numbers. The recipe starts at seed_recipe_defaults (every
 * layer at 0, SOURCE = TRACK), or at seed_recipe_init with init=1 (V6: RANDOM on an empty track).
 *
 * Context keys:
 *   orig=FILE      the original track as a dump (tools/emu.py dump); default: a new project's
 *                  track (no trigs, swing on every 2nd step, fields at their defaults)
 *   length=N       the original's length (default 16, or the dump's)
 *   track=N        the track in scope (0 = BD), mixed into the take's numbers
 *   other=N:FILE   another track's trigs, for MASK and COPY (repeatable)
 *   map_x, map_y, map_chaos   the pattern's shared MAP settings
 *   all=1          also print origin and lane2 (the golden vectors)
 *   yolo=ROLL:WILD TOOLS YOLO (lib/yolo.c): the recipe rolled from the one the other keys make, for
 *                  the track in scope (its role: device/rytm_params.h), as the mod rolls it
 * The capabilities are the Rytm's (device/rytm_params.h); grooves (grv=N) are the mod's tables.
 *
 * Output, one line per field, one value per step below the length: length; trig (x or .); flags
 * (the SEED_KNOWN and SWING bits, hex); note, vel, len (- = the track's default); utime; cond
 * (- = none); rlen; rrate; rvel; lane1 (- = no lock). Exit status 3 and "result N" on stderr when
 * seed_render refuses (SEED_E_NA: a layer is active whose feature is not built yet). */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/seed.h"
#include "../device/rytm_params.h"

static const char *const n_src[] = { "track", "random", "euclid", "map", "copy", 0 };
static const char *const n_role[] = { "meter", "kick", "back", "hat", "perc", 0 };
static const char *const n_metr[] = { "4/4", "3/4", "6/8", "12/16", "5/8", "7/8", 0 };
static const char *const n_phrs[] = { "off", "aaaa", "aaab", "abab", "abac", "abcd", 0 };
static const char *const n_cvar[] = { "off", "1:2", "aaab", 0 };
static const char *const n_mask[] = { "off", "avoid", "only", 0 };
static const char *const n_fld[] = { "off", "vel", "note", "len", "utime", "cond", "rtrg", "acc", "lane1", "lane2",
                                     "all", 0 };
static const char *const n_fmod[] = { "trig", "grid", 0 };
static const char *const n_shape[] = { "flat", "strong", "weak", "off", "not1", "notbar", "rank", "noise", "evry",
                                       "map", 0 };
static const char *const n_amod[] = { "rand", "top", 0 };
static const char *const n_lmod[] = { "range", "legato", 0 };
static const char *const n_vsrc[] = { "range", "list", "noise", 0 };
static const char *const n_order[] = { "up", "down", "ping", "png2", "odd", "rand", 0 };
static const char *const n_ctyp[] = { "prob", "ab", "1st", "fill", "mix", 0 };

struct key {
    const char *name;
    size_t off;
    const char *const *names;           /* an enum's value names, or NULL */
    const char *path;                   /* the C member path in struct seed_recipe */
};

#define F(name, field, names) { name, offsetof(struct seed_recipe, field), names, #field }
#define V(name, field) F(name ".lo", field.lo, NULL), F(name ".hi", field.hi, NULL), F(name ".src", field.src, n_vsrc), \
    F(name ".order", field.order, n_order), F(name ".len", field.len, NULL), F(name ".zoom", field.zoom, NULL)

static const struct key keys[] = {
    F("vary", vary, NULL), F("phrs", phrs, n_phrs), F("cvar", cvar, n_cvar), F("metr", metr, n_metr),
    F("beat", beat, NULL), F("qual", qual, NULL),
    F("src", src, n_src), F("grid", grid, NULL), F("dens", dens, NULL), F("bias", bias, NULL),
    F("role", role, n_role), F("eu_steps", eu_steps, NULL), F("eu_puls", eu_puls, NULL),
    F("map_part", map_part, NULL), F("map_dens", map_dens, NULL), F("copy_trk", copy_trk, NULL),
    F("rot", rot, NULL), F("sync", sync, NULL), F("mask", mask, n_mask), F("mtrk", mtrk, NULL),
    F("skip", skip, NULL), F("fld", fld, n_fld), F("frot", frot, NULL), F("fmod", fmod, n_fmod),
    V("vel", vel), F("shape", shape, n_shape), F("amt", amt, NULL), F("n", n, NULL), F("acc", acc, NULL),
    F("amod", amod, n_amod), F("hum", hum, NULL),
    V("len", len), F("lmod", lmod, n_lmod), V("note", note),
    F("lane1.parm", lane[0].parm, NULL), F("lane1.pct", lane[0].pct, NULL), V("lane1", lane[0].val),
    F("lane2.parm", lane[1].parm, NULL), F("lane2.pct", lane[1].pct, NULL), V("lane2", lane[1].val),
    F("cond", cond, NULL), F("ctyp", ctyp, n_ctyp), V("cval", cval), F("rtrg", rtrg, NULL), V("rate", rate),
    F("rcur", rcur, NULL),
    F("grv", grv, NULL), F("timing", timing, NULL), F("gvel", gvel, NULL), F("quant", quant, NULL),
    F("rand", rand, NULL), F("drft", drft, NULL), F("shft", shft, NULL),
};
#define NKEYS ((int)(sizeof keys / sizeof keys[0]))

/* the Rytm MKII's capabilities as lib/ sees them; lane 1's default parameter is the sample slot */
/* the Rytm's capabilities, as device/rytm_ar2.h gives them to the mod */
static const struct seed_param params[RT_NPARAMS] = RT_PARAMS;
static const struct seed_caps caps = {
    13, 64, RT_VOICE_FIELDS, -23, 23, SEED_LANES, RT_NPARAMS, params, { RT_P_SMP, RT_P_FRQ }
};

/* the groove tables the mod is built with (mods/seedr-ar2/seedr.c): grooves-live.h when it is
 * there (local only), else grooves-swing.h */
#if __has_include("../mods/seedr-ar2/grooves-live.h")
#include "../mods/seedr-ar2/grooves-live.h"
#else
#include "../mods/seedr-ar2/grooves-swing.h"
#endif

static struct seed_work work;
static struct seed_track orig, out, others[SEED_TRACKS];
static uint8_t touched[NKEYS];          /* --fields: the keys the arguments set */
static int16_t *field(struct seed_recipe *r, const struct key *k)
{
    touched[k - keys] = 1;
    return (int16_t *)((char *)r + k->off);
}

static void die(const char *what, const char *s)
{
    fprintf(stderr, "seedref: %s: %s\n", what, s);
    exit(2);
}

static int value(const struct key *k, const char *s)
{
    char *end;
    long v;
    int i;
    if (k && k->names)
        for (i = 0; k->names[i]; i++)
            if (!strcmp(s, k->names[i]))
                return i;
    if (!strcmp(s, "def"))
        return SEED_DEF;
    v = strtol(s, &end, 0);
    if (*s == 0 || *end)
        die("not a value", s);
    return (int)v;
}

static const struct key *find(const char *name)
{
    int i;
    for (i = 0; i < NKEYS; i++)
        if (!strcmp(keys[i].name, name))
            return &keys[i];
    return NULL;
}

/* NAME=V for a value field: def, V or LO:HI */
static int set_values(struct seed_recipe *r, const char *name, const char *v)
{
    char lo[48], hi[48];
    const struct key *klo, *khi;
    const char *colon = strchr(v, ':');
    snprintf(lo, sizeof lo, "%s.lo", name);
    snprintf(hi, sizeof hi, "%s.hi", name);
    klo = find(lo);
    khi = find(hi);
    if (!klo || !khi)
        return 0;
    if (!strcmp(v, "def")) {
        *field(r, klo) = SEED_DEF;
        return 1;
    }
    if (colon) {
        char a[32];
        size_t n = (size_t)(colon - v);
        if (n >= sizeof a)
            die("not a range", v);
        memcpy(a, v, n);
        a[n] = 0;
        *field(r, klo) = (int16_t)value(NULL, a);
        *field(r, khi) = (int16_t)value(NULL, colon + 1);
    } else {
        *field(r, klo) = *field(r, khi) = (int16_t)value(NULL, v);
    }
    return 1;
}

/* a new project's track on the Rytm (what tools/emu.py dump shows on a fresh BD) */
static void fresh(struct seed_track *t, int len)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = (int16_t)len;
    t->fields = caps.fields;
    t->default_note = 60;
    t->default_vel = 100;
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step *s = &t->s[i];
        s->flags = i & 1 ? SEED_SWING : 0;
        s->note = s->vel = s->len = SEED_DEFAULT;
        s->cond = SEED_NO_COND;
        s->rlen = SEED_NEW_RLEN;
        s->rrate = SEED_NEW_RRATE;
        s->origin = (int8_t)i;
        s->lane[0] = s->lane[1] = SEED_NO_LOCK;
    }
}

static int dashed(const char *s, int none) { return !strcmp(s, "-") ? none : (int)strtol(s, NULL, 0); }

/* a dump (this program's output, or tools/emu.py dump) back into a track */
static void read_dump(struct seed_track *t, const char *path)
{
    char line[1024];
    FILE *f = fopen(path, "r");
    if (!f)
        die("cannot open", path);
    fresh(t, 16);
    while (fgets(line, sizeof line, f)) {
        char *name = strtok(line, " \n"), *v;
        int i = 0;
        if (!name)
            continue;
        while ((v = strtok(NULL, " \n")) && i < SEED_STEPS) {
            struct seed_step *s = &t->s[i];
            if (!strcmp(name, "length")) t->length = (int16_t)atoi(v);
            else if (!strcmp(name, "flags")) s->flags = (uint16_t)((s->flags & ~0x3fu) | strtoul(v, NULL, 16));
            else if (!strcmp(name, "note")) s->note = (uint8_t)dashed(v, SEED_DEFAULT);
            else if (!strcmp(name, "vel")) s->vel = (uint8_t)dashed(v, SEED_DEFAULT);
            else if (!strcmp(name, "len")) s->len = (uint8_t)dashed(v, SEED_DEFAULT);
            else if (!strcmp(name, "utime")) s->utime = (int8_t)atoi(v);
            else if (!strcmp(name, "cond")) s->cond = (int8_t)dashed(v, SEED_NO_COND);
            else if (!strcmp(name, "rlen")) s->rlen = (uint8_t)atoi(v);
            else if (!strcmp(name, "rrate")) s->rrate = (uint8_t)atoi(v);
            else if (!strcmp(name, "rvel")) s->rvel = (int8_t)atoi(v);
            else if (!strcmp(name, "lane1")) s->lane[0] = (int16_t)dashed(v, SEED_NO_LOCK);
            else if (!strcmp(name, "lane2")) s->lane[1] = (int16_t)dashed(v, SEED_NO_LOCK);
            i++;
        }
    }
    fclose(f);
    if (t->length < 1 || t->length > SEED_STEPS)
        die("length out of range", path);
}

static void row(const char *name, const struct seed_track *t, int what)
{
    int i;
    printf("%s", name);
    for (i = 0; i < t->length; i++) {
        const struct seed_step *s = &t->s[i];
        switch (what) {
        case 0: printf(" %c", s->flags & SEED_TRIG ? 'x' : '.'); break;
        case 1: printf(" %02x", s->flags & (SEED_KNOWN | SEED_SWING)); break;
        case 2: s->note == SEED_DEFAULT ? printf(" -") : printf(" %d", s->note); break;
        case 3: s->vel == SEED_DEFAULT ? printf(" -") : printf(" %d", s->vel); break;
        case 4: s->len == SEED_DEFAULT ? printf(" -") : printf(" %d", s->len); break;
        case 5: printf(" %d", s->utime); break;
        case 6: s->cond == SEED_NO_COND ? printf(" -") : printf(" %d", s->cond); break;
        case 7: printf(" %d", s->rlen); break;
        case 8: printf(" %d", s->rrate); break;
        case 9: printf(" %d", s->rvel); break;
        case 10: s->lane[0] == SEED_NO_LOCK ? printf(" -") : printf(" %d", s->lane[0]); break;
        case 11: s->lane[1] == SEED_NO_LOCK ? printf(" -") : printf(" %d", s->lane[1]); break;
        case 12: printf(" %d", s->origin); break;
        }
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    static const char *const rows[] = { "trig", "flags", "note", "vel", "len", "utime", "cond", "rlen", "rrate",
                                        "rvel", "lane1", "lane2", "origin" };
    struct seed_recipe r;
    struct seed_ctx c;
    const char *orig_path = NULL;
    int i, rc, all = 0, init = 0, length = 0, yroll = -1, ywild = 0;
    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "orig=", 5)) orig_path = argv[i] + 5;
        else if (!strcmp(argv[i], "init=1")) init = 1;
        else if (!strncmp(argv[i], "length=", 7)) length = atoi(argv[i] + 7);
    }
    if (orig_path)
        read_dump(&orig, orig_path);
    else
        fresh(&orig, 16);
    if (length >= 1 && length <= SEED_STEPS)
        orig.length = (int16_t)length;
    if (init)
        seed_recipe_init(&r, &orig, NULL, &caps);
    else
        seed_recipe_defaults(&r, &caps);
    memset(&c, 0, sizeof c);
    c.caps = &caps;
    c.tracks = caps.tracks;
    c.grooves = grooves;
    c.ngrooves = N_GROOVES;

    if (argc == 2 && !strcmp(argv[1], "--keys")) {
        printf("seed %u\n", (unsigned)r.seed);
        for (i = 0; i < NKEYS; i++)
            printf("%s %d\n", keys[i].name, *field(&r, &keys[i]));
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--layout")) {
        printf("seed %u 4\n", (unsigned)offsetof(struct seed_recipe, seed));
        for (i = 0; i < NKEYS; i++)
            printf("%s %u 2\n", keys[i].path, (unsigned)keys[i].off);
        printf("sizeof %u\n", (unsigned)sizeof(struct seed_recipe));
        return 0;
    }
    memset(touched, 0, sizeof touched);
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--fields") || !strcmp(argv[i], "--recipe"))
            continue;
        char name[64], *eq = strchr(argv[i], '=');
        const struct key *k;
        size_t n = eq ? (size_t)(eq - argv[i]) : 0;
        if (!eq || n >= sizeof name)
            die("not KEY=VALUE", argv[i]);
        memcpy(name, argv[i], n);
        name[n] = 0;
        eq++;
        if (!strcmp(name, "orig") || !strcmp(name, "init") || !strcmp(name, "length"))
            continue;
        if (!strcmp(name, "yolo")) {
            if (sscanf(eq, "%d:%d", &yroll, &ywild) != 2)
                die("not yolo=ROLL:WILD", eq);
            continue;
        }
        if (!strcmp(name, "seed")) r.seed = (uint32_t)strtoul(eq, NULL, 0);
        else if (!strcmp(name, "track")) c.track = atoi(eq);
        else if (!strcmp(name, "all")) all = atoi(eq);
        else if (!strcmp(name, "map_x")) c.shared.map_x = (int16_t)atoi(eq);
        else if (!strcmp(name, "map_y")) c.shared.map_y = (int16_t)atoi(eq);
        else if (!strcmp(name, "map_chaos")) c.shared.map_chaos = (int16_t)atoi(eq);
        else if (!strcmp(name, "other")) {
            int t = atoi(eq);
            const char *path = strchr(eq, ':');
            if (t < 0 || t >= SEED_TRACKS || !path)
                die("not other=TRACK:FILE", eq);
            read_dump(&others[t], path + 1);
        } else if ((k = find(name)) != NULL) *field(&r, k) = (int16_t)value(k, eq);
        else if (!set_values(&r, name, eq)) die("unknown key", name);
    }
    if (c.track < 0 || c.track >= SEED_TRACKS)
        die("track out of range", "track");
    if (yroll >= 0) {
        static const int8_t roles[] = RT_TRACK_ROLES;
        struct seed_recipe from = r;
        struct seed_yolo_in in;
        in.caps = &caps;
        in.role = c.track < (int)sizeof roles ? roles[c.track] : SEED_ROLE_METER;
        in.length = orig.length;
        in.fields = c.track == 12 ? RT_FX_FIELDS : RT_VOICE_FIELDS;
        in.ngrooves = N_GROOVES;
        for (in.any = 0, i = 0; i < orig.length; i++)
            in.any |= orig.s[i].flags & SEED_TRIG;
        seed_yolo(&r, &from, yroll, ywild, &in);
        memset(touched, 1, sizeof touched);
    }
    if (argc > 1 && (!strcmp(argv[1], "--fields") || !strcmp(argv[1], "--recipe"))) {
        int whole = !strcmp(argv[1], "--recipe"), seeded = whole;
        for (i = 1; i < argc; i++)
            if (!strncmp(argv[i], "seed=", 5))
                seeded = 1;
        if (seeded)
            printf("seed %u\n", (unsigned)r.seed);
        for (i = 0; i < NKEYS; i++)
            if (touched[i] || whole)
                printf("%s %d\n", keys[i].path, *(int16_t *)((char *)&r + keys[i].off));
        return 0;
    }
    /* the context: the track in scope is the original, the others as given (empty otherwise) */
    others[c.track] = orig;
    for (i = 0; i < c.tracks; i++) {
        int s;
        c.length[i] = (uint8_t)(others[i].length ? others[i].length : 16);
        for (s = 0; s < SEED_STEPS; s++)
            if (others[i].s[s].flags & SEED_TRIG)
                c.trig[i][s >> 3] |= (uint8_t)(1u << (s & 7));
    }
    rc = seed_render(&out, &orig, &r, &c, &work);
    if (rc != SEED_OK) {
        fprintf(stderr, "result %d\n", rc);
        return 3;
    }
    printf("length %d\n", out.length);
    for (i = 0; i < 13; i++)
        if (all || i < 11)
            row(rows[i], &out, i);
    return 0;
}
