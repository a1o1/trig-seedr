/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host test of the Syntakt's parameter tables (st_params.h, plan-syntakt.md task S0.5): each machine and
 * page is well formed (names, ranges, stock ids unique and inside the table's 505), every track's menu
 * offers a machine, and st_track_params gives each track a list lib/ can take (seed_param.id its place,
 * the lanes' defaults inside it, lane 2 on the filter's frequency, no stock id twice). */
#include <stdio.h>
#include <string.h>
#include "st_params.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static const struct st_machine machines[ST_NMACHINES] = ST_MACHINES;

static size_t len(const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n])
        n++;
    return n;
}

static void check_param(const struct st_param *p)
{
    size_t n = len(p->name, SEED_NAME);
    CHECK(n >= 1 && n < SEED_NAME);
    CHECK(p->id < 505);
    CHECK(p->lo < p->hi && p->lo >= 0 && p->hi <= 127);
}

static void check_page(const struct st_param *p, int n)
{
    int i, k;
    CHECK(n >= 1);
    for (i = 0; i < n; i++) {
        check_param(&p[i]);
        for (k = 0; k < i; k++)
            CHECK(p[k].id != p[i].id);
    }
}

static void test_machines(void)
{
    int i, k, t;
    for (i = 0; i < ST_NMACHINES; i++) {
        const struct st_machine *m = &machines[i];
        CHECK(m->n >= 1 && m->n <= 8);
        CHECK(m->lane1 < m->n);
        CHECK(m->tracks != 0 && (m->tracks & ~0xfffu) == 0);
        CHECK(m->role < SEED_ROLE_N);
        CHECK(len(m->name, sizeof m->name) < sizeof m->name && m->name[0]);
        check_page(m->p, m->n);
        for (k = 0; k < i; k++)
            CHECK(machines[k].type != m->type);
        CHECK(st_machine_of(machines, ST_NMACHINES, m->type) == m);
    }
    CHECK(st_machine_of(machines, ST_NMACHINES, 21) == 0);          /* CP CLASSIC: in no menu */
    for (t = 0; t < 12; t++) {
        int offered = 0;
        for (i = 0; i < ST_NMACHINES; i++)
            offered += (machines[i].tracks >> t) & 1;
        CHECK(offered >= 13);                                       /* 13 digital, 20 drum, 14 cymbal */
    }
}

static void test_pages(void)
{
    static const struct st_param fltr[] = ST_PAGE_FLTR, basew[] = ST_PAGE_BASEW, amp[] = ST_PAGE_AMP,
                                 lfo[] = ST_PAGE_LFO, fxf[] = ST_PAGE_FX_FLTR, fxa[] = ST_PAGE_FX_AMP;
    check_page(fltr, ST_PAGE_FLTR_N);
    check_page(basew, ST_PAGE_BASEW_N);
    check_page(amp, ST_PAGE_AMP_N);
    check_page(lfo, ST_PAGE_LFO_N);
    check_page(fxf, ST_PAGE_FX_FLTR_N);
    check_page(fxa, ST_PAGE_FX_AMP_N);
}

static void check_track(const struct st_machine *m, int track)
{
    struct seed_param out[ST_TRACK_PARAMS_MAX];
    uint16_t ids[ST_TRACK_PARAMS_MAX];
    int16_t lane[SEED_LANES] = { -1, -1 };
    int n = st_track_params(m, track, out, ids, lane), i, k;
    CHECK(n >= 1 && n <= ST_TRACK_PARAMS_MAX);
    CHECK(lane[0] >= 0 && lane[0] < n && lane[1] >= 0 && lane[1] < n && lane[0] != lane[1]);
    CHECK(strcmp(out[lane[1]].name, "FRQ") == 0);
    for (i = 0; i < n; i++) {
        CHECK(out[i].id == i);
        CHECK(out[i].lo < out[i].hi);
        for (k = 0; k < i; k++)
            CHECK(ids[k] != ids[i]);
    }
}

static void test_tracks(void)
{
    int i, t;
    for (t = 0; t < 12; t++)
        for (i = 0; i < ST_NMACHINES; i++)
            if ((machines[i].tracks >> t) & 1)
                check_track(&machines[i], t);
    check_track(0, ST_FX_TRACK);
    check_track(0, 0);                                              /* DISABLE: the pages alone */
}

/* A machine change on a track (the Syntakt mod's st_machine_sync, SD4): a lane follows its stock id into the
 * new machine's list. Every page parameter is found again in the same row of the page, and a SYN control
 * only when the new machine has that stock id, at the knob it has there. */
static int find(const uint16_t *ids, int n, uint16_t sid)
{
    int k;
    for (k = 0; k < n && ids[k] != sid; k++)
        ;
    return k < n ? k : -1;
}

static void test_machine_change(void)
{
    struct seed_param out[2][ST_TRACK_PARAMS_MAX];
    uint16_t ids[2][ST_TRACK_PARAMS_MAX];
    int16_t lane[2][SEED_LANES];
    int t, a, b, k, m;
    for (t = 0; t < 12; t++)
        for (a = 0; a < ST_NMACHINES; a++)
            for (b = 0; b < ST_NMACHINES; b++) {
                const struct st_machine *ma = &machines[a], *mb = &machines[b];
                int na, nb;
                if (!((ma->tracks >> t) & 1) || !((mb->tracks >> t) & 1))
                    continue;
                na = st_track_params(ma, t, out[0], ids[0], lane[0]);
                nb = st_track_params(mb, t, out[1], ids[1], lane[1]);
                CHECK(find(ids[1], nb, ids[0][lane[0][1]]) == lane[1][1]);   /* lane 2's default: FRQ */
                for (k = 0; k < na; k++) {
                    int at = find(ids[1], nb, ids[0][k]);
                    if (k >= ma->n) {                                       /* a page's: found, same row */
                        CHECK(at == k - ma->n + mb->n);
                        CHECK(at >= 0 && strcmp(out[0][k].name, out[1][at].name) == 0);
                    } else {                                                /* a SYN control */
                        for (m = 0; m < mb->n && mb->p[m].id != ids[0][k]; m++)
                            ;
                        CHECK(at == (m < mb->n ? m : -1));
                    }
                }
            }
}

int main(void)
{
    test_machines();
    test_pages();
    test_tracks();
    test_machine_change();
    printf("test_st_params: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
