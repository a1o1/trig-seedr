/* SPDX-License-Identifier: GPL-2.0-or-later */
/* TOOLS YOLO (lib/yolo.c, 0.7): every roll, at every WILD, on every role and length, gives a recipe
 * the chain renders without a refusal; the same roll gives the same recipe; the meter and the
 * lanes' parameters stay the user's; the ranges stay musical (no condition, retrig or lock on most
 * trigs, a density that is neither empty nor full); WILD 0 stays gentle; and a roll from a source
 * that makes trigs makes some. YOLO_ROLLS=n sets the number of rolls per WILD (default 400). */
#include <stdio.h>
#include <stdlib.h>
#include "props.h"

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; if (fails < 20) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static struct seed_work work;

/* the layers rolled; EUCLID's turn (ROT) belongs to the source, not to a PLACE layer */
static int layers(const struct seed_recipe *r)
{
    int l, n = 0;
    for (l = SEED_L_PLACE; l < SEED_L_N; l++)
        n += seed_layer_active(r, (enum seed_layer)l);
    if (r->src == SEED_SRC_EUCLID && r->rot != 0 && r->sync == 0)
        n--;
    return n;
}

static void test_rolls(int rolls)
{
    static const int roles[] = { SEED_ROLE_KICK, SEED_ROLE_BACK, SEED_ROLE_HAT, SEED_ROLE_PERC, SEED_ROLE_METER };
    static const int lengths[] = { 16, 64, 24, 13, 32 };
    struct prop_rng g = { 0x7010 };
    int wild, roll, empty = 0, made = 0, track_kept[2] = { 0, 0 };
    for (wild = 0; wild <= 100; wild += 10)
        for (roll = 0; roll < rolls; roll++) {
            struct seed_track orig, out;
            struct seed_ctx c;
            struct seed_recipe from, r, r2;
            struct seed_yolo_in in;
            int len = lengths[roll % 5], role = roles[(roll / 5) % 5], rc, i, trigs = 0;
            prop_track(&orig, &g, len, roll % 7 == 0 ? 0 : 35);
            prop_ctx_full(&c, &orig, roll % 12, &g);
            seed_recipe_defaults(&from, prop_cap);
            from.metr = (int16_t)prop_int(&g, 0, SEED_METR_N - 1);
            from.beat = (int16_t)prop_int(&g, 1, 8);
            from.lane[0].parm = (int16_t)prop_int(&g, 0, prop_cap->nparams - 1);
            do
                from.lane[1].parm = (int16_t)prop_int(&g, 0, prop_cap->nparams - 1);
            while (from.lane[1].parm == from.lane[0].parm);
            in.caps = prop_cap;
            in.role = role;
            in.length = len;
            in.fields = orig.fields;
            in.ngrooves = PROP_GROOVES;
            for (in.any = 0, i = 0; i < len; i++)
                in.any |= orig.s[i].flags & SEED_TRIG;
            seed_yolo(&r, &from, roll, wild, &in);
            seed_yolo(&r2, &from, roll, wild, &in);
            CHECK(!memcmp(&r, &r2, sizeof r), "the same roll, another recipe: roll %d wild %d", roll, wild);
            CHECK(r.metr == from.metr && r.beat == from.beat && r.lane[0].parm == from.lane[0].parm &&
                  r.lane[1].parm == from.lane[1].parm, "the meter or a lane's parameter changed: roll %d", roll);
            rc = seed_render(&out, &orig, &r, &c, &work);
            CHECK(rc == SEED_OK, "refused (%d): roll %d wild %d role %d length %d src %d", rc, roll, wild, role, len, r.src);
            /* musical ranges */
            CHECK(r.cond <= 45 && r.rtrg <= 20 && r.lane[0].pct <= 35 && r.lane[1].pct == 0 && r.sync <= 50 && r.hum <= 15,
                  "a layer too wide: roll %d wild %d", roll, wild);
            CHECK(r.src != SEED_SRC_RANDOM || (r.dens >= 12 && r.dens <= 90), "DENS %d: roll %d", r.dens, roll);
            CHECK(r.src != SEED_SRC_COPY && r.mask == SEED_MASK_OFF, "another track read: roll %d", roll);
            CHECK(layers(&r) <= 4, "%d layers: roll %d wild %d", layers(&r), roll, wild);
            if (wild == 0)
                CHECK(layers(&r) <= 2 && r.lane[0].pct == 0 && r.note.lo == SEED_DEF && r.phrs == SEED_PHRS_OFF,
                      "WILD 0 not gentle: roll %d", roll);
            if (rc == SEED_OK && (r.src == SEED_SRC_RANDOM || r.src == SEED_SRC_EUCLID)) {
                made++;
                for (i = 0; i < len; i++)
                    trigs += !!(out.s[i].flags & SEED_TRIG);
                empty += trigs == 0;
            }
            if (in.any && wild <= 20)
                track_kept[r.src == SEED_SRC_TRACK]++;
        }
    CHECK(empty * 100 <= made, "%d of %d RANDOM / EUCLID rolls made no trig", empty, made);
    CHECK(track_kept[1] > track_kept[0], "WILD 0..20 on a track with trigs: SRC = TRACK %d times, another %d",
          track_kept[1], track_kept[0]);
}

int main(void)
{
    const char *n = getenv("YOLO_ROLLS");
    test_rolls(n ? atoi(n) : 400);
    printf("test_yolo: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
