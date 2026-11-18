# trig-seedr: plan for the Syntakt

Draft, 2026-11-18. trig-seedr on the Elektron Syntakt: the same engine (`lib/`), the same pages and the same generate
and apply rules as on the Analog Rytm MKII, behind a Syntakt device layer and a Syntakt mod. This page records what
is known, the decisions to take, and the tasks in order. The hard rules of [`plan-v1.md`](plan-v1.md) section 2
apply unchanged; so do its gates (section 7) for every merge.

## 1. What is known

### The Syntakt

| | Finding | Source |
|---|---|---|
| OS | 1.42 is current (October 2026). The tools below target 1.42; a mod targets one stock file by its hash | analog-emu `docs/SYNTAKT-PORT.md`; Synfact `mods/core-st/mod.json` |
| Platform | The Rytm MKII's family: the main CPU is an MCF5441x running a Digitakt-style OS. A second MCF5441x renders the digital voices behind the FPGA. trig-seedr only edits pattern data, which is the main CPU's | analog-emu `docs/SYNTAKT-PORT.md` section 1 |
| Tracks | 12 (1–8 digital, 9–12 analog) and an FX track: 13, as on the Rytm | analog-emu `README.md`; Synfact `mods/core-st/README.md` (0.12.17, the FX track) |
| Keys | The Digitakt's: TRIG, SYN, FLTR, AMP, LFO, FX, GLOBAL, MENU, BANK, PTN, TRK, FUNC, YES, NO, the arrows, PAGE, 16 trig keys, knobs A–H and LEVEL. No SCENE or PERF key | analog-emu `src/panel.rs` (`SYNTAKT_KEYS`); Synfact `mods/core-st/README.md` (key IDs) |
| Sequencer | Notes with up to three chord notes per trig (NOT2–NOT4), parameter locks, a Euclidean mode (since 1.30); conditions, micro timing and retrigs to be confirmed in S0.4 | Synfact `mods/arp-st/README.md`; press coverage of OS 1.30 |
| Parameter locks | Synfact writes locks into patterns (LFO3's pattern-lock service, used by ARP): pattern and track models come from the application object (`4016e804` on 1.42), a lock-aware per-step getter exists | Synfact `mods/lfo3-st/DESIGN.md`, `mods/arp-st/README.md` |

### The tools

| | State | Consequence |
|---|---|---|
| **analog-emu** (ours, `../analog-emu`) | Boots Syntakt 1.42 with both CPUs: panel, keys by name in `dtboot`, snapshots, sound for the digital tracks and a model of the analog ones | Our emulator tests (`tools/emu.sh`) can run on the Syntakt. Not yet known: whether it boots an elekloader-built Syntakt file |
| **elekloader upstream** | No Syntakt profile or core | |
| **Synfact** (`../external/synfact-dev`, GPL-2.0-or-later) | elekloader with a Syntakt profile (1.41, 1.42) and a core, `core-st` 2.0.3: `ev_tick`, `ev_draw`, `ev_key`, `ev_enc`, `ev_settings` with the same handler forms as our Rytm core, audio and storage events. Very active; versions have been withdrawn after boot errors; most features await a check on a unit | The UI hooks seedr needs exist. Ordinary mods share **64 KiB at `0x46000000`** for code and data |
| SyntHack OS, Syntakt Firmware Workbench (MIT) | Feature patches for 1.41 with their own tools | Reference only |

### trig-seedr

- `lib/` is device-neutral (`lib/seed.h`: the Rytm's encodings, "which the Syntakt and Analog Four share or map
  onto"); a device fills `struct seed_caps` and `struct seed_ctx`.
- The Rytm is in `device/rytm_ar2.h`, `device/rytm_params.h` and the 29 places `mods/seedr-ar2/seedr.c` calls them.
  `pages.c`, `preview.c` and `view.c` call none.
- **Size:** the Rytm mod is 57.7 KB of code with the swing grooves (69 KB with `grooves-live.h`) and 107 KB of data
  (`.bss`: the snapshot, the take, the work buffers). That is more than Synfact's 64 KiB.

## 2. Decisions to take

Each is taken by Alan; the tasks in section 3 that measure what it needs come first.

| # | Decision | Options | Recommendation |
|---|---|---|---|
| SD1 | The core | (a) Synfact's `core-st`, as a dependency; (b) our own Syntakt core for upstream elekloader; (c) ask elekloader to take Synfact's profile and core | After S0.3. (a) lets trig-seedr sit beside Synfact's mods on one unit and reuses its findings; it ties our releases to its core's versions |
| SD2 | Memory | (a) the 64 KiB reserve, with the data moved to the OS heap; (b) an SMZ4 bank module (heap-loaded); (c) a larger area agreed with Synfact; (d) our own area, with (b) of SD1 | After S0.3: the measured code size and what the heap gives decide |
| SD3 | Where seedr opens | after stock's TRIG sub-pages, as on the Rytm: the Syntakt's TRIG key cycles Trig Parameters and Euclidean (S0.2, measured); TRIG held + PAGE as the shortcut | Keep the Rytm's keys if S0.4 finds the same TRIG cycle; otherwise the shortcut only |
| SD4 | LOCK's parameters | The Syntakt's lockable parameters depend on each track's machine | Lanes list the parameters of the track's machine; a lane whose parameter the machine lacks is refused (SETTING OUT OF RANGE), as two lanes on one parameter are |
| SD5 | Chord notes | NOT2–NOT4 per trig | Version 1 on the Syntakt writes NOTE only and keeps NOT2–NOT4 as they are (I3). Chords stay with note pools (design §12) |
| SD6 | Releases | One version number for both devices, two firmware files per release, or separate versions | One version: `trig-seedr-<version>-os1.74.syx` and `trig-seedr-<version>-st-os1.42.syx` |

### Provisional decisions (2026-11-18)

Alan asked for the Syntakt work to go ahead without his intervention, so the lead took each recommendation above
as a provisional decision. Each stands until Alan reverses it; work that depends on one says so.

| # | Provisional decision |
|---|---|
| SD1 | Synfact's `core-st` (`../external/synfact-dev`), pinned to one commit, recorded in `mods/seedr-st/` |
| SD2 | The code in the 64 KiB reserve; the large buffers (snapshot, take, work) on the OS heap through the core's allocation exports. Revisited if S0.3's measurements say it cannot fit |
| SD3 | seedr opens as a third TRIG page, after Trig Parameters and Euclidean (measured in S0.2); TRIG held + PAGE as the shortcut |
| SD4 | LOCK's lanes list the parameters of the track's machine; a lane whose parameter the machine lacks is refused (SETTING OUT OF RANGE) |
| SD5 | NOTE only; NOT2–NOT4 kept as they are |
| SD6 | One version number for both devices; the Syntakt file is `trig-seedr-<version>-st-os1.42.syx` |

S1 starts now, on a branch, instead of after the Rytm's 1.0 tag: nothing merges to `main` until Alan reviews it,
and the Rytm's golden vectors and full suite must be unchanged.

## 3. Tasks

### S0. Research (no product code; may run before the Rytm's 1.0)

| Task | What | Done when |
|---|---|---|
| S0.1 | Alan: the OS on his Syntakt (**1.42**, 2026-11-18); the stock `Syntakt_OS1.42.syx` in `Official Firmwares/Syntakt OS 1.42/Syntakt OS/` | **done 2026-11-18:** sha256 `ded3c59f…37c1`, as Synfact's profile (`elekloader/devices/syntakt.py`) and analog-emu's notes give for 1.42 (the `8e2488…` in `core-st`'s README is 1.41's) |
| S0.2 | analog-emu on the Syntakt: boot stock 1.42 with `dtboot`, drive keys by name, save and load snapshots, power off to a card | a script in `tools/` does each, from this checkout. **Done 2026-11-18** except power off ([`status/S0.2.md`](status/S0.2.md)): `tools/st_s02.sh` |
| S0.3 | Synfact on the Mac: build `core-st` alone and with a hello mod from `../external/synfact-dev`; boot both in analog-emu; measure what the reserve and the heap give | the sizes in a status file; SD1 and SD2 can be decided |
| S0.4 | Firmware notes for 1.42, as `firmware-notes-os174.md` for the Rytm: the current track and pattern, the track record (trig flags, note, velocity, length, micro timing, condition, retrig), NOT2–NOT4, track length and scale, the lock store and its limits, grid recording, the Euclidean flag, the TRIG page's sub-pages, the step routines stock uses to write | `docs/firmware-notes-st142.md`, each finding marked measured or inferred, Synfact's findings credited |
| S0.5 | The lockable parameters of each Syntakt machine, as data (`device/st_params.h`) | the table, checked against the emulator's parameter pages. **Done 2026-11-18** ([`status/S0.5.md`](status/S0.5.md)): 44 machines, the shared pages, `st_track_params`; `tools/st_params.py`, `tools/st_s05.sh` |

Gate SG0: Alan takes SD1–SD6.

### S1. Two devices in the code (after the Rytm's 1.0 tag)

| Task | What | Done when |
|---|---|---|
| S1.1 | Split `mods/seedr-ar2/seedr.c` into the device-neutral frame (keys, audition, recipes, drawing) and a Rytm part; the 29 device calls go through one interface | the Rytm's golden vectors are the same; the full Rytm suite passes; the `.syx` it builds differs from 1.0's only where the split moves code |
| S1.2 | Host tests and the fuzzer on a second profile: the Syntakt's capabilities (13 tracks, its fields, its micro-timing range, machine-dependent lock parameters) | `make test` runs every module on both profiles |
| S1.3 | `tools/emu.sh` per device: key names, addresses and the fixtures chosen by the firmware loaded | the Rytm suite passes unchanged |

### S2. The Syntakt device layer

| Task | What | Done when |
|---|---|---|
| S2.1 | `device/syntakt_st.h`: read, write, snapshot and restore, the lock-room check and the change notice, as `rytm_ar2.h` does, through stock's step routines | a host test with a model of stock's memory, as `test_rytm_ar2.c` (I6–I8) |
| S2.2 | `device/cf_syntakt_st.c`: the ColdFire build check | no undefined symbols |

### S3. The Syntakt mod

| Task | What | Done when |
|---|---|---|
| S3.1 | `mods/seedr-st/`: `mod.json` for the core SD1 picks, the device part of the frame, opening and closing (SD3), the messages | it boots in analog-emu and opens |
| S3.2 | The pages: LOCK's lists per machine (SD4); every other page as on the Rytm | `test_pages` and `test_interplay` pass for the Syntakt |
| S3.3 | Memory as SD2 decides | the build fits, with the margin a status file states |

### S4. Emulator tests on the Syntakt

The Rytm's checks, on the Syntakt: generate, NO, apply (01, 13), save and reload, power cycle (03, I9), the lock
limit (12, I8), the reference comparison (I2), the refusals on the FX track and in Euclidean mode (44), the fuzz
sample (41), YOLO (43). Each file runs on both devices where it can; a Syntakt-only check has its own file.

### S5. Documentation

The manual says where the Syntakt differs (keys, LOCK's lists, chord notes); the site's install and back-to-stock
pages cover the Syntakt (its STARTUP menu, the stock file over DIN MIDI); `requirements.md` names each
requirement's tests on each device; `testing.md` gives both devices' counts.

### S6. On a unit

Alan's Syntakt: `flashing.md`'s checks, adapted, with recovery to stock practised first. Then a beta for the Syntakt,
on the Rytm's rules for a beta.

## 4. Order

S0 can start now: it writes notes and scripts, not product code, and does not touch the Rytm's 1.0. S1 waits for the
1.0 tag, so 1.0 is not delayed or changed by the split. S2–S5 follow in order; S2 and S5's drafts can run beside
S3.

## 5. Risks

| Risk | Mitigation |
|---|---|
| Synfact's core changes often, and a build is tied to one core version | pin the core version per release; S0.3 measures how often its event sites move |
| The 64 KiB reserve | SD2, decided on S0.3's measurements |
| analog-emu does not boot an elekloader-built Syntakt file | S0.3 finds out first; analog-emu is ours to fix |
| Machine-dependent locks make LOCK more complex than on the Rytm | S0.5 before S3.2; SD4 keeps refusals as the backstop |
| Bricking a unit | elekloader never touches the bootloader; recovery to stock is practised before the first flash (S6) |
| A new Syntakt OS | each release names its stock file; a new OS is a new port of the device layer, as on the Rytm |
