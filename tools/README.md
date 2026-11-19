# tools

## Tests

    sh tools/doctor.sh [--host]      the environment (docs/dev-environment.md section 7); no emulator
    sh tools/run_tests.sh [NAME...]  every tools/tests/NN-*.sh, or those whose name contains NAME (about 9 minutes)
    sh tools/golden.sh [--write]     the golden vectors in lib/golden/
    sh tools/fuzz.sh [SECS [JOBS]]   the long fuzz run of lib/test_fuzz.c with ASan and UBSan (docs/testing.md)
    sh tools/build_release.sh        the firmware to flash (docs/flashing.md)
    sh tools/screens.sh [--install]  the screenshots for the README and the site's manual (docs/screens/)

`run_tests.sh` builds core-ar2 and the seedr mod, patches the stock OS 1.74 (`build/emu/seedr.syx`, version GR01) and
drives analog-emu's `dtboot` through the Rytm's own keys and knobs. Each check reads A01 BD from the emulator
snapshot (`emu.py dump`: trig, flags, note, velocity, length, µtime, condition, retrig, the sample-slot lock) and
compares it with `seedref`, the host build of the same chain with the same recipe.

Each test file writes only under this checkout's `build/emu/<name>/` (card, snapshots, screens, `actions.log`);
core-ar2 and the mod are built into `build/emu/` too (core-ar2 with `-std=gnu17` when the cross compiler is gcc 15 or
later). Nothing goes into `analog-emu/` or `elekloader/`, so suites in two worktrees run side by side. `WORKSPACE` is
found from a worktree's path; set it for a checkout elsewhere.

