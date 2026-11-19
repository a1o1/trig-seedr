# trig-seedr: requirements (version 1)

What version 1 must do, as statements a test can check, for the Analog Rytm MKII with OS 1.74. Each requirement
names where it comes from in [`design-v1.md`](design-v1.md) (decisions **V**, engine principles **P**, invariants
**I**, assumptions **A**, sections **§**) or the plan's task that settled it ([`plan-v1.md`](plan-v1.md)), and the tests
that check it. The user's description of each behaviour is the manual
([a1o1.github.io/trig-seedr](https://a1o1.github.io/trig-seedr/manual/concepts/)). State: release 0.7.0.

This page replaces 0.3's requirements (D1–D21: the GENERATOR, ROTATE and GROOVE pages), which are in the history
(`git show v0.3.0:docs/requirements.md`); [how they map](#from-03s-requirements) is at the end.

## How to read the tests column

| Written | Is |
|---|---|
| `test_xxx` | a host test, `lib/test_xxx.c` (`make -C lib test`); `test_rytm_ar2` is `device/`'s, `test_pages` and `test_interplay` are `frame/`'s (also run on a Syntakt-shaped profile, `_st`) |
| `NN` | an emulator test file, `tools/tests/NN-*.sh` (`sh tools/run_tests.sh NN`), listed in [`tools/README.md`](../tools/README.md) |
| golden | the frozen vectors, `lib/golden/` (`sh tools/golden.sh`) |
| fuzz | `lib/test_fuzz.c`, and its long runs (`tools/fuzz.sh`) |
| hw *n* | check *n* of [`flashing.md`](flashing.md)'s first checks, by hand on a unit |
| — | no automated test; the reason is given |

What each level covers and what is not tested: [`testing.md`](testing.md).

## G. General

| # | Requirement | From | Tests |
|---|---|---|---|
| G1 | Runs on the Analog Rytm MKII with OS 1.74, as an elekloader mod in a patched stock OS; the boot screen shows the version (TS07 for 0.7) | V8, plan §1 | every emulator test; hw 1 |
| G2 | The engine (`lib/`) is device-neutral C99, freestanding, integer only, with no C library; it builds for ColdFire with no undefined symbols | P6 | 00 (`make -C lib coldfire`) |
| G3 | Everything written is ordinary pattern data: it plays, saves, reloads and survives a power cycle on the stock OS | I9 | 03, 11, 41; hw 7 |
| G4 | Only the tracks in scope change, and only below the track's length | I6 | `test_props`, fuzz, `test_rytm_ar2`, 13 |
| G5 | The same recipe, original and context give the same take | I2 | every emulator take against `seedref`; golden |
| G6 | The repository holds no Elektron firmware or anything built from it, and nothing derived from Ableton's content | plan §2 | — (`.gitignore`; review at merge) |
| G7 | The mod fits the loader's limits; a render on a 64-step track stays in milliseconds | plan M2.3 | 42 |

## K. Keys and screen

| # | Requirement | From | Tests |
|---|---|---|---|
| K1 | Opens as a third TRIG page (TRIG on EUCLIDEAN) and with TRIG held + PAGE; closes with TRIG, a parameter page or menu key, or NO with no take pending | §8 | 07, 10; hw 3 |
| K2 | LEFT / RIGHT step through the pages in the order TAKE, RHYTHM, PLACE, DYNAMICS, NOTE, LOCK, TRIG, TIME, VIEW, TOOLS; PAGE, the TRIG keys and the transport stay stock's | V10, §5 | 10; hw 8 |
| K3 | UP / DOWN pick the lock lane on LOCK | §5, §8 | `test_pages`, 26 |
| K4 | The title bar shows the page, the track, the page's place, and the source: TRK, EUC, CPY, MAP (CHAOS below 4) or S and the seed | V6, §8 | 10, 23 |
| K5 | A dot before the source shows while a take is pending | M2.5 | 13 (pending state); hw 13 |
| K6 | A range knob sets the high end, FUNC + knob the low end; each end stops at the other; VEL, LEN and NOTE have DEF below their lowest value | §5, M2.5 | `test_pages`; hw 14 |
| K7 | A knob that cannot change the take is hidden and keeps its value (PHRS on 16 steps or fewer; CVAR, QUAL unless the trig positions come from the seed; ROLE at BIAS 0; AMOD at ACC 0; CTYP, CVAL at COND 0; RATE, RCUR at RTRG 0; TIME, GVEL, SHFT with no groove); a hidden value, turned, leaves the take byte-identical | M2.1, 0.7 | `test_pages`, `test_interplay`, 40 |
| K8 | FUNC + knob turns SEED in steps of 100 and other knobs with more than 20 values in steps of 10 | §8 | `test_pages` |
| K9 | A message replaces the strip for 2 seconds and says why when nothing is written | §8 | 06, 13 |

## A. Generate and apply

| # | Requirement | From | Tests |
|---|---|---|---|
| A1 | YES generates only with grid recording on; otherwise it writes nothing and says RECORD TO GENERATE | §9 | 13; hw 4 |
| A2 | The first YES snapshots the tracks in scope; every YES renders the whole recipe from that original | V2, §9 | 01, 13 |
| A3 | YES with no knob turned takes the next seed while a take is pending or just after FUNC + YES; after NO or NOT APPLIED it brings the same take back | V2, M2.5 | 13, 26; hw 5, 13 |
| A4 | NO restores the original byte for byte, lock store included | I7 | 01, 13, 11, `test_rytm_ar2` |
| A5 | FUNC + YES applies the take and drops the snapshot; it is the only way a take stays | §9, A4 | 01, 13 |
| A6 | Changing track or pattern, closing trig-seedr or turning grid recording off with a take pending restores the original (NOT APPLIED) | §9 | 13; hw 6 |
| A7 | A pattern save during a pending take stores the take as it stands | §9 | — (stock's save; not checked separately) |
| A8 | A take that cannot be written (a lock with no row left, a refused recipe) writes nothing and says why | I8 | 06, 12, 41, `test_rytm_ar2` |
| A9 | YES writes nothing on the FX track (NOT ON THE FX TRACK) or on a track in stock's Euclidean mode (EUCLID IS ON: TURN IT OFF) | §10 | — (no emulator test yet) |

## E. Engine

| # | Requirement | From | Tests |
|---|---|---|---|
| E1 | One recipe per track; a new one starts on SRC = TRACK when the track has trigs and on RAND when it is empty; METR and BEAT come from the last recipe generated | V5, V6, A7 | 01, `test_seed_render` |
| E2 | The chain runs RHYTHM, PLACE (ROT, SYNC, MASK, SKIP), DYNAMICS, NOTE, LOCK, TRIG, TIME, then FLD; changing one layer changes only the fields it owns | §4, P1, I3 | `test_props`, fuzz |
| E3 | A layer at 0 does not touch its fields: SRC = TRACK with every layer at 0 leaves the track byte-identical | V3, V4, I1 | `test_props`, 00, 10 |
| E4 | Share knobs (DENS, SYNC, ACC, COND, RTRG, a lane's %) keep every step they had chosen when raised | P2, I4 | `test_seed_rng`, `test_props` |
| E5 | DENS gives its exact count; MASK and SKIP only remove | I5 | `test_src_random`, `test_place_mask`, `test_interplay` |
| E6 | VARY: 0 gives independent takes, 100 identical ones, in between the share kept | P3, I10 | `test_seed_rng`, `test_vary`, fuzz |
| E7 | SKIP, MASK and the velocity shapes refer to the steps' final positions | P5 | `test_place`, `test_dyn` |
| E8 | A seed gives the same take from release to release; a change is written down with its reason | V11 | golden |
| E9 | Recipes live in RAM, per pattern in play; power off forgets them | A3 | — (by design) |

## Pages

### TAKE

| # | Requirement | From | Tests |
|---|---|---|---|
| TK1 | SEED 0–9999 names the take | §5 | every `seedref` comparison |
| TK2 | PHRS (AAAA, AAAB, ABAB, ABAC, ABCD) plays take SEED + the letter's offset on each 16-step page; SKIP, the groove, MASK, COPY and EUCLID keep track positions; a short last page is a phrase of its own length for TRACK, COPY and EUCLID | §7, A2, F9, M2.1 | `test_vary`, 29 |
| TK3 | CVAR writes takes SEED and SEED + 1 as complementary conditions (1:2 / 2:2; AAAB as !PRE / 4:4) | §7, F9 | `test_vary`, 29; hw 10 |
| TK4 | METR (4/4, 3/4, 6/8, 12/16, 5/8, 7/8) and BEAT rank the steps by Barlow's indispensability | F1 | `test_meter`, 21 |
| TK5 | QUAL scores 1–8 candidates on where the trigs are; a take is still a function of its seed | F11 | `test_score`, 31 |

### RHYTHM

| # | Requirement | From | Tests |
|---|---|---|---|
| RH1 | SRC = TRACK uses the trigs on the track, with all they carry | V3 | 00, 10 |
| RH2 | RAND: GRID, DENS (exact count), BIAS towards the rank, ROLE (METER, KICK, BACK, HAT, PERC) | §5, F1 | `test_src_random`, `test_meter`, 21 |
| RH3 | EUCLID gives the pattern stock's Euclidean mode plays for LEN = STEPS, PL1 = PULS | F2 | `test_src_euclid`, 22 |
| RH4 | MAP: the map's levels blended at X / Y, DENS threshold, CHAOS as a seed-drawn lift; PART and DENS per track, X, Y, CHAOS shared; levels as the original code gives them | §6, A1, A6, F3 | `test_src_map`, 23 |
| RH5 | COPY takes another track's trig positions as it plays (a Euclidean-mode track as that mode plays) | F4 | `test_src_copy`, 24 |

### PLACE

| # | Requirement | From | Tests |
|---|---|---|---|
| PL1 | ROT moves whole steps with all their parameter locks; swing stays with the position | §4 | `test_place`, 05, 11 |
| PL2 | SYNC anticipates trigs onto a weaker step; the count is kept, runs resolve onto a stronger step, SKIP is respected; on TRACK the step moves with its locks | F5 | `test_place_sync`, 25 |
| PL3 | MASK AVOID / ONLY against another track, on final positions; it only removes | F4, P5 | `test_place_mask`, 24 |
| PL4 | SKIP clears the first N steps | §5 | `test_place`, 08 |
| PL5 | FLD / FROT / FMOD rotate one field against the trigs, by trig or by grid | §5 | `test_place`, 05 |

### DYNAMICS

| # | Requirement | From | Tests |
|---|---|---|---|
| DY1 | VEL (DEF or a range), SHAPE with AMT (FLAT, STRONG, WEAK, OFF, NOT 1, NOTBAR, EVRY), HUM | §5 | `test_dyn`, 08 |
| DY2 | SHAPE = RANK and AMOD = TOP follow the meter's rank | F1 | `test_dyn_rank`, 21 |
| DY3 | SHAPE = NOISE: a contour with N as its zoom | F7 | `test_noise`, 27 |
| DY4 | SHAPE = MAP sets velocity from the map's level; with SRC = MAP, AMOD = TOP ranks by the map key, so ACC reproduces the original's accents | §6, F3, F12 | `test_dyn_rank`, 23, 32 |
| DY5 | ACC above 0 owns the accent flag of every trig; at 0 the flags stay | V4 | `test_dyn` |

### NOTE and LOCK

| # | Requirement | From | Tests |
|---|---|---|---|
| NT1 | LEN and NOTE: DEF or a range | §5 | `test_values` |
| NT2 | LIST: NLEN values from the range, sorted, played in an order (UP, DOWN, PING, PNG2, ODD, RAND) | V9, F6 | `test_values_list`, 26 |
| NT3 | NOISE: a smooth contour every ZOOM steps that loops without a jump; values belong to steps, not trigs | V9, F7 | `test_noise`, 27 |
| NT4 | LMOD = LEGATO: each trig lasts until the next | F6 | `test_values_list`, 26 |
| LK1 | Two lanes, each a parameter lock on a chosen parameter with RANGE, LIST or NOISE values; a lane's % above 0 owns that parameter's locks | §5, V8, V9 | `test_values`, 04, 12, 26, 27 |
| LK2 | A lane's PARM skips the other lane's parameter; a recipe with both lanes on one parameter is refused (SETTING OUT OF RANGE) | M2.2 | `test_pages`, fuzz (`test_found_lanes`), 41; hw 12 |
| LK3 | A lock the pattern's 72 lock rows cannot hold is refused before anything is written (NO LOCK ROWS LEFT) | I8, task D1 | 06, 12, `test_rytm_ar2` |

### TRIG and TIME

| # | Requirement | From | Tests |
|---|---|---|---|
| TG1 | COND above 0 owns the conditions: the share chosen get X%, A:B, 1ST, FILL or a mix, the others none | §5 | `test_trig` |
| TG2 | RTRG above 0 owns the retrig switch: the share chosen get a rate from RATE and the curve RCUR | §5 | `test_trig` |
| TM1 | A groove's micro timing (TIME) and velocity (GVEL), QUANT, RAND and SHFT, inside the device's micro-timing range | §5 | `test_timing`, 02 |
| TM2 | DRFT: correlated drift that belongs to the steps and loops without a jump | F8 | `test_timing_drift`, 28 |

### VIEW, the preview, TOOLS

| # | Requirement | From | Tests |
|---|---|---|---|
| VW1 | Every page but VIEW previews the take YES would write, from the recipe and the original, before YES; it follows other tracks COPY and MASK read | §8, F10 | 30 |
| VW2 | VIEW shows the track's steps and the fields of the step under its cursor, read only | §8, F10 | 30, 10 |
| TL1 | TOOLS ROT ALL rotates every track (Euclidean-mode tracks skipped) with its own YES, under generate and apply | V7 | 05, 13 |
| TL2 | TOOLS YOLO rolls a recipe for the track in scope from musical ranges, keeps the meter, the lanes' parameters and the shared MAP settings; NO restores the track and the recipe | V7, 0.7 | `test_yolo`, 43; hw 15 |

## Not in version 1

From design §12: the Analog Four MKII and Syntakt ports; note pools (chords, scales) and chord sequencing; more than
two lock lanes and LFO-shaped lock curves; recipes saved with the project. After 1.0: a TURING page (plan §8).

## From 0.3's requirements

| 0.3 | Now |
|---|---|
| D1, D3, D16 (GENERATOR, its conditions, retrigs and sample locks) | RHYTHM RAND (RH2), TRIG (TG1, TG2), LOCK lanes (LK1) |
| D7 (ROTATE) | PLACE ROT and FLD (PL1, PL5), TOOLS ROT ALL (TL1) |
| D8, D12–D14 (grooves) | TIME (TM1); `tools/groove2h.py` unchanged |
| D11, D15 (audition, YES regenerates) | generate and apply (A2–A5) |
| D17 (the third TRIG page) | K1 |
| D18 (leaving keeps the take) | changed at 0.5.1: leaving restores the original (A6) |
| D19–D21 (metric weights, SKIP, velocity shapes) | METR and BEAT (TK4), SKIP (PL4), DYNAMICS (DY1, DY2) |
| D2, D4 (the track's length, mono) | unchanged: a take covers the track's length (G4); no chords (not in version 1) |
| D5, D6, D9 (dropped or deferred) | EVERY N returned as SHAPE = EVRY (DY1); SYMMETRY and SHUFFLE are not in version 1 |
| D10 (device order) | design V8, §12 |
