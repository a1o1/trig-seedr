/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host tests for dyn_rank.c's AMOD = TOP with SRC = MAP (task F12): the accents against the
 * original's rule (a trig is accented when its level plus the part's chaos lift, clipped to 255,
 * is above 192), at the ACC share that counts them; the order of the share (I4) and its exact count
 * (I5); ties to the earlier step; the final positions (P5, after ROT); TOP with another source
 * still by the meter; ACC owning only the accent flag; seed_map_lift's arguments.
 *
 * The levels come from seed_map_level, which tools/tests/23-map.sh checks against the original
 * code; tools/tests/32-map-accents.sh checks the accents against the original code itself. */
#include "tests.h"

static struct seed_track orig, out;
static struct seed_recipe r;
static struct seed_ctx c;

static void base(int len, int part, int dens, int x, int y, int chaos)
{
    random_track(&orig, len);
    seed_recipe_defaults(&r, &test_caps);
    r.src = SEED_SRC_MAP;
    r.map_part = (int16_t)part;
    r.map_dens = (int16_t)dens;
    r.amod = SEED_AMOD_TOP;
    c.shared.map_x = (int16_t)x;
    c.shared.map_y = (int16_t)y;
    c.shared.map_chaos = (int16_t)chaos;
}

static int rendered(void)
{
    return seed_render(&out, &orig, &r, &c, &work) == SEED_OK;
}

/* the original's key for step i at take k: the level plus the part's perturbation for the pass,
 * byte * (CHAOS >> 2) >> 8, clipped to 255 (written out here, not taken from src_map.c) */
static int key_of(const struct seed_take *k, int part, int x, int y, int chaos, int i)
{
    int lev = seed_map_level(part, x, y, i), byte, pert;
    byte = (int)(seed_rand(k, SEED_SLOT(SEED_L_RHYTHM, SEED_P_CHAOS, part * 256 + i / 32)) >> 24);
    pert = byte * (chaos >> 2) >> 8;
    return lev < 255 - pert ? lev + pert : 255;
}

/* the smallest ACC % whose share of n trigs is m, or -1 */
static int acc_for(int m, int n)
{
    int p;
    for (p = 0; p <= 100; p++)
        if (seed_share(p, n) == m)
            return p;
    return -1;
}

/* the original's accents: over X / Y, parts, DENS, CHAOS, seeds and lengths, ACC at the share that
 * counts the trigs above 192 accents exactly those trigs */
static void test_original(void)
{
    static const int xs[] = { 0, 40, 100, 128, 200, 255 }, dens[] = { 30, 62, 63, 100, 128, 190, 230, 255 },
                     chaos[] = { 0, 3, 4, 100, 255 }, lens[] = { 16, 32, 37, 64 };
    struct seed_take k;
    int xi, yi, p, d, ci, li, i, cases = 0, found = 1, ok = 1, some = 0, none = 0, all = 0;
    for (xi = 0; xi < 6; xi++)
        for (yi = 0; yi < 6; yi++)
            for (p = 0; p < 3; p++)
                for (d = 0; d < 8; d++)
                    for (ci = 0; ci < 5; ci++)
                        for (li = 0; li < 4; li++) {
                            int x = xs[xi], y = xs[5 - yi], n = lens[li], m = 0, trig = 0, pct;
                            int want[SEED_STEPS];
                            base(n, p, dens[d], x, y, chaos[ci]);
                            r.seed = (uint32_t)(1 + cases % 7);
                            seed_take_init(&k, r.seed, (uint32_t)c.track, 0, 0);
                            for (i = 0; i < n; i++) {
                                int key = key_of(&k, p, x, y, chaos[ci], i);
                                want[i] = key > 255 - dens[d] && key > 192;
                                trig += key > 255 - dens[d];
                                m += want[i];
                            }
                            pct = acc_for(m, trig);
                            cases++;
                            if (pct < 0) {
                                found = 0;
                                continue;
                            }
                            r.acc = (int16_t)pct;
                            if (!rendered()) {
                                ok = 0;
                                continue;
                            }
                            for (i = 0; i < n; i++)
                                ok &= ((out.s[i].flags & SEED_ACCENT) != 0) == want[i];
                            some += m > 0 && m < trig;
                            none += m == 0 && trig > 0;
                            all += m > 0 && m == trig;
                        }
    CHECK(found);
    CHECK(ok);
    CHECK(some > 100 && none > 100 && all > 100);       /* the cases cover a part, none and all */
    printf("  original's accents: %d cases (some %d, none %d, all %d)\n", cases, some, none, all);
}

