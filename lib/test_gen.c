/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for gen.c: cc -std=c99 -Wall -Wextra -O2 gen.c test_gen.c -o test_gen && ./test_gen */
#include <stdio.h>
#include <string.h>
#include "gen.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void blank(struct gen_track *t, int len)
{
    int i;
    memset(t, 0, sizeof *t);
    t->length = len;
    t->default_note = 60;
    for (i = 0; i < GEN_STEPS; i++) {
        struct gen_step *s = &t->s[i];
        s->note = s->vel = s->len = GEN_DEFAULT;
        s->cond = GEN_NO_COND;
        s->slock = GEN_NO_LOCK;
        s->rlen = GEN_NEW_RLEN;
        s->rrate = GEN_NEW_RRATE;
        if (i & 1)
            s->flags = GEN_SWING;            /* the Rytm's default swing trigs */
    }
}

static int trigs(const struct gen_track *t)
{
    int i, n = 0;
    for (i = 0; i < t->length; i++)
        n += (t->s[i].flags & GEN_TRIG) != 0;
    return n;
}

static void show(const char *name, const struct gen_track *t)
{
    int i;
    printf("%-22s ", name);
    for (i = 0; i < t->length; i++)
        putchar(t->s[i].flags & GEN_TRIG ? (t->s[i].flags & GEN_ACCENT ? 'A' : 'x') : '.');
    putchar('\n');
}

