/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for place_sync.c (F5): SYNC. Worked cases in 4/4 (one move with its whole step and
 * swing; four on the floor; runs that would resolve onto a weaker trig; a lone trig; SKIP), then
 * properties over random tracks and recipes: the count of trigs (I5), one grid step earlier onto
 * a weaker step, every run of moved trigs resolving onto a stronger trig that stays, SKIP, the share
 * nesting (I4), and I2, I3, I6 with SYNC on (props.h). */
#include "tests.h"
#include "props.h"

#define ROUNDS 3000

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

/* a random track of length len whose trigs are exactly the steps listed (ending with -1) */
static void track_with(int len, const int *steps)
{
    int i;
    random_track(&orig, len);
    for (i = 0; i < SEED_STEPS; i++)
        orig.s[i].flags &= (uint16_t)~SEED_TRIG;
    for (i = 0; steps[i] >= 0; i++)
        orig.s[steps[i]].flags |= SEED_TRIG;
    seed_recipe_defaults(&r, &test_caps);
}

/* the trigs of out below the length as a bit mask (16 steps or fewer) */
static unsigned trigs16(const struct seed_track *t)
{
    unsigned m = 0;
    int i;
    for (i = 0; i < t->length && i < 32; i++)
        if (t->s[i].flags & SEED_TRIG)
            m |= 1u << i;
    return m;
}

#define B(i) (1u << (i))

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

/* a step equal to another but for swing, which stays with the position */
static int same_but_swing(const struct seed_step *a, const struct seed_step *b, uint16_t swing)
{
    struct seed_step x = *b;
    x.flags = (uint16_t)((x.flags & ~SEED_SWING) | swing);
    return memcmp(a, &x, sizeof x) == 0;
}

/* the trigs out has, over seeds 0..199 at this SYNC: each one of `allowed` (n masks), each seen */
static int outcomes(int sync, const unsigned *allowed, int n)
{
    int seed, j, ok = 1, seen[8] = { 0 };
    r.sync = (int16_t)sync;
    for (seed = 0; seed < 200; seed++) {
        unsigned got;
        r.seed = (uint32_t)seed;
        if (!rendered())
            return 0;
        got = trigs16(&out);
        for (j = 0; j < n && allowed[j] != got; j++)
            ;
        if (j == n) {
            printf("  seed %d: trigs %04x\n", seed, got);
            ok = 0;
        } else
            seen[j]++;
    }
    for (j = 0; j < n; j++)
        ok &= seen[j] > 0;
    return ok;
}

/* one anticipation: 4 -> 3, the whole step with it; 9 is no candidate (8 is stronger) */
static void test_single(void)
{
    static const int st[] = { 4, 9, -1 };
    track_with(16, st);
    r.sync = 100;
    CHECK(rendered() && trigs16(&out) == (B(3) | B(9)));
    CHECK(out.s[3].origin == 4 && out.s[4].origin == 3);
    CHECK(same_but_swing(&out.s[3], &orig.s[4], orig.s[3].flags & SEED_SWING));   /* swing stays */
    CHECK(same_but_swing(&out.s[4], &orig.s[3], orig.s[4].flags & SEED_SWING));
    CHECK(memcmp(&out.s[5], &orig.s[5], sizeof out.s[0] * 59) == 0 && memcmp(&out.s[0], &orig.s[0], sizeof out.s[0] * 3) == 0);
    r.sync = 40;                                            /* 40 % of one move: none */
    CHECK(rendered() && memcmp(&out, &orig, sizeof out) == 0);
    r.sync = 50;
    CHECK(rendered() && trigs16(&out) == (B(3) | B(9)));
}

/* four on the floor: all four are candidates. A run must resolve onto a stronger trig that stays:
 * 4 8 12 onto 0, or 8 12 onto 0; or two lone moves (0 and 8, 4 and 12). Never all four. */
static void test_four_on_the_floor(void)
{
    static const int st[] = { 0, 4, 8, 12, -1 };
    static const unsigned full[] = { B(0) | B(3) | B(7) | B(11), B(0) | B(4) | B(7) | B(11), B(4) | B(7) | B(12) | B(15),
                                     B(0) | B(3) | B(8) | B(11) };
    track_with(16, st);
    CHECK(outcomes(100, full, 4));
}

