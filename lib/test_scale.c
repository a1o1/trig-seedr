/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for scale.c (0.9, L9): every scale against a table written out by hand as note names
 * from C, every root and note: the result is in the scale, no note of the scale is nearer, a tie
 * goes to the lower note, OFF and CHR change nothing; and SCALE through NOTE in a render. */
#include "tests.h"

/* each scale's notes from C, by hand */
static const char *const table[SEED_SCALE_N] = {
    "C C# D D# E F F# G G# A A# B",     /* OFF */
    "C C# D D# E F F# G G# A A# B",     /* CHR */
    "C D E F G A B",                    /* MAJ */
    "C D D# F G G# A#",                 /* MIN */
    "C D D# F G A A#",                  /* DOR */
    "C C# D# F G G# A#",                /* PHR */
    "C D E F# G A B",                   /* LYD */
    "C D E F G A A#",                   /* MIX */
    "C C# D# F F# G# A#",               /* LOC */
    "C D D# F G G# B",                  /* HMIN */
    "C D D# F G A B",                   /* MMIN */
    "C D E G A",                        /* PMAJ */
    "C D# F G A#",                      /* PMIN */
    "C D E F# G# A#",                   /* WHOLE */
    "C D D# F F# G# A B",               /* DIM */
    "C D D# G G#",                      /* HIRA */
    "C F G A#",                         /* 7SUS4 */
    "C E G B",                          /* MAJ7 */
    "C D# G A#",                        /* MIN7 */
};

static int name_pc(const char *n)
{
    static const char *const names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int i;
    for (i = 0; i < 12; i++)
        if (!strcmp(n, names[i]))
            return i;
    return -1;
}

static unsigned mask_of(int s)
{
    char buf[64], *p;
    unsigned m = 0;
    strcpy(buf, table[s]);
    for (p = strtok(buf, " "); p; p = strtok(NULL, " "))
        m |= 1u << name_pc(p);
    return m;
}

static int in(unsigned m, int note, int root)
{
    return (m >> (((note - root) % 12 + 12) % 12)) & 1;
}

static void test_all(void)
{
    int s, root, note;
    for (s = 0; s < SEED_SCALE_N; s++) {
        unsigned m = mask_of(s);
        for (root = 0; root < 12; root++)
            for (note = 0; note <= 127; note++) {
                int q = seed_scale_note(note, s, root), d = q > note ? q - note : note - q, e;
                CHECK(in(m, q, root));
                for (e = 0; e < d; e++)
                    CHECK(!in(m, note - e, root) && !in(m, note + e, root));
                if (q > note)
                    CHECK(!in(m, note - d, root));        /* a tie goes down */
                if (s == SEED_SCALE_OFF || s == SEED_SCALE_CHR)
                    CHECK(q == note);
            }
    }
    CHECK(seed_scale_note(60, SEED_SCALE_N, 0) == SEED_E_ARG);
    CHECK(seed_scale_note(60, SEED_SCALE_MAJ, 12) == SEED_E_ARG);
    CHECK(seed_scale_note(61, SEED_SCALE_MAJ, 0) == 60);      /* C# -> C, the lower */
    CHECK(seed_scale_note(66, SEED_SCALE_MAJ, 0) == 65);      /* F# -> F */
    CHECK(seed_scale_note(61, SEED_SCALE_MAJ, 1) == 61);      /* C# major has C# */
}

/* SCALE through NOTE: every note written is in the scale; OFF leaves the take as it was */
static void test_render(void)
{
    static struct seed_track orig, a, b;
    struct seed_recipe r;
    struct seed_ctx c = test_ctx(0);
    int s, i, n;
    for (n = 0; n < 200; n++) {
        random_track(&orig, rnd(1, 64));
        seed_recipe_defaults(&r, &test_caps);
        r.src = SEED_SRC_RANDOM;
        r.seed = (uint32_t)n;
        r.note.lo = -24;
        r.note.hi = 24;
        CHECK(seed_render(&a, &orig, &r, &c, &work) == SEED_OK);
        s = rnd(1, SEED_SCALE_N - 1);
        r.scale = (int16_t)s;
        r.root = (int16_t)rnd(0, 11);
        CHECK(seed_render(&b, &orig, &r, &c, &work) == SEED_OK);
        for (i = 0; i < b.length; i++) {
            if (!(b.s[i].flags & SEED_TRIG))
                continue;
            CHECK(in(mask_of(s), b.s[i].note, r.root));
            CHECK(b.s[i].note == seed_scale_note(a.s[i].note, s, r.root) || a.s[i].note == 126);
        }
        r.scale = SEED_SCALE_OFF;
        CHECK(seed_render(&b, &orig, &r, &c, &work) == SEED_OK);
        CHECK(!memcmp(&a, &b, sizeof a));
    }
    r.scale = SEED_SCALE_N;
    CHECK(seed_render(&b, &orig, &r, &c, &work) == SEED_E_ARG);
}

int main(void)
{
    test_setup();
    test_all();
    test_render();
    printf("test_scale: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