| File | Checks |
|---|---|
| `00-host` | no emulator: `make test` and `coldfire` for `lib/` and `device/`, the page-table tests, the recipe layout on the host and on ColdFire, `--fields`, identity, the golden vectors |
| `01-generate` | boot empty; the new recipe is RANDOM (V6); YES seed 1 (new trigs with stock's SYN / SMP / ENV bits); seed 2 from the original; NO; FUNC + YES applies; the applied take stays after closing |
| `02-groove` | TIME GRV on a RANDOM take; NO; SOURCE = TRACK with a groove on the kept take; GVEL (needs `grooves-live.h`); the regenerate rule |
| `03-save` | SAVE / RELOAD PATTERN; power cycle; the project's modified mark |
| `04-smplocks` | LOCK lane 1 (sample slot) locks; NO removes them; they survive SAVE + RELOAD PATTERN |
| `05-rotate` | SOURCE = TRACK with PLACE ROT, FLD VEL by trig, TOOLS' ROTATE ALL (SD moves too), NO; whole steps carry sample locks |
| `06-norows` | a test build with no lock rows left: a take with locks is refused and writes nothing (I8) |
| `07-keys` | TRIG on EUCLIDEAN opens seedr, TRIG goes back to stock's TRIG page, then EUCLIDEAN, seedr again; SRC closes it |
| `08-meter` | SKIP 2, BIAS −40 %, SHAPE NOT 1 at 100 %; NO |
| `09-inject` | a whole recipe written into the mod's store with `dtboot --poke` |
| `10-pages` | stock PAGE and TRIG keys while seedr is open (V10); LEFT / RIGHT over the ten pages; VIEW's cursor; SOURCE = TRACK identity on the record and the lock store (I1); the TRIG cycle; ROTATE ALL; a source not built refuses; a screenshot of every page |
| `11-device` | the device layer through a test mod (`11-device.c`): a step with three locks moved, saved, reloaded, power-cycled (I9); restore (I7); a lock with no room refused (I8) |
| `12-lock-rows` | stock's lock-row type for every lockable parameter on BD, MT and FX (test mod `12-lock-rows.c`); near the 72-row limit a lock that fits survives SAVE + RELOAD, one that does not is refused (I8) (task D1) |
| `13-apply` | generate and apply: RECORD off writes nothing (RECORD TO GENERATE); YES, NO, FUNC + YES (APPLIED); RECORD off, closing or another pattern with a take pending restore the original (NOT APPLIED) |
| `21`..`32` | one file per version 1 workstream (F1 `21-rank` … F12 `32-map-accents`): each feature against `seedref`, and against the original code where there is one |
| `41-fuzz-sample` | recipes from the host fuzzer (`lib/test_fuzz.c --sample`) on BD at 16, 24, 37 and 64 steps: each take against `seedref`, lane 2 included (I2); NO byte for byte (I7); applied, played, saved, reloaded, power-cycled (I9); refused with no lock rows (I8); two lanes on one parameter refused (task M2.2) |

`01` leaves the fixture `kept2` (seed 2 kept) that `02`, `03`, `05`, `07` and `08` start from; `04` leaves `smp3` for `05`.

| File | What |
|---|---|
| `emu.sh` | sourced by the tests: `emu_build [VARIANT CFLAGS]`, `emu_boot`, `emu_open`, `emu_page NAME`, `emu_knob PAGE LETTER:KEY VALUE` (turns until the mod's value is VALUE; `LETTER:shared.FIELD` for the pattern's shared settings), `emu_sknob ENCODER ADDRESS VALUE` (a stock knob, until a byte is VALUE), `emu_keys`, `emu_yes` / `emu_no` / `emu_keep`, `emu_poke` and `emu_recipe` (settings written with `dtboot --poke`), `emu_dump`, `emu_check`, `emu_fixture` (a shared state made once per build) |
| `emu_st.sh`, `st_s02.sh` | the Syntakt (plan-syntakt.md S0.2): `emu.sh` on the stock Syntakt OS 1.42 (`emu_boot`, `st_lit` the lit trig keys); `sh tools/st_s02.sh` checks boot, keys, snapshots and power off |
| `st_build.sh`, `st_s03.sh` | the Syntakt (S0.3): `sh tools/st_build.sh NAME [--core st\|mini] [MODDIR...]` builds a core and mods with Synfact's elekloader into `build/emu-st/NAME-emu.syx` (analog-emu only); `sh tools/st_s03.sh` boots the hook-bus core and `mods/hello-st`; `ST_AREA=LO-HI` (hex) sets the mods' area, default `47a00000-47b00000` (U1) |
| `eft.sh`, `st_unit.sh` | the unit-shaped Syntakt file (plan-syntakt.md U3): `tools/eft.sh` compiles elektron-firmware-tool (`../external/`) into `build/eft/`; `st_build.sh --unit` repacks seedr's MAIN into the stock file and verifies it; `sh tools/st_unit.sh` checks it with the tool and runs tests-st 01 and 13 on it (`ST_UNIT=1`). Not for a unit yet |
| `st_s2.sh` | the Syntakt device layer (plan-syntakt.md S2) in analog-emu through the test mod `mods/devtest-st`: attach, snapshot, a known take written through stock's routines and read back with `st_peek.py`, restore byte for byte |
| `st_s3.sh` | seedr on the Syntakt (plan-syntakt.md S3): builds `mods/seedr-st` on the hook-bus core and checks in `../analog-emu-st` that it opens as the third TRIG page, steps every page, takes knobs, refuses YES with RECORD off, writes a take with it on and NO restores the track byte for byte, and refuses the FX track; screenshots in `build/emu/st-s3/` |
| `st_s3b.sh` | the Syntakt mod's S3b points (plan-syntakt.md): the track's machine read from its Sound, LOCK's lists and lanes through a machine change (BD MODERN to SY BITS: SETTING OUT OF RANGE for a lane the new machine lacks), RNGE defaults, ROTATE ALL +3 against each dump row rotated, and the project's modified mark |
| `st_u1.sh` | the Syntakt's mods' area (plan-syntakt.md U1): a canary over `ST_AREA` (default the gap `478b0000-47c00000`) with analog-emu-st's `dtboot --fill` / `--check-fill`, through a cold boot, a session of stock's keys, PLAY, pattern changes, a project save and load, a power off and a cold boot on that card; two controls show the probe sees the OS's boot writes |
| `st_peek.py`, `st_s04.sh` | the Syntakt's step data (plan-syntakt.md S0.4, `docs/firmware-notes-st142.md`): `st_peek.py dump SNAP [TRACK [N [PAT]]] [--lane ID]` in `emu.py dump`'s format, `rec`, `trigpage`, `euc`; `sh tools/st_s04.sh` checks them against edits made with stock keys |
| `st_params.py`, `st_s05.sh` | the Syntakt's lockable parameters (plan-syntakt.md S0.5): `st_params.py STOCK.syx ELEKLOADER_DIR` prints `device/st_params.h` from the stock 1.42 file; `sh tools/st_s05.sh` checks the header is its output and screenshots the machine menus and pages it was read from |
| `st_run_tests.sh`, `tests-st/` | the Syntakt's emulator tests (plan-syntakt.md S4): `sh tools/st_run_tests.sh [NN...]` runs `tools/tests-st/NN-*.sh` (st.sh and fixtures.sh: emu.sh's helpers on seedr-st in `../analog-emu-st`; every take against `seedref --device st`) |
| `emu.py` | symbols from the patched OS's `.map.json`, memory, pokes, the track dump, struct layouts from the cross compiler |
| `seedref.c` | the version 1 chain on the host, with the Rytm's capabilities (`device/rytm_params.h`) and the mod's grooves: `seedref KEY=VALUE...` (every `struct seed_recipe` field by name; `orig=FILE` an original track as a dump) prints the take in `emu.py dump`'s format; `--keys`, `--fields`, `--recipe`, `--layout`; `--device st [machine=TYPE]` the Syntakt's capabilities per track, `--ids` its parameter list |
| `golden.sh` | `lib/golden/recipes.txt` through seedref against `lib/golden/NAME.txt` |
| `tests/fixtures.sh` | the shared fixtures and `check_take LABEL SEEDREF-ARGS` |

