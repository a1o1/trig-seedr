# trig-seedr 0.5

0.5 is version 1's milestone M1: every researched idea of the design (`design-v1.md`) is built. It is the release to
flash: it has 0.4.1's fixes and the device-layer fixes below.

Status: every check of `tools/run_tests.sh` passes in analog-emu on the Mac (24 test files, 126 golden vectors). Not
yet run on a unit. A build with MAP's data is GPL-3.0-or-later (README, License).

## New

| Page | What | Text |
|---|---|---|
| TAKE | METR (4/4, 3/4, 6/8, 12/16, 5/8, 7/8) and BEAT, with Clarence Barlow's indispensability as the steps' ranks | `docs/v1/meter.md` |
| TAKE | PHRS: a phrase of takes over the pages (AAAA, AAAB, ABAB, ABAC, ABCD); CVAR: takes n and n + 1 written as alternating trig conditions (1:2, AAAB) | `docs/v1/vary.md` |
| TAKE | QUAL: the best spread of 1..8 candidate takes (fewer long holes and clumps; at BIAS 0 no pull towards the beats) | `docs/v1/score.md` |
| RHYTHM | ROLE for RAND: KICK, BACK, HAT, PERC | `docs/v1/meter.md` |
| RHYTHM | EUCLID: the same patterns as stock's Euclidean mode | `docs/v1/euclid.md` |
| RHYTHM | MAP: a 5 × 5 map of drum patterns, X and Y shared by the pattern's tracks, PART and DENS per track, CHAOS | `docs/v1/map.md` |
| RHYTHM / PLACE | COPY another track's trigs; MASK: AVOID or ONLY another track's trigs (a track in Euclidean mode counts as it plays) | `docs/v1/copy-mask.md` |
| PLACE | SYNC: anticipation onto the weaker step before; the moved step keeps all its locks | `docs/v1/sync.md` |
| DYNAMICS | SHAPE RANK, NOISE and MAP; AMOD TOP (the strongest steps get the accents) | `docs/v1/meter.md`, `noise.md`, `map.md` |
| NOTE / LOCK | value sources LIST (UP, DOWN, PING, PNG2, ODD, RAND, its own length) and NOISE (a contour that loops without a jump); LMOD = LEGATO | `docs/v1/lists.md`, `noise.md` |
| TIME | DRFT: correlated drift of micro timing | `docs/v1/drift.md` |
| every page but VIEW | the bottom strip previews the take YES would write, before YES | `docs/v1/preview.md` |
| VIEW | both lanes' locks, muted trigs | `docs/v1/preview.md` |
| title bar | the sprout and the page's place (`VIEW BD 9/10`); the source: TRK, EUC, CPY, MAP (MAP while CHAOS is below 4), or S and the seed | |

## Fixed

- **Lock rows** (task D1): the device layer looked up stock's lock-row types in the FX track's table. It now uses
  the table stock uses per track (measured for every lockable parameter on BD, MT and FX), so a move with locks
  that needs no new row is no longer refused, and the room check counts as stock counts. Past the 71-row limit a
  take is refused, as before (decided; to be revisited after customer testing).
- From 0.4.1: every ColdFire build uses `-fno-store-merging` (a gcc 16.2 miscompile); new trigs keep stock's
  SYN / SMP / ENV bits.

## What a seed gives

The golden vectors frozen at 0.4 are unchanged, except two RAND takes with BIAS (one trig each moves: BIAS now
uses Barlow's ranks in place of 0.3's five levels). With every new knob at its default, a recipe gives the same take
as in 0.4.

## On the hardware, please check

- **CVAR AAAB** relies on how the Rytm's PRE condition counts: take n's trigs should play on loops 1–3, take
  n + 1's on loop 4.
- Trigs written by trig-seedr play their sample (SMP) as trigs from the TRIG keys do.
- The preview strip's speed on a 64-step track with lists (the slowest case measured: 6 ms a render in the
  emulator, against 1 ms usually).

## Choices kept until testing on the hardware

Each is a line or two to change: F1's role orders, BEAT counting the meter's note value, RANK and TOP ignoring
ROLE; F5's reading of "runs resolve onto a strong step"; F6's sorted lists and ODD order; F8's drift scale (±23 at
100 %) and per-track drift; F9's AAAB with PRE. The status files in `docs/status/` give each one.
