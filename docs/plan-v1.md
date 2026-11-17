# trig-seedr: plan for version 1

For agents working in parallel. Read `design-v1.md` first: it is the specification; this file is how the work is cut up. `requirements.md` states version 1's requirements, each traced to the design and its tests; `firmware-notes-os174.md` describes the stock OS.

## 1. Shape of the work

| Milestone | What | Agents | Result |
|---|---|---|---|
| **M0 Foundation** | new engine structure, page framework, device contract, test harness; 0.3's features ported onto it | 1 lead, then 4 in parallel | release 0.4: same features, new structure, SOURCE = TRACK, LEFT / RIGHT |
| **M1 Features** | eleven workstreams, each a layer or source | up to 8 in parallel | releases 0.5 … 0.9 as workstreams land |
| **M2 Integration** | interplay tests, fuzzing, performance, docs, site, hardware checklist | 2–3 | release 1.0 |

The rule that makes parallel work possible: **M0 creates every enum value, recipe field, page table entry and function stub that version 1 needs.** An M1 workstream then fills bodies in files it owns and flips its `SEED_HAVE_…` flag; it does not edit the contract, the framework or another workstream's files.

## 2. Hard rules

1. **Never commit** Elektron firmware or anything built from it (`.syx`, snapshots, card images, unpacked sections), nor Ableton-derived data (`grooves-live.h`). `build/` and `out/` stay untracked.
2. **Never put Alan's email address** in commits, files or requests to outside services.
3. Do not look for tools that decrypt Ableton's protected `.agr` files.
4. No "Mutable Instruments" and no "Grids" in product names, pages or the mod's metadata; credit in the acknowledgements only. Map data keeps its GPL-3.0-or-later header.
5. Conventional Commits (`CONTRIBUTING.md`); one task per branch (Claude Code names a worktree's branch `worktree-<name>`; use the task id as the name, e.g. `f5-sync`); nothing is pushed to `main` except by the lead after the gates in section 7.
6. Get a missing tool working before reaching for a workaround. Ask Alan one question at a time.
7. Factual wording in docs: no weasel words, no hyperbole.

## 3. Environment

Full description, per operating system, with first-run checks: **`dev-environment.md`**. In short:

Workspace `~/dev/github/elektron` on the Mac, where development, tests and releases are (`dev-environment.md`):

| Folder | Use |
|---|---|
| `trig-seedr/` | this repository |
| `elekloader/` | `python3 -m elekloader.sdk.build`, `elekloader.patch`; core-ar2 0.13 |
| `analog-emu/` | `dtboot`, `dtpeek`, `dtcard` (headless), `rytm` (window, Mac) |
| `Official Firmwares/` | the stock OS 1.74 `.syx` (local only) |
| `mi/eurorack/grids/` | Grids' source and map data |
| `.toolchains/` | the Linux toolchain 0.3 was built with (history; does not run on macOS) |

- On the Mac: Homebrew's `m68k-elf-gcc` (`ELEKLOADER_CROSS=m68k-elf-`) and analog-emu's Mac build in `analog-emu/target/release`.
- `make -C lib test` (host, ASan / UBSan), `make -C lib coldfire` (no undefined symbols), `sh tools/run_tests.sh` (every check: `00-host` runs the `make` targets of `lib/`, `device/` and the page tables, then the emulator files; `NAME...` for some).
- One git worktree per task. Claude Code creates them under `.claude/worktrees/<name>/` (git-ignored) on a branch of their own. Inside a worktree `export WORKSPACE=~/dev/github/elektron` (the test scripts otherwise look for `elekloader/` beside the checkout). `.worktreeinclude` copies the local-only `grooves-live.h` into each worktree; it stays untracked.
- New worktrees branch from the default branch **on the remote**. A task's prerequisites must be merged and pushed to `main` before its agent starts.
- Since task 0.5 each checkout's suite writes only under its own `build/emu/` (core-ar2 included), so suites in two worktrees run side by side. Release 0.3's `tools/test_seedr.sh`, which wrote into `analog-emu/` and elekloader's `out/`, was removed at G0.

## 4. Files and owners

| Path | Content | Owner |
|---|---|---|
| `lib/seed.h` | the contract: step, track, recipe, capabilities, enums, stage prototypes | lead (frozen after 0.1; changes by request) |
| `lib/seed_rng.c` | streams, slots, keys, VARY (P1–P3) | lead |
| `lib/seed_render.c` | the chain | lead |
| `lib/meter.c` | rank tables, meters, roles | F1 |
| `lib/src_random.c`, `src_euclid.c`, `src_map.c` + `map_data.c`, `src_copy.c` | sources | 0.2, F2, F3, F4 |
| `lib/place.c`, `place_sync.c`, `place_mask.c` | ROT, SKIP, field rotation; SYNC; MASK | 0.2, F5, F4 |
| `lib/dyn.c`, `dyn_rank.c`, `dyn_noise.c` | range, shapes, accent; RANK / TOP; NOISE | 0.2, F1, F7 |
| `lib/values.c`, `values_list.c`, `noise.c` | range; lists and orders; value noise | 0.2, F6, F7 |
| `lib/trig.c`, `timing.c`, `timing_drift.c` | conditions, retrigs; random and groove timing; drift | 0.2, 0.2, F8 |
| `lib/groove.h` | a shim: `gen_groove` = `seed_groove`, for groove tables made before 0.4 | lead |
| `lib/vary.c`, `score.c` | PHRS, CVAR; candidate scoring | F9, F11 |
| `lib/test_<module>.c`, `test_props.c`, `golden/` | host tests, invariants, frozen vectors | each owner; 0.5 for the framework |
| `lib/props.h`, `lib/tests.h` | the property-test framework (random tracks and recipes, I1–I3, I6); host-test helpers | lead (from 0.5, 0.2) |
| `device/rytm_ar2.h`, `rytm_params.h` | device layer; the lockable parameters and track fields as data (shared with `tools/seedref.c`) | 0.4, then the lead |
| `device/Makefile`, `test_rytm_ar2.c`, `cf_rytm_ar2.c` | the device layer's host test and ColdFire check | 0.4, then the lead |
| `mods/seedr-ar2/seedr.c` | keys, navigation, audition, recipe store, drawing | 0.3, then the lead |
| `mods/seedr-ar2/pages.h`, `test_pages.c`, `Makefile` | the page-table types and their host test | 0.3, then the lead |
| `mods/seedr-ar2/mod.json` | the mod's sources and metadata | lead; a workstream adding a `lib/` file adds its line to `sources` and says so in its status file |
| `mods/seedr-ar2/pages.c` | every page's knob table | 0.3 creates all; owners edit only their page's rows |
| `mods/seedr-ar2/preview.c`, `preview.h`, `view.c` | preview strip; VIEW | F10 |
| `tools/seedref.c`, `tools/emu.sh`, `emu.py`, `golden.sh`, `doctor.sh`, `run_tests.sh`, `tests/fixtures.sh` | reference renderer, emulator helpers, golden vectors, environment check | 0.5, then the lead |
| `tools/tests/NN-<name>.sh` | one test file per task: `00`..`11` are M0's; F1..F11 use `21`..`31` (F1 `21-rank.sh`, F5 `25-sync.sh`, …) | each owner adds its file |
| `docs/screens/` | screenshots: the top level is what the README and the site show; `docs/screens/<id>/` a task's own | each owner |
| `docs/firmware-notes-os174.md`, `docs/dev-environment.md`, `docs/release-<version>.md` | firmware findings, the environment, release notes | lead (findings by request in a status file) |
| `site/src/content/docs/manual/<page>.md` | the manual: user-facing text, one file per page in on-screen order, plus Concepts and Keys and messages (until 0.7.1: `docs/v1/<layer>.md`, one per feature) | each owner edits its page's sections; M2.6, then the lead |
| `docs/status/<id>.md` | state, decisions taken, how to test, open points | each owner |

`lib/gen.c`, `lib/gen.h`, `lib/groove.c`, `tools/genref.c` and `tools/test_seedr.sh` were retired at gate G0 (task 0.6).

## 5. M0 Foundation

| Task | Owner | Content | Done when |
|---|---|---|---|
| **0.1 Contract** | lead, alone, first | `lib/seed.h` with every version-1 field, enum and prototype (stubs return "not available"); `seed_rng.c` with tests for I2, I4, I10; `seed_render.c` skeleton; `docs/status/` and branch layout | header reviewed against `design-v1.md` sections 3–7 and 10; host tests pass |
| **0.2 Engine port** | A | 0.3's features as stages: RANDOM source (GRID, DENS, BIAS on today's five-level weights), ROT, SKIP, field rotation, VEL range and METER's shapes, LEN, NOTE, sample lock as lane 1, COND, RTRG, random and groove micro timing; SOURCE = TRACK; layer-at-0 rule | `test_props.c` proves I1–I6 for these layers; ColdFire build clean |
| **0.3 Page framework** | B | table-driven pages; per-track recipe store; LEFT / RIGHT; PAGE and TRIG keys passed to stock (verify in the emulator, report if stock misbehaves); title bar; preview strip placeholder; pages hidden when their `SEED_HAVE_…` flag or a capability is off; VIEW cursor on a knob; TOOLS page with ROTATE ALL | every 0.3 function reachable; screenshots of each page |
| **0.4 Device layer** | C | capabilities; read any track; write with origins (single-step moves carrying all 44 lock values through stock's lock set / clear); lock lanes on a chosen parameter with names; room check per lane | emulator test: a hand-made step with three parameter locks moved by one step, locks intact after save and reload; I7–I9 |
| **0.5 Test harness** | D | `tools/seedref` (recipe as `key=value` → step dump); `tools/emu.sh` (build, boot, open, go to a page by name, set a knob to a value, YES, compare with `seedref`); recipe injection for tests (`dtboot --poke` into the recipe store, or a test build with `-D` defaults as the lock-row test does); snapshots and cards under each worktree's `build/`; a lock around the core-ar2 build; `tools/run_tests.sh` running `tools/tests/*.sh`; golden-vector generator; `tools/doctor.sh` checking the environment | 0.3's parts 1–8 re-expressed and passing on the new structure; two suites run side by side without interfering |
| **0.6 Integration** | lead | merge A–D; docs and site for the new navigation and SOURCE = TRACK; `build_release.sh` for 0.4 | gate G0 |

0.2–0.5 run in parallel once 0.1 is merged. B and C work against stub renders until A lands.

**Gate G0** (all required): host tests and I1–I10 pass; every 0.3 emulator check has a passing equivalent; SOURCE = TRACK identity shown in the emulator on the step record and the lock store; no undefined symbols; mod size and `.bss` reported and inside the loader's limits; golden vectors frozen; Alan has tried 0.4 in the emulator window.

## 6. M1 Feature workstreams

Each workstream delivers: the module with host tests, its rows in `pages.c`, its `tools/tests/` file, its section of the manual (`site/src/content/docs/manual/<page>.md`; until 0.7.1 `docs/v1/<layer>.md`), `docs/status/<id>.md`, and a screenshot.

| Id | Workstream | Content | Needs | Waits for |
|---|---|---|---|---|
| F1 | Rank and meter | Barlow's indispensability (formula or tables; check 6-pulse values 5 0 3 1 4 2 and 5 0 2 4 1 3, and 4/4 = 15 0 8 4 12 2 10 6 14 1 9 5 13 3 11 7); METR and BEAT; ROLE tables; SHAPE = RANK; AMOD = TOP | G0 | A7 |
| F2 | EUCLID source | STEPS, PULS; the same patterns as stock's Euclidean mode for the same settings | G0 | |
| F3 | MAP source | map data from `mi/eurorack/grids/resources.cc` with its licence header; X / Y blend as `ReadDrumMap`; DENS threshold; CHAOS as a P3 slot; SHAPE = MAP; a host test comparing levels with the original code's for a set of X / Y | G0 | A1, A6 |
| F4 | COPY and MASK | COPY source; MASK AVOID / ONLY against another track | G0 | |
| F5 | SYNC | anticipation onto the preceding weaker step; count preserved; SKIP respected; runs resolve onto a strong step; works on SOURCE = TRACK through origins | G0, 0.4 | |
| F6 | Value lists | LIST source with ORDR and its own length for NOTE and both LOCK lanes; LMOD = LEGATO | G0 | |
| F7 | Noise | integer value noise; NOISE source for NOTE and LOCK; SHAPE = NOISE with ZOOM | G0 | |
| F8 | Drift | correlated micro timing (DRFT) | G0 | |
| F9 | Variation | PHRS over pages; CVAR as conditions; interaction with a track shorter than four pages | G0 | A2 |
| F10 | Preview and VIEW | the preview strip from the work buffer on every knob change; VIEW on the new step type; drawing cost measured | G0 | |
| F11 | Candidate scoring | QUAL; must keep P3 (a take stays a function of n) | F1 | design note agreed with the lead first |
| F12 | MAP accents | AMOD = TOP ranks by the map level when SRC = MAP, so ACC reproduces Grids' accents (level above 192 at ACC as Grids sets it), as design section 6 first said; F3's open point. A contract change (TOP needs the level) and `dyn_rank.c`; a host test against Grids' accent rule; MAP's user text (then `docs/v1/map.md`, now the manual's RHYTHM and DYNAMICS pages) and design section 6 updated back | F1, F3 | scheduled by Alan 2026-11-15, after M1's merges |

Suggested release grouping: 0.5 = F1, F2, F10; 0.6 = F4, F5; 0.7 = F6, F7, F8; 0.8 = F3; 0.9 = F9, F11.

## 7. Gates for merging a workstream

1. `make -C lib test` and `make -C lib coldfire` pass; the workstream's host tests cover its rows of the invariants table.
2. `tools/run_tests.sh` passes in full, not only the new file.
3. Golden vectors unchanged, unless the workstream's status file says why and the lead agrees.
4. With the feature's knobs at their defaults, every other test's output is unchanged.
5. Screenshot and the manual's text present (`site/src/content/docs/manual/`); no file outside the workstream's rows in section 4 changed without a note in the status file.

The lead merges in the order workstreams pass; after each merge the others rebase and re-run.

## 8. M2 Integration and 1.0

Started 2026-11-16, after 0.5.1 passed a first test on a Rytm MKII with stock OS 1.74.

| Id | Task | Content | Owner | Files |
|---|---|---|---|---|
| M2.1 | Interplay | every source (TRACK, RAND, EUCLID, MAP, COPY) × PLACE (ROT, SYNC, MASK, SKIP, FLD) × DYNAMICS shape × VARY / PHRS / CVAR / QUAL on 16- and 64-step tracks, in the emulator and on the host. Combinations that make no sense or misbehave are hidden on the page or refused with a message; each decision written down | agent `m2-interplay` | `tools/tests/40-interplay.sh`, host tests, `pages.c` rows' conditions, fixes where a module misbehaves (noted per file) |
| M2.2 | Fuzzing | random recipes on the host against I1–I6 and I10 for hours (ASan / UBSan), drawing from every source and layer (`props.h`); a sample replayed in the emulator against I7–I9; every failure reduced to a host test | agent `m2-fuzz` | `lib/props.h`, `lib/test_fuzz.c` (long runs behind a flag), `tools/fuzz.sh`, `tools/tests/41-fuzz-sample.sh` |
| M2.3 | Performance and size | render and preview cost per source and layer on the emulator's instruction counter; the idle cost of seedr's screen (measured by M2.3: 1.9 % of the CPU at 0.5, mostly stock redrawing under it; 0.25 % after); mod size, `.bss`, lock-row use; fixes that keep every golden vector | agent `m2-perf` | `tools/tests/42-perf.sh`, `seedr.c` (the screen's reads), `preview.c`, F6's list speed-up if measured worth it (`lib/values_list.c`, `lib/test_values_list.c`) |
| M2.4 | Cleanups | the site workflow's actions on Node.js 20; `golden.sh` labels a refused recipe "pending"; small items from M2.1–M2.3 | lead | `.github/`, `tools/golden.sh` |
| M2.5 | Hardware and UX | Alan's UX / CX list from the first hardware test; the choices kept until hardware testing (F1 role orders, F5 runs, F6 list order, F8 drift scale, F9 CVAR AAAB with PRE); the track-change revert with the pads; findings filed as issues. **Closed 2026-11-17** (Alan): the notes became 0.7.0 (range knobs, YES after a revert, the pending dot, YOLO); the parked choices and M2.1's MAP points (MAP under PHRS, SHAPE = MAP's PART, X, Y) stay as built until beta feedback | Alan, then the lead or an agent | as each item needs |
| M2.6 | Manual and site | `docs/v1/*.md` (retired) and the Using pages into one coherent manual (a quick start, a section per page, a knob reference); README as a short front door; screenshots (`tools/screens.sh`); requirements renumbered to the version-1 design; written for beta users (Alan, 2026-11-17: 0.7 goes to customers as a beta, its feedback leads to 1.0): what beta means, how to report, how to go back to stock | agent | `docs/`, `site/`, `README.md`, `tools/screens.sh` |
| 0.7.1 | Release | M2.6's manual: the beta release for customers; docs only, the firmware as 0.7.0; passed on hardware (Alan, 2026-11-17) | lead | `docs/release-0.7.md`, version fields |
| 0.6.0 | Release | M2.1–M2.4 merged: notes in `docs/release-0.6.md`; version 0.6.0, OS version field TS06 (Alan, 2026-11-16) | lead | `docs/release-0.6.md`, version fields |
| 0.7.0 | Release | M2.5's changes, and the lead's small items: TRIG and TIME knobs hidden while COND or RTRG is 0 (M2.1's `sense()`), the device layer fuzzed on the host, the test counts in the README and `docs/testing.md` (Alan, 2026-11-16) | lead | `docs/release-0.7.md`, version fields |
| 1.0 | Release | golden vectors frozen again; a final hardware pass with `flashing.md`'s checks; tag | lead and Alan | |

M2.1, M2.2 and M2.3 start together from `main`; their test file numbers are 40, 41 and 42. M2.6 waits for M2.5's changes.

### After 1.0

- **TURING, a page of its own** (Alan, 2026-11-16): a Turing Machine for trigs, after the Music Thing Turing
  Machine and Moog's Labyrinth. While the pattern plays, the track is rewritten at the end of each loop, each step
  with a chance to change set by a CORRUPT knob: fully left freezes what plays (FUNC + YES keeps it, NO puts the
  original back); towards the middle step values (velocity, notes) drift; further right trigs flip on and off too.
  A loop length (2–16 steps) repeated across the track. First a spike: find the playhead in OS 1.74's RAM and prove a
  loop-end rewrite in the emulator. If live rewriting is not safe, the fallback is a TURING source on RHYTHM where
  each YES clocks one pass. A new feature, so a minor release (1.1.0) under semantic versioning.

## 9. Regression policy

- **Kept:** every behaviour 0.3's emulator checks cover (generation, audition, keep, NO, save paths, lock rows, groove, rotate, sample locks, the key cycle), re-expressed in 0.5 and required at G0.
- **Changed on purpose at 0.4:** what a seed produces (design V11); the page set and navigation (V10); ROT's relation to BIAS (design section 4). These are listed in the 0.4 release notes.
- **From 0.4 on:** golden vectors and gate rule 4 hold results fixed.

## 10. Risks

| Risk | Handling |
|---|---|
| Stock PAGE or TRIG keys misbehave while the mod draws | checked first in task 0.3; fallback: PAGE stays the mod's, FUNC + LEFT / RIGHT shows the pattern pages |
| Single-step moves with locks are slow or exhaust lock rows | 0.4 measures; SYNC on SOURCE = TRACK refuses with a message when rows run out (I8) |
| The mod outgrows the loader's limits | sizes reported at G0 and at every merge |
| core-ar2 0.13 has not run on a unit | Alan's first flash of 0.3; findings go to the lead before 0.6 |
| Contract changes ripple through parallel work | only the lead edits `seed.h`; requests go through `docs/status/` |

## 11. Starting agents

Parallel work means separate sessions, one per task, each in its own worktree. Two ways (Claude Code on the Mac, in the repository):

```sh
# A. one screen: dispatch a prompt per task; each becomes a background session in its own worktree
claude agents --model opus

# B. one terminal per task
claude --worktree m0-engine  --model opus
claude --worktree m0-pages   --model opus
```

Order for M0: the lead does task 0.1 alone and it is merged and pushed; then 0.2–0.5 are started together; then the lead does 0.6. For M1, start the workstreams of a release group together once gate G0 is on `main`.

The prompt for each session:


> You are working on trig-seedr. Read `docs/design-v1.md`, then `docs/plan-v1.md`. Your task is **<id>** in section 5 or 6. Work in your own worktree and its branch; edit only the files that task owns (section 4); follow the hard rules (section 2). Keep `docs/status/<id>.md` current. You are done when the task's "done when" and the gates in section 7 are met; then report what you built, the test results and anything the lead must decide.