/* a run that would resolve onto a weaker trig: 0 and 4 both moved would end before 5 */
static void test_runs(void)
{
    static const int st[] = { 0, 4, 5, -1 }, st2[] = { 2, 4, 12, -1 }, one[] = { 4, -1 };
    static const unsigned a[] = { B(4) | B(5) | B(15), B(0) | B(3) | B(5) };
    /* 2 4 resolve onto 12 (13 > 12, 8); 4 12 and 12 2 would not, so 12 alone or 2 and 4 */
    static const unsigned b[] = { B(1) | B(3) | B(12), B(2) | B(4) | B(11) };
    track_with(16, st);
    CHECK(outcomes(100, a, 2));
    track_with(16, st2);
    CHECK(outcomes(100, b, 2));
    track_with(16, one);
    r.sync = 100;
    CHECK(rendered() && memcmp(&out, &orig, sizeof out) == 0);   /* a lone trig */
}

/* SKIP: no trig moves into the steps it clears, none comes out of them */
static void test_skip(void)
{
    static const int st[] = { 0, 4, 8, 12, -1 };
    /* SKIP 4: 4 -> 3 would be cleared; 8 12 would resolve onto 4 (round the loop), weaker */
    static const unsigned s4[] = { B(4) | B(7) | B(12), B(4) | B(8) | B(11) };
    /* SKIP 1: 4 8 12 are the trigs; 12 4 resolve onto 8, or 8 alone */
    static const unsigned s1[] = { B(3) | B(8) | B(11), B(4) | B(7) | B(12) };
    track_with(16, st);
    r.skip = 4;
    CHECK(outcomes(100, s4, 2));
    r.skip = 1;
    CHECK(outcomes(100, s1, 2));
}

static void test_args(void)
{
    static const int st[] = { 0, 4, -1 };
    track_with(16, st);
    r.sync = 50;
    r.metr = SEED_METR_N;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
    r.metr = -1;
    CHECK(seed_render(&out, &orig, &r, &c, &work) == SEED_E_ARG);
}

static void random_sync_recipe(struct seed_recipe *q)
{
    random_recipe(q);
    q->sync = (int16_t)rnd(1, 100);
    q->metr = (int16_t)rnd(0, SEED_METR_N - 1);
    q->beat = (int16_t)rnd(1, 8);
    q->bias = (int16_t)(rnd(0, 1) ? 0 : rnd(-100, 100));
    q->fld = SEED_FLD_OFF;                                  /* moves values between trigs */
}

/* the step one grid step before s, round the loop */
static int before_step(const struct seed_recipe *q, int s, int n)
{
    int g = q->src == SEED_SRC_RANDOM && (q->grid == 2 || q->grid == 4 || q->grid == 8) ? q->grid : 1;
    return ((s - g) % n + n) % n;
}

/* a (SYNC 0) against b (SYNC on), same recipe otherwise: every difference in trigs is a move one
 * grid step earlier onto a weaker step at or past SKIP, past no other trig, and every run of
 * moved trigs resolves. Returns the number of moves, or -1. */
