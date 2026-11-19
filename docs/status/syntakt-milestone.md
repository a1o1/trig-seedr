# The Syntakt milestone: trig-seedr on Syntakt OS 1.42 in analog-emu

State: the emulator milestone is met; for Alan's review
Branch: worktree-s0-2-emu-st (merges of S0.3, S0.4, S0.5, S1, S2, S3, S3b, S4, S5); nothing is on `main` or pushed
Date: 2026-11-19

Alan asked on 2026-11-18 for the Syntakt work to go ahead without his intervention, with parallel agents, to a
locally tested milestone. The lead took SD1–SD6 provisionally (plan section 2), ran the tasks as agents in their
own worktrees, and reviewed, re-ran and merged each one.

## What works, in analog-emu

trig-seedr opens on the stock Syntakt OS 1.42 as a third TRIG page and works as on the Rytm:

- every page and knob, LOCK's lanes following each track's machine (SD4)
- generate, NO, FUNC + YES and the NOT APPLIED rules
- the refusals on the FX track and in stock's Euclidean mode
- YOLO and ROTATE ALL
- the project's modified mark

Every take is compared field by field with `tools/seedref --device st`, the same engine built for the computer. The
sequencer plays the trigs seedr writes, and a kept take survives a pattern save and reload and a project load after
a power cycle.

## Tests (the lead's re-runs on the merged branch)

| Suite | Result |
|---|---|
| `sh tools/st_run_tests.sh` (`tools/tests-st/`) | 8 files, 102 checks, 0 failed (about 29 minutes): 01-generate 8, 03-save 6, 13-apply 21, 14-fields 18, 15-play 4, 41-fuzz-sample 13, 43-yolo 15, 44-refuse 17 |
| `tools/st_s3.sh`, `tools/st_s3b.sh` | 18 and 28 checks, 0 failed |
| `make -C device test` | `test_syntakt_st` 15,244, `test_st_params` 256,668, `test_rytm_ar2` 27,905; 0 failed |
| `make -C lib test`, the frame tests | pass on the Rytm and the Syntakt profiles (interplay: 557,062 Rytm, 560,288 Syntakt) |
| Rytm golden vectors | 129 the same |
| Full Rytm suite (`sh tools/run_tests.sh`, on e6291d3 in a separate checkout) | 30 files, 476 checks, 0 failed (84 minutes); golden vectors 129 the same. S1's split does not change the Rytm. (`testing.md` says 477 for 0.7.1: its 43-yolo count, 29, differs from the 26 the file runs; S1.md) |

## Sizes

The Syntakt build uses our minimal core (440 bytes of code), not Synfact's `core-st`, which does not link with gcc
16.2 (SD1, revised). seedr is built with `-Os`. In the 64 KiB area at `0x46000000`: code 53,788 bytes, data 10,420,
**836 bytes spare**; about 163 KB of buffers from the OS heap (SD2).

## Not ready for a unit

These stop a build from going on a Syntakt as it is:

1. **The memory area.** Stock's sample buffer reaches `0x46000000` past about 84 s of loaded samples (Synfact's
   finding). A unit build needs an area proven unused.
2. **The file.** elekloader's packer makes the container 144,912 bytes larger than stock's, which the Syntakt
   profile refuses; `tools/st_build.sh` lifts that limit for the emulator only. A unit build needs the repack
   Synfact uses (`elektron-firmware-tool`, not installed).
3. **analog-emu** accepts a patched Syntakt file only on the local branch `syntakt-modified` (in
   `../analog-emu-st`, not pushed). It belongs upstream.

## For Alan

- **SD1–SD6**: confirmed by Alan on 2026-11-19 (plan section 2), SD2 as U1's area.
- **Rule 1:** `device/st_params.h` is generated from the stock file's parameter table (names, ids, ranges). Is it
  committable and publishable?
- **On the unit, in order (plan S6, `docs/flashing-st.md`):** S6.1 the way back on stock; S6.2 stock round-tripped
  by elektron-firmware-tool; S6.3 the memory canary with all 64 sample slots loaded; S6.4 trig-seedr and the 15
  first checks, power off with a take kept (I9) and pending (A7) among them. The files are built:
  `sh tools/st_canary.sh`, `sh tools/build_release_st.sh`.
- **Room:** solved by U1's area (983,876 bytes spare in its 1 MiB).

## Open points

- Fixed (P1): seedr steps aside for stock's popups and whole-screen menus (the view controller's classes, as on the
  Rytm); `tools/st_s3.sh` checks TEMPO, PTN, TRK and FUNC + TEMPO, and that FUNC held keeps seedr's screen.
- Retrig is not written on the Syntakt (it has none on its trig page; the bytes travel with the step).
- 15-play checks one track's onsets; velocities and micro timing are not checked by level or ear.
- Done (P1): 41's fuzz sample is the Syntakt's (`test_fuzz --sample-st`): lanes over each machine's whole list, on
  tracks 1, 12, 4 and 9.
- The groove tables still live in `mods/seedr-ar2/`.
- Done (P1): `test_pages` has a Syntakt build (`test_pages_st`).
- S5 (the manual and the site) is done: the Syntakt is "in development, emulator only" on every page. U1–U3 (a safe memory area, the power-off model, the unit file's repack) are done (status/U1.md … U3.md); S6's steps
  on a unit wait for Alan (docs/flashing-st.md).
- Fixed: PLACE's FLD offered RTRG and ACC on the Syntakt, where they do nothing (found by S5). A choice now names the fields it needs (`frame/pages.c`); `test_pages` checks it.

Each task's details: `docs/status/S0.2.md` … `S4.md`, `U1.md` … `U3.md`, `P1.md`.
