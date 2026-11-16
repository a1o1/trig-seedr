# How trig-seedr is tested

trig-seedr is installed by flashing a modified operating system onto an Analog Rytm MKII. This page lists what is
tested before a release, how, and what is not tested. Numbers are from version 0.7.0 (2026-11-16).
How to run each check is in [`tools/README.md`](../tools/README.md) and [`dev-environment.md`](dev-environment.md).

## What a take must never do

The design ([`design-v1.md`](design-v1.md), section 11) states ten invariants. Every test below checks one or more of
them.

| # | Invariant |
|---|---|
| I1 | SOURCE = TRACK with every layer at 0 leaves the track byte-identical, parameter locks included |
| I2 | the same recipe, original and context give the same result |
| I3 | changing one layer's settings changes only the fields that layer owns |
| I4 | raising a share knob keeps every step it had already chosen |
| I5 | DENS gives its exact count; MASK and SKIP only remove |
| I6 | only the track in scope changes, and only below its length |
| I7 | NO, and leaving with a take pending, restore the track byte for byte; FUNC + YES keeps the take |
| I8 | a lock that cannot be stored is refused before anything is written |
| I9 | the result plays, saves, reloads and survives a power cycle on the stock OS |
| I10 | VARY 0 gives independent takes, 100 identical ones, and in between the share it says |

## The levels

| Level | What | Size | Invariants |
|---|---|---|---|
| Host unit tests | `make -C lib test`, `make -C device test`, the page tables: every module of the engine and the device layer, with AddressSanitizer and UndefinedBehaviorSanitizer | about 636,000 checks in all (the interplay and fuzz tests below included), a few minutes | each module's own |
| Property tests | `lib/props.h`: random tracks and recipes checked against the invariants inside each module's tests | thousands of random cases per module | I1–I6 |
| Fuzzing | `lib/test_fuzz.c`: random originals, patterns and recipes over every source, layer and value source, on three device profiles, 6 % with knobs set outside their ranges | 4,000 cases in every `make test`; long runs with `tools/fuzz.sh` (below) | I1–I6, I10, value ranges |
| Device-layer fuzzing | `device/test_rytm_ar2.c`: random patterns, lengths, lane parameters, row limits and takes written through the device layer into a model of stock's memory; checks that a refused write changes nothing, other tracks and steps past the length are untouched, each trig carries its fields, lanes and origin's locks, the 72-row limit holds, and the restore is byte-identical | 3,000 cases in every `make -C device test` (`DEV_FUZZ` for more; 200,000 found nothing) | I6–I8 |
| YOLO | `lib/test_yolo.c`: every roll at WILD 0–100 on five track roles and five lengths renders without a refusal, repeats exactly, keeps the meter and the lock lanes' parameters and stays in its musical ranges; `tools/tests/43-yolo.sh` compares the emulator's rolled recipes and takes with `seedref yolo=ROLL:WILD` | 31,000 checks per `make test`; 29 emulator checks | I2, I6, I7 |
| Reference comparison | `tools/seedref`, the same engine built for the computer, renders every take the emulator writes; the two must agree field by field | every emulator check that generates | I2 |
| Golden vectors | 129 recipes over every layer with their frozen results (`lib/golden/`); a change to any result fails until its reason is written down | every run | I2, design V11 |
| Interplay | `mods/seedr-ar2/test_interplay.c`: recipes the pages can reach, from every source on 13–64 steps; every hidden knob, turned, must leave the take byte-identical, and each source must keep the trig counts it promises through ROT and SYNC | about 533,000 checks in every `make test`; a 60,000-recipe run made 10 million | I3, I5, I6 |
| Emulator | `tools/run_tests.sh` boots the patched OS in [analog-emu](https://github.com/trigcondition/analog-emu) and drives it through the Rytm's own keys and knobs: generate, NO, apply, save, reload, power cycle, the lock-row limit, every page | 29 test files, 449 checks; the full run takes about 75 minutes on the Mac (42-perf alone 25) | I1, I2, I7–I9 |
| Emulator fuzz sample | `tools/tests/41-fuzz-sample.sh`: recipes from the fuzzer replayed on the emulated Rytm | 6 recipes per run (more with `FUZZ_SAMPLES`) | I2, I7–I9 |
| Hardware | `docs/flashing.md`'s checks on a Rytm MKII with OS 1.74; every pass is listed there (Tested on hardware) | one unit (0.5.1, 0.6.0) | I7, I9 |

Each test level catches a different kind of mistake. The host tests and the fuzzer test the engine's logic on millions
of cases in minutes. The emulator runs the real stock firmware with the mod in it, so it tests what the engine cannot
know: how stock stores steps and parameter locks, the 72-row lock limit, saving to the card. The hardware pass
checks what the emulator cannot: the real unit's timing, sound and card.

## Fuzzing

A fuzzer generates random inputs in very large numbers and checks that every result obeys the rules. Here an input is
a random track (1 to 64 steps, with trigs, notes, velocities, lengths, micro timing, conditions, retrigs, parameter
locks and flag bits), a random pattern around it, and a random recipe that can use every source, layer, shape, value
source, order and mode trig-seedr has. Each case is checked against I1–I6 and I10, and every value in the result
must be one the Rytm can store (micro timing inside its range, locks inside each parameter's range, steps that come
from a real step of the track). The fuzzer runs with AddressSanitizer and UndefinedBehaviorSanitizer, which stop it
on any out-of-bounds memory access or undefined C behaviour.

| Run | Cases | Failures |
|---|---|---|
| every `make -C lib test` | 4,000 | 0 |
| long run, 4 jobs for 62 minutes (`sh tools/fuzz.sh 3720 4`) | 18,660,356 | 0 |

Any failure prints a case number; `FUZZ_CASE=<number> lib/test_fuzz` replays it and prints the recipe, the
original and the take.

### What the fuzzer and the interplay test found

The fuzzer and the interplay test found faults that the earlier tests did not:

- **Two lock lanes on one parameter.** LOCK has two lanes, and each can be set to any lockable parameter. With
  lane 1 at 100 % on the sample slot and lane 2 left on the sample slot too, the Rytm stored lane 2's value (the
  original's locks) and lane 1's new locks were lost. On the emulated Rytm the take had no sample locks where the
  reference showed one on every trig. Such a recipe is now refused with SETTING OUT OF RANGE and nothing is written.
  A host test keeps it fixed (`test_found_lanes` in `lib/test_fuzz.c`). LOCK's PARM knob now also skips the
  parameter the other lane is on, so the refusal is only a backstop. Details: [`status/M2.2.md`](status/M2.2.md).
- **PHRS cut EUCLID into pages.** On 64 steps, EUCLID 13/5 under PHRS played four copies of its first 16 steps.
  EUCLID now runs along the whole track and the phrase letters only dress its trigs.
- **PHRS lost trigs on a short last page.** On a 24- or 40-step track, ROT and SYNC pushed real trigs past the
  track's end. A short last page is now a phrase of its own length for TRACK, COPY and EUCLID.
- **A:B's CVAL showed `/10`** where the engine uses 8.

Details of these three: [`status/M2.1.md`](status/M2.1.md).

## What is not tested

- **Sound.** No test listens: the emulator's audio is not compared, and the hardware pass is by ear.
- **Other units and OS versions.** The mod is built for OS 1.74 only, and the hardware pass covers one unit.
- **The emulator is not the hardware.** It runs Elektron's own firmware, but its timing and peripherals are models.
- **Very long sessions.** No test runs the mod for hours on the device.

Modified firmware is used at your own risk; see [`flashing.md`](flashing.md) for how to go back to the stock OS.
