# trig-seedr: TURING (version 0.9)

Draft, 2026-11-20. A Turing machine for trig-seedr: a loop of steps that changes a little each time it comes round,
after the Music Thing Turing Machine, the déjà vu loop of Mutable Instruments' Marbles and Moog's Labyrinth (credited
in the acknowledgements; their names are not used on screen, as plan-v1 rule 4 asks for Grids). Alan's decisions
are TD1–TD5; the lead's own, taken so the work could go ahead without him and to be checked on a unit, are L1–L12.
Testing on units decides what changes.

## 1. Decisions

| # | Decision | By |
|---|---|---|
| TD1 | The live machine is a **page at the end of seedr's ring**, after TOOLS: TURING. Not a fourth TRIG press | Alan |
| TD2 | Take the fast path for how a live run behaves; on-device testing tunes it (L6) | Alan |
| TD3 | Three flavours: **DJVU** (after Marbles' déjà vu), **CRPT** (after Labyrinth's CORRUPT), **SHFT** (the classic shift register) | Alan |
| TD4 | Notes are quantized to **trig-seedr's own SCALE and ROOT** (on NOTE); a copy on the TURING page only if testing asks for it | Alan |
| TD5 | The names DJVU, CRPT, SHFT | Alan |
| L1 | **One engine, two uses**: offline, a RHYTHM source and a value source that write a whole track of evolving passes (every device); live, the TURING page that keeps rewriting while the pattern plays (each device whose step position is known) | lead |
| L2 | **Offline is deterministic** like every other source: the same recipe, original and context give the same take (I2), `seedref` renders it, golden vectors freeze it, VARY, preview, YES / NO / FUNC + YES unchanged | lead |
| L3 | **A cell** holds a trig bit and a value (0..65535). The loop is LOOP cells long (1..16; SHFT 2..16). Cell values feed NOTE, VEL and the LOCK lanes through the value source TURG, so pitch, dynamics and locks move with the trigs | lead |
| L4 | **One knob, AMT 0..100**, read per flavour: DJVU déjà vu (0 new each pass, 50 locked, 100 shuffled), CRPT corrupt (0 locked; to 50 values change up to 25 %; above 50 bits flip too, up to 50 % at 100), SHFT probability (0 locked inverted, the loop doubles; 50 coin flip; 100 locked). BIAS (DJVU, SHFT's density of new bits) is the chance a new cell's bit is on | lead |
| L5 | **Offline passes**: pass k fills steps k·LOOP .. (k+1)·LOOP − 1 of the track; between passes the flavour's rule mutates the cells with the recipe's seeded random streams. STRT chooses the first pass: SEED (random cells) or TRACK (the original's first LOOP steps: the machine evolves your own pattern) | lead |
| L6 | **Live is a pending take that keeps changing (TD2's fast path)**: YES on TURING starts it (grid recording on, as every generate); every step the playhead has passed mutates that step's cell and rewrites its places on the track behind the playhead; FUNC + YES keeps what is on the track and stops; NO puts the original back and stops; closing seedr, another track or pattern, or RECORD off puts the original back (NOT APPLIED), as for any pending take. AMT can be turned while it runs | lead |
| L7 | **Live keys**: YES start / restart from the current cells; FUNC + YES apply; NO revert; **FUNC + UP** remembers the cells (Labyrinth's BUFFER), **FUNC + DOWN** recalls them | lead |
| L8 | **The step position** is read where each device gives it: the Rytm's core events (`ev_seq_step`), else a playhead address polled from the tick. A device without either has no TURING page (the frame hides pages a device lacks); its offline source still works | lead |
| L9 | **SCALE** (OFF, CHR, MAJ, MIN, DOR, PHR, LYD, MIX, LOC, HMIN, MMIN, PMAJ, PMIN, WHOLE, DIM, plus Labyrinth's set where different: HIRA, 7SUS4, MAJ7, MIN7) and **ROOT** (C..B) on NOTE; they quantize every note value source, not only TURG. OFF is the default, so every existing recipe and golden vector stays the same | lead |
| L10 | **New recipe fields** at the end of `struct seed_recipe` (`tur_flav`, `tur_loop`, `tur_amt`, `tur_bias`, `tur_strt`, `scale`, `root`), defaults that change nothing; `lib/seed.h` changes by the lead's request in this document | lead |
| L11 | SHFT's notes are the register's weighted sum (8 bits, as an 8-bit DAC) through SCALE; its trigs are bit 0 | lead |
| L12 | Version 0.9.0 (TS09) for both devices; the candidate is built for on-device testing before any release | lead |

## 2. The flavours

**DJVU.** Each pass, for each cell: with p = (2·AMT/100 − 1)², if AMT ≤ 50 the cell is replaced by a new one (a
bit with chance BIAS, a uniform value) with chance p; if AMT > 50 the pass plays the loop from a random position
with chance p per step (Marbles' jump), else in order. AMT 50 repeats exactly.

**CRPT.** Each pass, for each cell that is on: its value is redrawn with chance min(AMT, 50) / 2 % (0..25 %) or,
above 50, 25 + (AMT − 50) / 2 % (25..50 %); above 50 every cell's bit flips with chance (AMT − 50) % (0..50 %). A
cell flipped on draws a new value. AMT 0 locks the loop.

**SHFT.** A register of LOOP bits rotates one step per step; the bit that comes round is kept, or inverted with
chance |AMT − 50| / 50 mapped so that 0 inverts always (the loop doubles), 50 is a coin flip and 100 keeps always.
The trig is the bit leaving; the value is the 8 newest bits as a byte.

## 3. Where it lives

- `lib/src_turing.c` (the engine; offline passes and a step function the live page shares), `lib/scale.c` (the
  quantizer), `lib/seed.h` (L10), `tools/seedref.c` (keys `tur_flav=djvu|crpt|shft`, `tur_loop`, `tur_amt`,
  `tur_bias`, `tur_strt`, `scale`, `root`, `vsrc=turg`).
- Pages: RHYTHM's SRC gains TURG with FLAV, LOOP, AMT, BIAS, STRT; NOTE gains SCALE, ROOT; VSRC gains TURG; the
  TURING page (L6, L7) shows the cells, the playhead and the knobs.
- Devices: `sd_playhead()` in each device part (L8).

## 4. Invariants added

| # | Invariant |
|---|---|
| T1 | Offline TURING obeys I1–I6 and I10 as every source does |
| T2 | AMT at its locking end (DJVU 50, CRPT 0, SHFT 100) gives the same pass every time: the track is LOOP steps repeated |
| T3 | Live: NO and leaving restore the original byte for byte (I7); writes only touch the track in scope, below its length, behind the playhead |
| T4 | SCALE OFF changes nothing (every golden vector the same) |
