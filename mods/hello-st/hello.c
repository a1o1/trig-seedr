/* SPDX-License-Identifier: GPL-2.0-or-later */
/* hello-st: a measurement mod for the Syntakt OS 1.42 (docs/plan-syntakt.md, task S0.3). It changes nothing the
 * unit does: it counts the core's UI events and, on its first tick, measures the native heap. tools/st_s03.sh reads
 * `hello` from the snapshot by symbol.
 *
 *   ev_tick  counts; the first one (task context) allocates seedr's buffers' size from the native heap, writes
 *            both ends, and finds the largest block the heap gives (a search to 4 KiB), freeing each
 *   ev_draw  counts
 *   ev_key   counts; keeps the last key ID (event+12) and flags (event+16); never consumes
 *   ev_enc   counts
 */
#include <stdint.h>

#define FW_MALLOC ((void *(*)(uint32_t))0x401036d0)    /* native allocator, task context (core-st fw.s) */
#define FW_FREE   ((void (*)(void *))0x401038d6)
#define EV_INT(ev, off) (*(const int32_t *)((const uint8_t *)(ev) + (off)))

#define SEEDR_BSS  107336u                  /* seedr 0.7.1's .bss on the Rytm: what would move to the heap */
#define HEAP_MAX   (16u << 20)              /* the native heap's arena (Synfact: 16 MiB) */

/* one struct, so one symbol to read: fields are 32-bit words in this order */
struct hello {
    uint32_t magic;                         /* 'HELO' once the first tick ran */
    uint32_t ticks, draws, keys, encs;
    int32_t last_key, last_flags;
    uint32_t buf;                           /* SEEDR_BSS bytes from the heap (address), 0 on failure */
    uint32_t buf_ok;                        /* 1: both ends written and read back */
    uint32_t largest;                       /* the largest block the heap gave, in bytes */
};
struct hello hello;

static int good(const void *p)
{
    return (uint32_t)p >= 0x40000000u;      /* core-st's test: below is a failure */
}

static void measure(void)
{
    uint8_t *p = FW_MALLOC(SEEDR_BSS);
    uint32_t lo = 0, hi = HEAP_MAX;
    if (good(p)) {
        p[0] = 0x5a;
        p[SEEDR_BSS - 1] = 0xa5;
        hello.buf = (uint32_t)p;
        hello.buf_ok = p[0] == 0x5a && p[SEEDR_BSS - 1] == 0xa5;
    }
    while (hi - lo > 4096) {                /* with seedr's buffer held, as seedr would hold it */
        uint32_t mid = lo + (hi - lo) / 2;
        void *q = FW_MALLOC(mid);
        if (good(q)) {
            FW_FREE(q);
            lo = mid;
        } else
            hi = mid;
    }
    hello.largest = lo;
    hello.magic = 0x48454c4fu;
}

void hello_tick(void *ctrl)
{
    (void)ctrl;
    if (!hello.ticks++)
        measure();
}

void hello_draw(void *bmp, void *ctrl)
{
    (void)bmp;
    (void)ctrl;
    hello.draws++;
}

int hello_key(void *brain, void *ev)
{
    (void)brain;
    hello.keys++;
    hello.last_key = EV_INT(ev, 12);
    hello.last_flags = EV_INT(ev, 16);
    return 0;
}

int hello_enc(void *brain, void *ev)
{
    (void)brain;
    (void)ev;
    hello.encs++;
    return 0;
}