/* raising ACC keeps the accents it had (I4); the count is the share (I5); only trigs */
static void test_share(void)
{
    uint16_t prev[SEED_STEPS];
    int pct, i, m, t, nest = 1, exact = 1, only = 1;
    base(64, 2, 200, 100, 200, 255);
    r.seed = 5;
    memset(prev, 0, sizeof prev);
    for (pct = 1; pct <= 100; pct++) {
        r.acc = (int16_t)pct;
        CHECK(rendered());
        for (i = m = t = 0; i < 64; i++) {
            m += (out.s[i].flags & SEED_ACCENT) != 0;
            t += (out.s[i].flags & SEED_TRIG) != 0;
            only &= !(out.s[i].flags & SEED_ACCENT) || (out.s[i].flags & SEED_TRIG);
            nest &= !(prev[i] & SEED_ACCENT) || (out.s[i].flags & SEED_ACCENT);
            prev[i] = out.s[i].flags;
        }
        exact &= m == seed_share(pct, t);
    }
    CHECK(nest && exact && only);
}

/* the accents follow the key: no accented trig has a lower key than a trig without; equal keys go
 * to the earlier step (DENS 255 at X = Y = 0, CHAOS 255: many keys clip to 255) */
static void test_order(void)
{
    struct seed_take k;
    int pct, i, j, ok = 1, ties = 0;
    for (pct = 5; pct <= 95; pct += 5) {
        base(64, 0, 255, 0, 0, 255);
        r.seed = 11;
        r.acc = (int16_t)pct;
        CHECK(rendered());
        seed_take_init(&k, r.seed, (uint32_t)c.track, 0, 0);
        for (i = 0; i < 64; i++)
            for (j = 0; j < 64; j++) {
                int ki = key_of(&k, 0, 0, 0, 255, i), kj = key_of(&k, 0, 0, 0, 255, j);
                int ai = (out.s[i].flags & SEED_ACCENT) != 0, aj = (out.s[j].flags & SEED_ACCENT) != 0;
                if (!(out.s[i].flags & SEED_TRIG) || !(out.s[j].flags & SEED_TRIG) || !ai || aj)
                    continue;
                ok &= ki > kj || (ki == kj && i < j);
                ties += ki == kj;
            }
    }
    CHECK(ok);
    CHECK(ties > 0);
}

/* P5: after PLACE the key is read at the trig's final position, as SHAPE = MAP's level */
static void test_rot(void)
{
    struct seed_take k;
    int i, j, ok = 1, moved = 0;
    struct seed_track plain;
    base(32, 1, 160, 128, 64, 0);
    r.acc = 30;
    CHECK(rendered());
    plain = out;
    r.rot = 3;
    CHECK(rendered());
    seed_take_init(&k, r.seed, (uint32_t)c.track, 0, 0);
    for (i = 0; i < 32; i++)
        for (j = 0; j < 32; j++)
            if ((out.s[i].flags & SEED_ACCENT) && (out.s[j].flags & SEED_TRIG) && !(out.s[j].flags & SEED_ACCENT))
                ok &= key_of(&k, 1, 128, 64, 0, i) > key_of(&k, 1, 128, 64, 0, j) ||
                      (key_of(&k, 1, 128, 64, 0, i) == key_of(&k, 1, 128, 64, 0, j) && i < j);
    for (i = 0; i < 32; i++)
        moved += ((plain.s[i].flags & SEED_ACCENT) != 0) != ((out.s[(i + 3) % 32].flags & SEED_ACCENT) != 0);
    CHECK(ok);
    CHECK(moved > 0);                                   /* the accents did not rotate with the trigs */
}