static void test_rng(void)
{
    struct gen_rng a, b;
    int i, same = 1, lo = 1000, hi = -1000;
    gen_seed(&a, 42);
    gen_seed(&b, 42);
    for (i = 0; i < 1000; i++)
        same &= gen_next(&a) == gen_next(&b);
    CHECK(same);
    gen_seed(&a, 0);
    CHECK(a.x != 0);
    for (i = 0; i < 10000; i++) {
        int v = gen_range(&a, -3, 3);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    CHECK(lo == -3 && hi == 3);
    CHECK(gen_range(&a, 5, 5) == 5);
    {
        int v = gen_range(&a, 9, 2);          /* bounds in either order */
        CHECK(v >= 2 && v <= 9);
    }
}

static void test_cond_ab(void)
{
    CHECK(gen_cond_ab(1, 2) == 30);
    CHECK(gen_cond_ab(2, 2) == 31);
    CHECK(gen_cond_ab(1, 3) == 32);
    CHECK(gen_cond_ab(1, 4) == 35);
    CHECK(gen_cond_ab(4, 4) == 38);
    CHECK(gen_cond_ab(1, 5) == 39);
    CHECK(gen_cond_ab(1, 8) == 57);
    CHECK(gen_cond_ab(8, 8) == 64);
}

static void test_generate_basics(void)
{
    struct gen_track t, u;
    struct gen_params p;
    int i, ok;

    gen_defaults(&p);
    blank(&t, 16);
    p.dens = 50;
    p.seed = 7;
    CHECK(gen_generate(&t, &p) == 8);
    CHECK(trigs(&t) == 8);
    show("dens 50, grid 1", &t);

    /* same seed, same result */
    blank(&u, 16);
    gen_generate(&u, &p);
    CHECK(memcmp(&t, &u, sizeof t) == 0);
    /* another seed, another result */
    p.seed = 8;
    blank(&u, 16);
    gen_generate(&u, &p);
    CHECK(memcmp(&t, &u, sizeof t) != 0);

    /* velocities in range, defaults left where the params say so */
    ok = 1;
    for (i = 0; i < 16; i++)
        if (t.s[i].flags & GEN_TRIG)
            ok &= t.s[i].vel >= 80 && t.s[i].vel <= 120 && t.s[i].len == GEN_DEFAULT &&
                  t.s[i].note == GEN_DEFAULT && t.s[i].utime == 0 && t.s[i].cond == GEN_NO_COND;
    CHECK(ok);
    /* swing bits kept */
    ok = 1;
    for (i = 0; i < 16; i++)
        ok &= ((t.s[i].flags & GEN_SWING) != 0) == (i & 1);
    CHECK(ok);
    /* steps past the length are left alone */
    blank(&t, 16);
    t.s[20].flags = GEN_TRIG;
    t.s[20].vel = 3;
    p.dens = 100;
    gen_generate(&t, &p);
    CHECK(t.s[20].flags == (GEN_TRIG) && t.s[20].vel == 3);
    CHECK(trigs(&t) == 16);
}

static void test_generate_grid_and_density(void)
{
    struct gen_track t;
    struct gen_params p;
    int i, ok, g;
    static const int grids[] = {1, 2, 4, 8};

    gen_defaults(&p);
    for (g = 0; g < 4; g++) {
        int d;
        for (d = 0; d <= 100; d += 25) {
            int npos = (32 + grids[g] - 1) / grids[g];
            blank(&t, 32);
            p.grid = grids[g];
            p.dens = d;
            p.seed = (uint32_t)(g * 100 + d);
            gen_generate(&t, &p);
            CHECK(trigs(&t) == (d * npos + 50) / 100);
            ok = 1;
            for (i = 0; i < 32; i++)
                if (t.s[i].flags & GEN_TRIG)
                    ok &= i % grids[g] == 0;
            CHECK(ok);
        }
    }
    blank(&t, 16);
    p.grid = 4;
    p.dens = 100;
    gen_generate(&t, &p);
    show("grid 4, dens 100", &t);
    /* an odd grid is treated as 1 */
    blank(&t, 16);
    p.grid = 3;
    p.dens = 100;
    gen_generate(&t, &p);
    CHECK(trigs(&t) == 16);
}

static void test_generate_fields(void)
{
    struct gen_track t;
    struct gen_params p;
    int i, ok, accents = 0, conds = 0, retrigs = 0, n = 0;

    gen_defaults(&p);
    blank(&t, 64);
    p.dens = 100;
    p.note_lo = -5;
    p.note_hi = 7;
    p.len_lo = 14;
    p.len_hi = 30;
    p.utim = 50;
    p.acc = 25;
    p.cond = 50;
    p.ctyp = GEN_CT_PROB;
    p.cval_lo = 5;
    p.cval_hi = 12;
    p.rtrg = 20;
    p.rate_lo = 9;
    p.rate_hi = 12;
    p.rcur = -30;
    p.seed = 1234;
    gen_generate(&t, &p);
    show("all fields, 64 steps", &t);
    ok = 1;
    for (i = 0; i < 64; i++) {
        struct gen_step *s = &t.s[i];
        if (!(s->flags & GEN_TRIG))
            continue;
        n++;
        ok &= s->note >= 55 && s->note <= 67;
        ok &= s->len >= 14 && s->len <= 30;
        ok &= s->utime >= -11 && s->utime <= 11;   /* 23 * 50% */
        if (s->flags & GEN_ACCENT)
            accents++;
        if (s->cond != GEN_NO_COND) {
            conds++;
            ok &= s->cond >= 5 && s->cond <= 12;
        }
        if (s->flags & GEN_RETRIG) {
            retrigs++;
            ok &= s->rrate >= 9 && s->rrate <= 12 && s->rvel == -30;
        }
    }
    CHECK(ok);
    CHECK(n == 64);
    CHECK(accents > 5 && accents < 30);
    CHECK(conds > 20 && conds < 45);
    CHECK(retrigs > 3 && retrigs < 25);
    printf("%-22s accents %d, conditions %d, retrigs %d of 64\n", "", accents, conds, retrigs);

    /* condition kinds */
    p.cond = 100;
    p.ctyp = GEN_CT_AB;
    p.cval_lo = 4;
    blank(&t, 32);
    gen_generate(&t, &p);
    ok = 1;
    for (i = 0; i < 32; i++)
        ok &= t.s[i].cond >= 35 && t.s[i].cond <= 38;
    CHECK(ok);
    p.ctyp = GEN_CT_1ST;
    blank(&t, 8);
    gen_generate(&t, &p);
    CHECK(t.s[0].cond == 28 && t.s[7].cond == 28);
    p.ctyp = GEN_CT_FILL;
    blank(&t, 8);
    gen_generate(&t, &p);
    CHECK(t.s[3].cond == 22);
    p.ctyp = GEN_CT_MIX;
    p.cval_lo = 0;
    p.cval_hi = 21;
    blank(&t, 64);
    gen_generate(&t, &p);
    {
        int prob = 0, ab = 0, first = 0, fill = 0;
        for (i = 0; i < 64; i++) {
            int c = t.s[i].cond;
            prob += c >= 0 && c <= 21;
            ab += c >= 30;
            first += c == 28;
            fill += c == 22;
        }
        CHECK(prob && ab && first && fill);
    }
    /* a note offset near the edge clamps to 0..126 */
    p.cond = 0;
    p.note_lo = p.note_hi = 24;
    blank(&t, 4);
    t.default_note = 120;
    gen_generate(&t, &p);
    CHECK(t.s[0].note == 126);
}

static void test_generate_clears(void)
{
    struct gen_track t;
    struct gen_params p;
    int i;

    gen_defaults(&p);
    blank(&t, 16);
    for (i = 0; i < 16; i++) {
        t.s[i].flags |= GEN_TRIG | GEN_ACCENT | GEN_RETRIG | GEN_SLIDE | 0x0380;
        t.s[i].cond = 30;
        t.s[i].utime = 9;
        t.s[i].vel = 1;
    }
    p.dens = 0;
    gen_generate(&t, &p);
    CHECK(trigs(&t) == 0);
    for (i = 0; i < 16; i++) {
        CHECK((t.s[i].flags & (GEN_ACCENT | GEN_RETRIG | GEN_SLIDE)) == 0);
        CHECK(t.s[i].cond == GEN_NO_COND && t.s[i].utime == 0 && t.s[i].vel == GEN_DEFAULT);
        CHECK((t.s[i].flags & 0x0380) == 0x0380);     /* bits gen does not know stay */
    }
}

static void test_rotate_all(void)
{
    struct gen_track t, o;
    int i, ok;

    blank(&t, 16);
    t.s[0].flags |= GEN_TRIG | GEN_ACCENT;
    t.s[0].vel = 100;
    t.s[15].flags |= GEN_TRIG;
    t.s[15].vel = 50;
    o = t;
    gen_rotate(&t, 0, 16, 1, GEN_W_ALL, GEN_R_TRIG);
    CHECK((t.s[1].flags & (GEN_TRIG | GEN_ACCENT)) == (GEN_TRIG | GEN_ACCENT) && t.s[1].vel == 100);
    CHECK((t.s[0].flags & GEN_TRIG) && t.s[0].vel == 50);          /* step 16 wrapped to step 1 */
    ok = 1;
    for (i = 0; i < 16; i++)
        ok &= ((t.s[i].flags & GEN_SWING) != 0) == (i & 1);       /* swing stays */
    CHECK(ok);
    gen_rotate(&t, 0, 16, -1, GEN_W_ALL, GEN_R_TRIG);
    CHECK(memcmp(&t, &o, sizeof t) == 0);                          /* +1 then -1: back */
    gen_rotate(&t, 0, 16, 16, GEN_W_ALL, GEN_R_TRIG);
    CHECK(memcmp(&t, &o, sizeof t) == 0);                          /* a full turn: unchanged */
    gen_rotate(&t, 0, 16, 37, GEN_W_ALL, GEN_R_TRIG);              /* 37 = 2 * 16 + 5 */
    CHECK((t.s[5].flags & GEN_TRIG) && t.s[5].vel == 100);
    /* a sub-range (a page): only it moves */
    t = o;
    t.s[20].flags |= GEN_TRIG;
    gen_rotate(&t, 16, 16, 2, GEN_W_ALL, GEN_R_TRIG);
    CHECK(t.s[22].flags & GEN_TRIG);
    CHECK(!(t.s[20].flags & GEN_TRIG));
    CHECK(t.s[0].vel == 100);
}

static void test_rotate_fields(void)
{
    struct gen_track t;

    /* trigs on steps 1, 4, 7, 11 (0, 3, 6, 10), velocities 10, 20, 30, 40 */
    blank(&t, 16);
    t.s[0].flags |= GEN_TRIG; t.s[0].vel = 10;
    t.s[3].flags |= GEN_TRIG; t.s[3].vel = 20;
    t.s[6].flags |= GEN_TRIG; t.s[6].vel = 30;
    t.s[10].flags |= GEN_TRIG; t.s[10].vel = 40;

    /* by trig: each velocity moves to the next trig */
    gen_rotate(&t, 0, 16, 1, GEN_W_VEL, GEN_R_TRIG);
    CHECK(t.s[0].vel == 40 && t.s[3].vel == 10 && t.s[6].vel == 20 && t.s[10].vel == 30);
    CHECK(t.s[1].vel == GEN_DEFAULT);                 /* empty steps untouched */
    gen_rotate(&t, 0, 16, -1, GEN_W_VEL, GEN_R_TRIG);
    CHECK(t.s[0].vel == 10 && t.s[10].vel == 40);

    /* by grid: +3 moves step 1's value to step 4 (a trig), step 4's to step 7 ... step 11's to
     * step 14 (no trig: dropped); step 1 receives from step 14 (no trig): keeps its value */
    gen_rotate(&t, 0, 16, 3, GEN_W_VEL, GEN_R_GRID);
    CHECK(t.s[3].vel == 10 && t.s[6].vel == 20 && t.s[0].vel == 10);
    CHECK(t.s[13].vel == GEN_DEFAULT && !(t.s[13].flags & GEN_TRIG));
    CHECK(t.s[10].vel == 40);                         /* step 11 got nothing from step 8 (empty) */

    /* accents move by grid step and may land on empty steps */
    t.s[0].flags |= GEN_ACCENT;
    gen_rotate(&t, 0, 16, 2, GEN_W_ACC, GEN_R_TRIG);
    CHECK(!(t.s[0].flags & GEN_ACCENT) && (t.s[2].flags & GEN_ACCENT));
    CHECK(!(t.s[2].flags & GEN_TRIG));

    /* retrig travels as a group */
    t.s[6].flags |= GEN_RETRIG; t.s[6].rrate = 14; t.s[6].rvel = -5; t.s[6].rlen = 20;
    gen_rotate(&t, 0, 16, 1, GEN_W_RTRG, GEN_R_TRIG);
    CHECK((t.s[10].flags & GEN_RETRIG) && t.s[10].rrate == 14 && t.s[10].rvel == -5 && t.s[10].rlen == 20);
    CHECK(!(t.s[6].flags & GEN_RETRIG) && t.s[6].rrate == GEN_NEW_RRATE);

    /* micro timing, conditions, notes and lengths use the same path */
    t.s[0].utime = 7;
    gen_rotate(&t, 0, 16, 1, GEN_W_UTIM, GEN_R_TRIG);
    CHECK(t.s[3].utime == 7 && t.s[0].utime == 0);
    t.s[3].cond = 35;
    gen_rotate(&t, 0, 16, -1, GEN_W_COND, GEN_R_TRIG);
    CHECK(t.s[0].cond == 35);
    /* one trig or none: nothing to rotate */
    blank(&t, 16);
    t.s[5].flags |= GEN_TRIG; t.s[5].vel = 9;
    gen_rotate(&t, 0, 16, 3, GEN_W_VEL, GEN_R_TRIG);
    CHECK(t.s[5].vel == 9);
    /* bad ranges are ignored */
    gen_rotate(&t, 60, 10, 1, GEN_W_ALL, GEN_R_TRIG);
    gen_rotate(&t, -1, 4, 1, GEN_W_ALL, GEN_R_TRIG);
    CHECK(t.s[5].vel == 9 && (t.s[5].flags & GEN_TRIG));
}

static void test_generate_rot(void)
{
    struct gen_track a, b;
    struct gen_params p;
    gen_defaults(&p);
    p.dens = 30;
    p.seed = 99;
    blank(&a, 16);
    gen_generate(&a, &p);
    gen_rotate(&a, 0, 16, 3, GEN_W_ALL, GEN_R_TRIG);
    p.rot = 3;
    blank(&b, 16);
    gen_generate(&b, &p);
    CHECK(memcmp(&a, &b, sizeof a) == 0);
}

static void test_generate_smp(void)
{
    struct gen_track a, b;
    struct gen_params p;
    int i, locked = 0, ok = 1;
    gen_defaults(&p);
    p.seed = 5;
    /* SMP 0: existing sample locks stay, and the rest is exactly as without the feature */
    blank(&a, 16);
    a.s[3].slock = 7;
    gen_generate(&a, &p);
    CHECK(a.s[3].slock == 7);
    /* SMP 100: every trig locked within the range, every other step's lock removed */
    p.smp = 100; p.smp_lo = 10; p.smp_hi = 13;
    blank(&a, 32);
    for (i = 0; i < 32; i++)
        a.s[i].slock = 99;
    gen_generate(&a, &p);
    for (i = 0; i < 32; i++) {
        if (a.s[i].flags & GEN_TRIG)
            ok &= a.s[i].slock >= 10 && a.s[i].slock <= 13;
        else
            ok &= a.s[i].slock == GEN_NO_LOCK;
    }
    CHECK(ok);
    /* SMP 50: some trigs; the trig placement matches SMP 0 (the shuffle comes first; the later
     * fields of a take change, as with ACC or COND) */
    p.smp = 50;
    blank(&a, 64); gen_generate(&a, &p);
    p.smp = 0;
    blank(&b, 64); gen_generate(&b, &p);
    ok = 1;
    for (i = 0; i < 64; i++) {
        ok &= (a.s[i].flags & GEN_TRIG) == (b.s[i].flags & GEN_TRIG);
        locked += a.s[i].slock != GEN_NO_LOCK;
    }
    CHECK(ok);
    CHECK(locked > 4 && locked < 28);
    /* lo > hi is taken in either order */
    p.smp = 100; p.smp_lo = 20; p.smp_hi = 18;
    blank(&a, 16); gen_generate(&a, &p);
    ok = 1;
    for (i = 0; i < 16; i++)
        if (a.s[i].flags & GEN_TRIG)
            ok &= a.s[i].slock >= 18 && a.s[i].slock <= 20;
    CHECK(ok);
}

static void test_meter(void)
{
    struct gen_track a, b;
    struct gen_params p;
    int i, seed, ok, on, off, on0, ok2;

    /* weights, 4 steps per beat and 3 */
    CHECK(gen_weight(0, 4) == 4 && gen_weight(16, 4) == 4 && gen_weight(48, 4) == 4);
    CHECK(gen_weight(8, 4) == 3 && gen_weight(24, 4) == 3);
    CHECK(gen_weight(4, 4) == 2 && gen_weight(12, 4) == 2);
    CHECK(gen_weight(2, 4) == 1 && gen_weight(14, 4) == 1);
    CHECK(gen_weight(1, 4) == 0 && gen_weight(3, 4) == 0 && gen_weight(15, 4) == 0);
    CHECK(gen_weight(0, 3) == 4 && gen_weight(6, 3) == 3 && gen_weight(3, 3) == 2 && gen_weight(9, 3) == 2);
    CHECK(gen_weight(1, 3) == 0 && gen_weight(2, 3) == 0);

    /* defaults are a no-op: beat, skip, bias 0, vamt 0 give the same take whatever the shape */
    gen_defaults(&p);
    CHECK(p.beat == 4 && p.skip == 0 && p.bias == 0 && p.vshp == GEN_VS_FLAT && p.vamt == 0);
    p.dens = 60; p.seed = 7;
    blank(&a, 32); gen_generate(&a, &p);
    p.vshp = GEN_VS_STRONG; p.beat = 3;
    blank(&b, 32); gen_generate(&b, &p);
    CHECK(memcmp(&a, &b, sizeof a) == 0);

    /* SKIP clears the first steps and generates after them, also with ROT */
    gen_defaults(&p);
    p.dens = 100; p.skip = 3;
    blank(&a, 16);
    for (i = 0; i < 16; i++) { a.s[i].flags |= GEN_TRIG | GEN_ACCENT; a.s[i].vel = 9; }
    CHECK(gen_generate(&a, &p) == 13);
    ok = 1;
    for (i = 0; i < 16; i++)
        ok &= (i < 3) == !(a.s[i].flags & GEN_TRIG);
    ok &= a.s[0].vel == GEN_DEFAULT && !(a.s[0].flags & GEN_ACCENT);
    CHECK(ok);
    p.rot = 5; p.dens = 50;
    for (seed = 1, ok = 1; seed < 40; seed++) {
        p.seed = (uint32_t)seed;
        blank(&a, 16); gen_generate(&a, &p);
        ok &= trigs(&a) == 7 && !(a.s[0].flags & GEN_TRIG) && !(a.s[1].flags & GEN_TRIG) && !(a.s[2].flags & GEN_TRIG);
    }
    CHECK(ok);
    p.skip = 64; p.rot = 0;
    blank(&a, 16);
    CHECK(gen_generate(&a, &p) == 0 && trigs(&a) == 0);

    /* BIAS: the count stays exact; + puts trigs on strong steps, - on weak ones */
    gen_defaults(&p);
    p.dens = 25;
    on0 = on = off = 0; ok = ok2 = 1;
    for (seed = 1; seed <= 300; seed++) {
        p.seed = (uint32_t)seed;
        p.bias = 0;   blank(&a, 16); gen_generate(&a, &p);
        for (i = 0; i < 16; i++) on0 += (a.s[i].flags & GEN_TRIG) && gen_weight(i, 4) >= 2;
        p.bias = 100; blank(&a, 16); gen_generate(&a, &p);
        ok &= trigs(&a) == 4;
        for (i = 0; i < 16; i++) on += (a.s[i].flags & GEN_TRIG) && gen_weight(i, 4) >= 2;
        p.bias = -100; blank(&a, 16); gen_generate(&a, &p);
        ok2 &= trigs(&a) == 4;
        for (i = 0; i < 16; i++) off += (a.s[i].flags & GEN_TRIG) && gen_weight(i, 4) >= 2;
    }
    printf("%-22s beats/1200 trigs: bias 0 %d, +100 %d, -100 %d\n", "meter bias", on0, on, off);
    CHECK(ok && ok2);
    CHECK(on0 > 230 && on0 < 370);       /* uniform: a quarter of the trigs on the 4 beats */
    CHECK(on > 2 * on0);
    CHECK(off < on0 / 3);

    /* the velocity shapes at VAMT 100, VEL 40..120, every step a trig */
    gen_defaults(&p);
    p.dens = 100; p.vel_lo = 40; p.vel_hi = 120; p.vamt = 100;
    p.vshp = GEN_VS_STRONG; blank(&a, 16); gen_generate(&a, &p);
    CHECK(a.s[0].vel == 120 && a.s[8].vel == 100 && a.s[4].vel == 80 && a.s[2].vel == 60 && a.s[1].vel == 40);
    p.vshp = GEN_VS_WEAK; blank(&a, 16); gen_generate(&a, &p);
    CHECK(a.s[0].vel == 40 && a.s[4].vel == 80 && a.s[1].vel == 120);
    p.vshp = GEN_VS_OFF; blank(&a, 16); gen_generate(&a, &p);
    CHECK(a.s[2].vel == 120 && a.s[1].vel == 80 && a.s[4].vel == 40 && a.s[0].vel == 40);
    p.vshp = GEN_VS_NOT1; blank(&a, 16); gen_generate(&a, &p);
    CHECK(a.s[0].vel == 40 && a.s[4].vel == 40 && a.s[12].vel == 40 && a.s[1].vel == 120 && a.s[2].vel == 120);
    p.vshp = GEN_VS_NOTBAR; blank(&a, 16); gen_generate(&a, &p);
    CHECK(a.s[0].vel == 40 && a.s[4].vel == 120 && a.s[8].vel == 120);
    /* VEL given high to low is the same range */
    p.vel_lo = 120; p.vel_hi = 40; blank(&b, 16); gen_generate(&b, &p);
    CHECK(b.s[0].vel == 40 && b.s[4].vel == 120);
    /* VAMT 50: half way between the random velocity and the shape's */
    p.vel_lo = 40; p.vel_hi = 120;
    p.vshp = GEN_VS_FLAT; p.vamt = 0; blank(&b, 16); gen_generate(&b, &p);
    p.vshp = GEN_VS_NOT1; p.vamt = 50; blank(&a, 16); gen_generate(&a, &p);
    ok = 1;
    for (i = 0; i < 16; i++) {
        int to = gen_weight(i, 4) >= 2 ? 40 : 120;
        ok &= a.s[i].vel == b.s[i].vel + (to - b.s[i].vel) * 50 / 100;
    }
    CHECK(ok);
    /* the shape is where the steps land: with ROT the bar's first step is still the loudest */
    p.vshp = GEN_VS_STRONG; p.vamt = 100; p.rot = 3;
    blank(&a, 16); gen_generate(&a, &p);
    CHECK(a.s[0].vel == 120 && a.s[1].vel == 40 && a.s[8].vel == 100);
    show("meter STRONG rot 3", &a);
}

int main(void)
{
    test_rng();
    test_cond_ab();
    test_generate_basics();
    test_generate_grid_and_density();
    test_generate_fields();
    test_generate_clears();
    test_rotate_all();
    test_rotate_fields();
    test_generate_rot();
    test_generate_smp();
    test_meter();
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