The mod's state (page, open, knob values, the recipe store `recipe[]` and `recipe_set`) is read from the snapshot by
symbol, so navigation and knob settings are checked, not timed.

`test_groove2h.py GROOVE_DIR` writes five of Live's plain `.agr` grooves out as MIDI files (format 0 and 1) and checks
`groove2h.py` gives the same tables from both (±1: MIDI holds whole ticks and velocities).

## Helpers

| Script | What |
|---|---|
| `groove2h.py` | `.agr` / `.mid` → groove tables (`--dump FILE` shows what it read) |
| `emucmp.py` | one track of an emulator snapshot in release 0.3's comparison format (trig and velocity line, flag words, µtime, sample locks, the modified mark) |
| `presses.py` | menu key sequences → `dtboot --press` arguments |
| `fb2png.py` | an emulator screen (`dtpeek SNAP fb`) → PNG |
| `voice.py`, `onsets.py` | one channel of `dtboot --usb-wav` (2 = BD), and its hits |

## Firmware study

Need `pip install capstone` (and `unicorn` for `emu_steps.py`) and an elekloader checkout. Run them in a scratch
directory outside the repository: the unpacked sections are Elektron's firmware, never commit or share them.

    python3 unpack.py <elekloader checkout> <Analog-Rytm_MKII_OS1.74.syx>   # sec3.bin = the OS at 0x40000400
    python3 cfdis.py 400c0776 80      # disassemble: address, byte count (hex)
    python3 scan.py 400c0300 400c2800 # functions writing at a fixed offset from a pointer
    python3 xref.py 400c2482          # callers and references
    python3 fns.py 400c2a00 400c3e00  # function starts in a range
    python3 summ.py 400c33ae          # a function's calls and the offsets it touches
    python3 emu_steps.py sec3.bin     # runs the stock step setters against a fake track object
    sh emu_rytm_steps.sh              # analog-emu: places trigs and sets fields through the UI, prints the record

## Samples in the emulator

The emulator's card has no factory samples. `dtcard IMAGE add FILES.wav` puts WAVs in `incoming`; on the Rytm, GLOBAL >
SAMPLES > incoming > RIGHT > SELECT ALL > RIGHT > LOAD TO PROJ > YES loads them into the project's slots, and
`dtboot --power-off` keeps them on the card for the next cold boot.