static int check_moves(const struct seed_track *a, const struct seed_track *b, const struct seed_recipe *q)
{
    int n = a->length, skip = q->skip < 0 ? 0 : q->skip, i, j, moves = 0;
    uint8_t moved_to[SEED_STEPS] = { 0 };
    for (i = 0; i < n; i++) {
        int ta = (a->s[i].flags & SEED_TRIG) != 0, tb = (b->s[i].flags & SEED_TRIG) != 0;
        if (ta && !tb) {                                    /* vacated: its trig went to p */
            int p = before_step(q, i, n);
            if (i < skip || p < skip || !(b->s[p].flags & SEED_TRIG) || (a->s[p].flags & SEED_TRIG))
                return -1;
            if (seed_rank(q->metr, q->beat, SEED_ROLE_METER, p, n) >= seed_rank(q->metr, q->beat, SEED_ROLE_METER, i, n))
                return -1;
            for (j = (p + 1) % n; j != i; j = (j + 1) % n)
                if (j >= skip && (a->s[j].flags & SEED_TRIG))
                    return -1;                              /* moved past another trig */
            moved_to[p] = 1;
            moves++;
        }
    }
    for (i = 0; i < n; i++) {
        int ta = (a->s[i].flags & SEED_TRIG) != 0, tb = (b->s[i].flags & SEED_TRIG) != 0;
        if (tb && !ta && !moved_to[i])
            return -1;                                      /* a trig from nowhere */
    }
    /* runs: two or more consecutive trigs moved (a's trigs from SKIP on, in order) end before a
     * trig that stays and is stronger than every step they left */
    {
        int tr[SEED_STEPS], mv[SEED_STEPS], m = 0, start, len = 0, top = 0;
        for (i = skip; i < n; i++)
            if (a->s[i].flags & SEED_TRIG) {
                mv[m] = !(b->s[i].flags & SEED_TRIG);
                tr[m++] = i;
            }
        for (start = 0; start < m && mv[start]; start++)
            ;
        if (m && start == m)
            return moves ? -1 : 0;                          /* every trig moved */
        for (j = 1; j <= m; j++) {
            int x = (start + j) % m, v = seed_rank(q->metr, q->beat, SEED_ROLE_METER, tr[x], n);
            if (mv[x]) {
                len++;
                top = v > top ? v : top;
                continue;
            }
            if (len >= 2 && v <= top)
                return -1;
            len = top = 0;
        }
    }
    return moves;
}

/* count (I5), placement, resolution, SKIP: over random tracks and recipes, every layer on */
static void test_props_moves(void)
{
    static struct seed_track a, b;
    struct seed_recipe q;
    int i, ok = 1, okc = 1, moves = 0, grid_moves = 0, skip_moves = 0, len;
    for (i = 0; i < ROUNDS; i++) {
        random_track(&orig, len = rnd(1, 64));
        random_sync_recipe(&q);
        q.sync = 0;
        ok &= seed_render(&a, &orig, &q, &c, &work) == SEED_OK;
        q.sync = (int16_t)rnd(1, 100);
        ok &= seed_render(&b, &orig, &q, &c, &work) == SEED_OK;
        okc &= ntrigs(&a) == ntrigs(&b);
        {
            int mv = check_moves(&a, &b, &q);
            if (mv < 0) {
                ok = 0;
                printf("  moves: round %d\n", i);
                continue;
            }
            moves += mv;
            grid_moves += mv && q.src == SEED_SRC_RANDOM && q.grid > 1;
            skip_moves += mv && q.skip > 0;
        }
    }
    CHECK(ok);
    CHECK(okc);
    printf("SYNC: %d moves in %d takes (GRID > 1: %d takes, SKIP: %d)\n", moves, ROUNDS, grid_moves, skip_moves);
    CHECK(moves > ROUNDS / 3);                                  /* SYNC did move trigs */
    CHECK(grid_moves > 20 && skip_moves > 20);
    (void)len;
}

/* I4: raising SYNC keeps every move it had (the vacated steps nest) */
static void test_monotonic(void)
{
    static struct seed_track prev, b;
    struct seed_recipe q;
    int i, j, pct, ok = 1, grew = 0;
    for (i = 0; i < ROUNDS / 6; i++) {
        random_track(&orig, rnd(2, 64));
        random_sync_recipe(&q);
        q.sync = 0;
        ok &= seed_render(&prev, &orig, &q, &c, &work) == SEED_OK;
        {
            static struct seed_track base;
            int had = 0;
            base = prev;
            for (pct = 1; pct <= 100; pct += rnd(1, 15)) {
                int now = 0;
                q.sync = (int16_t)pct;
                ok &= seed_render(&b, &orig, &q, &c, &work) == SEED_OK;
                for (j = 0; j < b.length; j++) {
                    int vac_prev = (base.s[j].flags & SEED_TRIG) && !(prev.s[j].flags & SEED_TRIG);
                    int vac_now = (base.s[j].flags & SEED_TRIG) && !(b.s[j].flags & SEED_TRIG);
                    ok &= !vac_prev || vac_now;
                    now += vac_now;
                }
                grew += now > had;
                had = now;
                prev = b;
            }
        }
    }
    CHECK(ok);
    CHECK(grew > ROUNDS / 12);
}

