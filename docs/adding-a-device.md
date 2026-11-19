# Adding a device

trig-seedr's engine (`lib/`) and its pages (`frame/`) are shared by every device; a device adds a device layer, a
mod, tests and its pages in the docs. This checklist is drawn from how the Syntakt was added
([`plan-syntakt.md`](plan-syntakt.md), tasks S0–S6, and its status files in [`status/`](status/)). The Analog Four
MKII and the Digitone II are the next candidates.

## What a device adds

| Part | The Syntakt's | What it is |
|---|---|---|
| A plan | `docs/plan-<device>.md` | what is known, the decisions to take, the tasks in order |
| Firmware notes | `docs/firmware-notes-st142.md` | the current track and pattern, the track record, the lock store, grid recording, the Euclidean flag, the TRIG page's sub-pages, the stock routines that write, the save path; each finding marked measured or inferred |
| Lockable parameters | `tools/st_params.py`, `tools/st_params.sh` → `device/st_params.h` | per machine and page: stock's id, the name as the screen shows it, the range. **Generated from your own stock file and never committed** (rule 1, Alan, 2026-11-19) |
| The device layer | `device/syntakt_st.h`, `device/test_syntakt_st.c`, `device/cf_syntakt_st.c` | `lib/seed.h`'s device contract: read, write through the stock routines, snapshot and restore, the lock-room check; a host test against a model of stock's memory; a ColdFire build check |
| A host profile | `lib/props.h` (`PROP_DEVICE_ST`), `frame/test_pages.c` (`PAGES_DEVICE_ST`) | every host test and the fuzzer run again on the device's capabilities |
| The mod | `mods/seedr-st/` | the device's side of `frame/frame.h`: keys, opening, drawing, the current track, grid recording, the heap, the pages' device hooks |
| The core and the file | `tools/st_build.sh`, `tools/st_unit.sh`, `tools/build_release_st.sh` | the elekloader core the mod runs on, the memory area (proven unused: `tools/st_u1.sh`, `mods/canary-st/`), the repack to a file the unit accepts, the release build and its checks |
| The emulator | analog-emu (or another), `tools/emu_st.sh` | boot, keys by name, snapshots, power off; a branch of the emulator when it needs a model of the device |
| Emulator tests | `tools/tests-st/`, `tools/st_run_tests.sh`, `tools/seedref.c --device st` | the Rytm's checks on the device: generate, apply, save, power cycle, refusals, fuzz sample, YOLO; every take against `seedref` |
| Docs | `docs/flashing-st.md`, the site's device page (`site/src/content/docs/devices/`), the manual's device notes, `docs/requirements.md`'s device table, `docs/testing.md`'s device section | the tester's page with the way back to stock, the first checks and the table of hardware passes |

## The order

1. **Research (S0):** the OS your unit runs and its stock file (its hash); the emulator on the stock OS (boot, keys,
   snapshots, power off); the elekloader core and how much memory it leaves; the firmware notes; the lockable
   parameters.
2. **Decisions (SG0):** the core, the memory, where the pages open, LOCK's parameters, what a take writes, the
   release file's name. Take them provisionally if the work must go on, and record the evidence.
3. **Two devices in the code (S1):** only if the device needs a seam the frame does not have yet; the Rytm's golden
   vectors and full suite must not change.
4. **The device layer (S2), the mod (S3), the emulator tests (S4), the docs (S5).**
5. **Unit readiness:** a memory area proven unused, the file the unit accepts, the power-off save in the emulator.
6. **On a unit (S6):** practise the way back to stock; flash the stock file round-tripped by the tool; the memory
   canary with the device's memory under load; then trig-seedr and the first checks. Record each pass.

## Rules that hold for every device

- Every build starts from your own stock file, recognised by its hash; nothing from Elektron is committed, and
  neither is anything generated from it (`.syx`, sections, snapshots, card images, `device/st_params.h`).
- The Rytm's golden vectors and its full emulator suite pass unchanged after any change to the shared code.
- A device's status on the site says what was tested where: "in development" (emulator only) until a first hardware
  pass, then "ready for testing".
