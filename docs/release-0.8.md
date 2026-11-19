# trig-seedr 0.8

## 0.8.0

trig-seedr comes to a second device: the **Elektron Syntakt** (OS 1.42), ready for testing. The Analog Rytm MKII
build is the same instrument as 0.7.1, on code reorganised so that both devices share it. The boot screen shows
**TS08** on both.

### The Syntakt (OS 1.42): ready for testing

- **What it does:** the same pages, recipes and generate and apply rules as on the Rytm, as a third TRIG page after
  stock's Trig Parameters and Euclidean (TRIG held + PAGE opens it from anywhere). LOCK's lanes follow each track's
  machine: its SYN controls first, then the shared pages. The pages hide what the Syntakt has not got: retrigs,
  accents, slides. NOTE is written, NOT2–NOT4 stay as they are. There is no lock-row limit on the Syntakt. Where it
  differs from the Rytm: the manual's device page, `devices/syntakt`.
- **On a unit:** its first hardware pass was on 2026-11-19, on one Syntakt with OS 1.42: the way back to stock, the
  stock file rebuilt by elektron-firmware-tool, the memory canary, then trig-seedr and its 15 first checks
  (`docs/flashing-st.md`, plan S6.1–S6.4), all passed. That pass was on the 0.7.1-labelled build of the same code;
  0.8.0 differs from it in the version field.
- **Build it** from your own stock `Syntakt_OS1.42.syx` (sha256 `ded3c59f…37c1`): `sh tools/build_release_st.sh`
  gives `build/trig-seedr-0.8.0-st-os1.42.syx`. It runs the unit-file checks first (the file rebuilt from stock with
  only the main OS section replaced, within the official file's size, an in-place margin of at least 4,096 bytes,
  Syntakt tests 01 and 13 on that exact file) and stops if any fails. Flashing and the way back:
  `docs/flashing-st.md`.
- **Not known yet:** how it behaves on other units; popups that only a unit shows (USB or card messages); the
  levels of velocities as heard (the tests check where the trigs fall, not how loud).

### The Analog Rytm MKII

- **The code is reorganised** into a device-neutral frame (`frame/`) and a Rytm part (`mods/seedr-ar2/`). The
  golden vectors are the same (129) and the full emulator suite passes. **0.8.0's Rytm build has passed the emulator
  suites but not yet a hardware pass**; 0.7.1 had one (`docs/flashing.md`).
- **New emulator tests:** A7, a save during a pending take (power off keeps the take as it stands), and A9, YES
  refused on the FX track and in stock's Euclidean mode (`tools/tests/03-save.sh`, `tools/tests/44-refuse.sh`).
- **YES + SCENE does not save while Trig Seedr is open**: a known quirk, now in the manual.
- **FLD offers only the fields a track has** (on the Rytm's voice tracks, every field, as before).

### Tools

- **elektron-firmware-tool** (Marcel Bierling, MIT; `tools/eft.sh` builds it from `../external/`) repacks the
  Syntakt's unit file.
- **`device/st_params.h`** is made locally from your own stock Syntakt file (`tools/st_params.sh`) and is not in the
  repository; without it the Rytm builds and tests run and the Syntakt's parts are skipped.
- **The memory canary** (`mods/canary-st`, `tools/st_canary.sh`) checks on a unit that the memory trig-seedr uses
  stays untouched.
- `docs/dev-environment.md`: set `WORKSPACE` and `B` on two export lines.

### Upstream used to build

| Project | Commit |
|---|---|
| elekloader | `9281c99` |
| analog-emu (the Rytm's emulator) | `2ac0f1f` |
| analog-emu, local branch `syntakt-modified` (the Syntakt's emulator; not upstream yet) | `19571f6` |
| synfact-dev (the Syntakt core's hook bus, elekloader's Syntakt profile) | `9db6680` |
| elektron-firmware-tool | `a5bce9a` |