/* with another source TOP ranks by the meter, whatever the MAP settings say; with MAP the metre
 * has no effect; ACC changes the accent flag and nothing else */
static void test_sources(void)
{
    struct seed_track a;
    int i, ok = 1, owned = 1, src;
    for (src = SEED_SRC_TRACK; src <= SEED_SRC_EUCLID; src++) {
        base(16, 0, 128, 0, 0, 0);
        r.src = (int16_t)src;
        r.acc = 25;
        if (src == SEED_SRC_TRACK)
            for (i = 0; i < 16; i++)
                orig.s[i].flags |= SEED_TRIG;
        if (src == SEED_SRC_RANDOM)
            r.dens = 100;
        if (src == SEED_SRC_EUCLID)
            r.eu_puls = 16;
        CHECK(rendered());
        for (i = 0; i < 16; i++)                         /* 4/4: the beats */
            ok &= ((out.s[i].flags & SEED_ACCENT) != 0) == (i % 4 == 0);
        a = out;
        r.map_part = 2;
        c.shared.map_x = 200;
        c.shared.map_chaos = 255;
        CHECK(rendered());
        ok &= memcmp(&a, &out, sizeof a) == 0;
    }
    CHECK(ok);
    base(64, 1, 180, 60, 190, 100);
    r.acc = 40;
    CHECK(rendered());
    a = out;
    r.metr = SEED_METR_7_8;
    r.beat = 2;
    CHECK(rendered());
    CHECK(memcmp(&a, &out, sizeof a) == 0);
    r.acc = 0;
    r.vel.lo = r.vel.hi = 90;                           /* DYNAMICS active without ACC */
    CHECK(rendered());
    a = out;
    r.acc = 40;
    CHECK(rendered());
    for (i = 0; i < SEED_STEPS; i++) {
        struct seed_step s = out.s[i];
        s.flags = (uint16_t)((s.flags & ~SEED_ACCENT) | (a.s[i].flags & SEED_ACCENT));
        owned &= memcmp(&s, &a.s[i], sizeof s) == 0;
    }
    CHECK(owned);
}

/* seed_map_lift: its arguments, its bound (62) and CHAOS below 4 */
static void test_lift(void)
{
    struct seed_take k;
    int s, ok = 1, top = 0;
    seed_take_init(&k, 1, 0, 0, 0);
    CHECK(seed_map_lift(&k, -1, 0, 255) == SEED_E_ARG);
    CHECK(seed_map_lift(&k, 3, 0, 255) == SEED_E_ARG);
    CHECK(seed_map_lift(&k, 0, -1, 255) == SEED_E_ARG);
    for (s = 1; s <= 300; s++) {
        int v;
        seed_take_init(&k, (uint32_t)s, 0, 0, 0);
        v = seed_map_lift(&k, s % 3, s % 64, 255);
        ok &= v >= 0 && v <= 62 && seed_map_lift(&k, s % 3, s % 64, 3) == 0 && seed_map_lift(&k, s % 3, s % 64, -5) == 0;
        ok &= v == seed_map_lift(&k, s % 3, s % 64, 999);
        ok &= seed_map_lift(&k, s % 3, 0, 255) == seed_map_lift(&k, s % 3, 31, 255);
        if (v > top)
            top = v;
    }
    CHECK(ok);
    CHECK(top > 50);
}

int main(void)
{
    test_setup();
    c = test_ctx(0);
    test_original();
    test_share();
    test_order();
    test_rot();
    test_sources();
    test_lift();
    printf("test_dyn_rank: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
