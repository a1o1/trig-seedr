/* SPDX-License-Identifier: GPL-2.0-or-later */
/* canary-st: a test build for a Syntakt unit (docs/plan-syntakt.md S6, docs/flashing-st.md). It checks on the unit
 * what status/U1.md could not in analog-emu: that Syntakt OS 1.42 never writes the memory U1 found unused, also
 * with all 64 sample slots loaded. It changes nothing the unit does but write that memory and draw a mark.
 *
 * Built with tools/st_build.sh --core mini in ST_AREA=47bf0000-47c00000, the top 64 KiB of U1's range, so the range
 * it checks, 0x478b0000-0x47bf0000 (3,264 KiB; seedr's 1 MiB area is inside it), does not hold its own code.
 *
 *   ev_tick  fills the range with a pattern that depends on each word's address, 64 KiB a tick; then checks it
 *            again and again, 16 KiB a tick (a whole pass about every 7 s at the 30 Hz tick). The first word that
 *            differs stops the checks and is kept
 *   ev_draw  a box in the bottom right corner of every screen: FILL while filling, OK n after n whole passes,
 *            BAD and the address (hex) when a word changed
 * tools/st_canary.sh reads `canary` from a snapshot by symbol. */
#include <stdint.h>

#define LO       0x478b0000u
#define HI       0x47bf0000u
#define FILL     (64u << 10)              /* bytes filled a tick */
#define STEP     (16u << 10)              /* bytes checked a tick */
#define PATTERN(a) ((a) ^ 0x5a3cc3a5u)

/* one struct, so one symbol to read: 32-bit words in this order */
struct canary {
    uint32_t magic;                       /* 'CANR' once the first tick ran */
    uint32_t state;                       /* 0 filling, 1 checking, 2 a word changed */
    uint32_t at;                          /* the next address to fill or check */
    uint32_t passes;                      /* whole passes checked with every word as written */
    uint32_t bad, bad_val;                /* the first word that differed and what it held */
    uint32_t ticks;
};
struct canary canary;

void canary_tick(void *ctrl)
{
    volatile uint32_t *p;
    uint32_t a, end;
    (void)ctrl;
    if (!canary.ticks++) {
        canary.magic = 0x43414e52u;
        canary.at = LO;
    }
    if (canary.state == 0) {
        end = canary.at + FILL > HI ? HI : canary.at + FILL;
        for (a = canary.at, p = (volatile uint32_t *)a; a < end; a += 4)
            *p++ = PATTERN(a);
        canary.at = end;
        if (end == HI) {
            canary.state = 1;
            canary.at = LO;
        }
    } else if (canary.state == 1) {
        end = canary.at + STEP;
        for (a = canary.at, p = (volatile uint32_t *)a; a < end; a += 4, p++)
            if (*p != PATTERN(a)) {
                canary.bad = a;
                canary.bad_val = *p;
                canary.state = 2;
                return;
            }
        canary.at = end;
        if (end == HI) {
            canary.passes++;
            canary.at = LO;
        }
    }
}

/* ---- drawing: the bitmap is 128 x 64, 8 bytes a column, row 0 the top in bit 0 of the column's last byte; text in
 * the OS's 6-pixel font (as mods/seedr-st/seedr.c, status/S3.md) ---- */

#define ST_FONT 0x443db220u

static uint8_t *pixels(void *bmp)
{
    return *(uint8_t **)((uint8_t *)bmp + 16);
}

static void dot(uint8_t *p, int x, int row, int on)
{
    if (x < 0 || x > 127 || row < 0 || row > 63)
        return;
    if (on)
        p[8 * x + 7 - row / 8] |= (uint8_t)(1 << (row % 8));
    else
        p[8 * x + 7 - row / 8] &= (uint8_t)~(1 << (row % 8));
}

/* s with its right end at column x, its top at row top; returns its width */
static int text(uint8_t *p, int x, int top, const char *s)
{
    const uint32_t *f = (const uint32_t *)((const uint32_t *)ST_FONT)[1];
    const uint8_t *w = (const uint8_t *)f[2], *g = (const uint8_t *)f[4];
    const uint16_t *off = (const uint16_t *)f[3];
    int h = (int)f[1], width = 0, i, r;
    const char *c;
    for (c = s; *c; c++)
        width += w[(uint8_t)*c] + 1;
    x -= width - 1;
    for (c = s; *c; c++) {
        uint8_t k = (uint8_t)*c;
        for (i = 0; i < w[k]; i++, x++)
            for (r = 0; r < h; r++)
                dot(p, x, top + r, (g[off[k] + i] >> (8 - h + r)) & 1);
        x++;
    }
    return width;
}

static char *num(char *o, uint32_t v, int hex)
{
    char t[10];
    int n = 0;
    do {
        uint32_t d = hex ? v & 15 : v % 10;
        t[n++] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
        v = hex ? v >> 4 : v / 10;
    } while (v && n < 10);
    while (n)
        *o++ = t[--n];
    *o = 0;
    return o;
}

void canary_draw(void *bmp, void *ctrl)
{
    uint8_t *p = pixels(bmp);
    char s[24], *o = s;
    const char *w;
    int x, row, width;
    (void)ctrl;
    if (canary.state == 0)
        w = "FILL";
    else {
        for (w = canary.state == 1 ? "OK " : "BAD "; *w; w++)
            *o++ = *w;
        num(o, canary.state == 1 ? canary.passes : canary.bad, canary.state == 2);
        w = s;
    }
    for (x = 0, width = 0; w[x]; x++)
        width += 6;                       /* a box wide enough for any character */
    for (x = 127 - width - 2; x <= 127; x++)
        for (row = 54; row <= 63; row++)
            dot(p, x, row, 0);
    text(p, 126, 56, w);
}