/* SOURCE = TRACK: a moved step is the original step, origin and lanes included (the device then
 * moves its parameter locks); the vacated step carries the empty one; swing stays */
static void test_origins(void)
{
    static struct seed_track b;
    struct seed_recipe q;
    int i, j, ok = 1, moved = 0;
    for (i = 0; i < ROUNDS; i++) {
        random_track(&orig, rnd(2, 64));
        seed_recipe_defaults(&q, &test_caps);
        q.seed = (uint32_t)rnd(0, 9999);
        q.metr = (int16_t)rnd(0, SEED_METR_N - 1);
        q.beat = (int16_t)rnd(1, 8);
        q.rot = (int16_t)(rnd(0, 1) ? 0 : rnd(-20, 20));
        q.skip = (int16_t)(rnd(0, 2) ? 0 : rnd(0, 8));
        q.sync = (int16_t)rnd(1, 100);
        ok &= seed_render(&b, &orig, &q, &c, &work) == SEED_OK;
        for (j = 0; j < b.length; j++) {
            const struct seed_step *s = &b.s[j];
            if (s->origin < 0) {
                ok &= j < q.skip;                          /* only SKIP leaves no origin */
                continue;
            }
            ok &= same_but_swing(s, &orig.s[s->origin], orig.s[j].flags & SEED_SWING)
                  || (j < q.skip);
            moved += (s->flags & SEED_TRIG) && s->origin != ((j - q.rot) % b.length + b.length) % b.length;
        }
        ok &= q.skip > 0 || ntrigs(&b) == ntrigs(&orig);
    }
    CHECK(ok);
    CHECK(moved > ROUNDS / 2);
}

/* I2, I6 and I3 (props.h) with SYNC on: PLACE moves steps, the later layers own only their fields */
static void test_props_frame(void)
{
    struct prop_rng g = { 55 };
    int k, i2 = 0, i3 = 0, L;
    for (k = 0; k < 600; k++) {
        static struct seed_track t;
        struct seed_recipe q;
        struct seed_ctx cx;
        prop_track(&t, &g, prop_int(&g, 1, SEED_STEPS), prop_int(&g, 0, 100));
        prop_ctx(&cx, &t, prop_int(&g, 0, 12));
        prop_recipe(&q, &g, 50);
        q.sync = (int16_t)prop_int(&g, 1, 100);
        i2 += !prop_i2_i6(&t, &q, &cx);
        for (L = SEED_L_DYN; L < SEED_L_N; L++) {
            unsigned ch;
            i3 += !prop_i3(&t, &q, &cx, &g, (enum seed_layer)L, &ch);
        }
    }
    CHECK(i2 == 0);
    CHECK(i3 == 0);
}

/* P1: SYNC draws from its own stream; another seed tries the candidates in another order */
static void test_seeds(void)
{
    static struct seed_track a, b;
    struct seed_recipe q;
    int i, differ = 0;
    for (i = 0; i < 200; i++) {
        random_track(&orig, 32);
        seed_recipe_defaults(&q, &test_caps);
        q.sync = 50;
        q.seed = (uint32_t)i;
        seed_render(&a, &orig, &q, &c, &work);
        q.seed = (uint32_t)i + 1000u;
        seed_render(&b, &orig, &q, &c, &work);
        differ += memcmp(&a, &b, sizeof a) != 0;
    }
    printf("SYNC 50: another seed gives another take on %d of 200 tracks\n", differ);
    CHECK(differ > 5);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    CHECK(seed_have_sync == 1);
    test_single();
    test_four_on_the_floor();
    test_runs();
    test_skip();
    test_args();
    test_props_moves();
    test_monotonic();
    test_origins();
    test_props_frame();
    test_seeds();
    printf("test_place_sync: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
