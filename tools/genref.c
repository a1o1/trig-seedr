/* genref: what GENERATOR writes on a new project's track (default note 60, swing trigs on every
 * second step), for comparing with the emulator. Usage: genref SEED [LENGTH]
 * The parameters are seedr 0.1's defaults: grid 1/16, density 50%, velocity 60-120, length,
 * note and micro timing left alone, no accents, conditions or retrigs. Prints, per step, the trig
 * (x or .) and the velocity in hex (ff = the track's default), then a line of the µtimes.
 *   genref SEED [LENGTH [GROOVE TIMING VELOCITY QUANT RANDOM SHIFT GSEED]]
 * applies grooves[GROOVE] (mods/seedr-ar2/grooves.h) after generating, as the GROOVE page does
 * with those knob values (track default velocity 100). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/gen.h"
#if __has_include("../mods/seedr-ar2/grooves-live.h")
#include "../mods/seedr-ar2/grooves-live.h"
#else
#include "../mods/seedr-ar2/grooves-swing.h"
#endif

int main(int argc, char **argv)
{
    struct gen_track t;
    struct gen_params p;
    int i, n = argc > 2 ? atoi(argv[2]) : 16;
    memset(&t, 0, sizeof t);
    t.length = n;
    t.default_note = 60;
    for (i = 0; i < GEN_STEPS; i++) {
        t.s[i].flags = (i & 1) ? GEN_SWING : 0;
        t.s[i].note = t.s[i].vel = t.s[i].len = GEN_DEFAULT;
        t.s[i].cond = GEN_NO_COND;
        t.s[i].rlen = GEN_NEW_RLEN;
        t.s[i].rrate = GEN_NEW_RRATE;
        t.s[i].slock = GEN_NO_LOCK;
    }
    gen_defaults(&p);
    p.vel_lo = 60;
    p.vel_hi = 120;
    p.seed = (uint32_t)atoi(argv[1]);
    if (argc > 10)
        p.dens = atoi(argv[10]);           /* genref SEED LEN G T V Q R S GS DENS [SMP LO HI] */
    if (argc > 13) {
        p.smp = atoi(argv[11]); p.smp_lo = atoi(argv[12]); p.smp_hi = atoi(argv[13]);
    }
    if (argc > 14)
        p.rot = atoi(argv[14]);            /* ... SMP LO HI ROT */
    gen_generate(&t, &p);
    if (argc > 9) {
        struct gen_groove_params g;
        memset(&g, 0, sizeof g);
        g.timing = atoi(argv[4]); g.velocity = atoi(argv[5]); g.quant = atoi(argv[6]);
        g.random = atoi(argv[7]); g.shift = atoi(argv[8]); g.seed = (uint32_t)atoi(argv[9]);
        gen_groove_apply(&t, 0, n, &grooves[atoi(argv[3])], &g, 100);
    }
    if (getenv("GENREF_ROT")) {            /* "K WHAT MODE": ROTATE afterwards, as the page does */
        int k = 0, w = 0, m = 0;
        sscanf(getenv("GENREF_ROT"), "%d %d %d", &k, &w, &m);
        gen_rotate(&t, 0, n, k, (enum gen_what)w, m ? GEN_R_GRID : GEN_R_TRIG);
    }
    for (i = 0; i < n; i++)
        printf("%c%02x ", t.s[i].flags & GEN_TRIG ? 'x' : '.', t.s[i].vel);
    printf("\nutime");
    for (i = 0; i < n; i++)
        printf(" %d", t.s[i].utime);
    printf("\nslock");
    for (i = 0; i < n; i++)
        printf(" %d", t.s[i].slock);
    printf("\n");
    return 0;
}
