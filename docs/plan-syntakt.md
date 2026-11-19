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

### Decisions (confirmed by Alan, 2026-11-19)

The lead took each recommendation above as a provisional decision on 2026-11-18, so the work could go ahead without
Alan's intervention; the work then measured them. Alan confirmed all six on 2026-11-19.

| # | Decision | Evidence |
|---|---|---|
| SD1 | **A minimal core of our own**: core-st's hook bus alone (`tools/st_build.sh --core mini`; 456 bytes of code), not Synfact's full `core-st` | `core-st` 2.0.3 does not link with gcc 16.2 (3,680 bytes over its area; S0.3). The minimal core gives tick, draw, key, encoder and SETTINGS. seedr does not run beside Synfact's mods on one unit; revisited only if users ask for that |
| SD2 | **U1's area, `0x47a00000`–`0x47b00000`** (`ST_AREA`'s default) for the code and data; the large buffers from the OS heap (`fw_malloc`) | Synfact's 64 KiB area at `0x46000000` held seedr only with `-Os` and 836 bytes spare, and stock's sample blob reaches it. U1's range is never used by the OS in every emulator check (status/U1.md). Before any flash: U1's canary on the unit, with all 64 sample slots loaded |
| SD3 | **A third TRIG page** after Trig Parameters and Euclidean; TRIG held + PAGE as the shortcut | The Syntakt's TRIG key cycles the Rytm's two sub-pages (measured, S0.2); `tools/st_s3.sh` opens seedr there |
| SD4 | **LOCK follows the track's machine**: its SYN controls first, then the shared pages; lane 1 on knob A (SMP1 on SP TWINSHOT); a lane the machine lacks is refused (SETTING OUT OF RANGE) | S3b; the takes equal `seedref --device st` field by field (S4) |
| SD5 | **NOTE only**; NOT2–NOT4 kept as they are | I3: a layer changes only its fields. Chords come with note pools (design §12), for both devices |
| SD6 | **One version number, two files**: `trig-seedr-<version>-os1.74.syx` and `trig-seedr-<version>-st-os1.42.syx`; the Syntakt's file is a beta until it passes the hardware checks on Alan's unit (S6) | One engine, golden vectors and frame; both suites run from one branch |

S1 starts now, on a branch, instead of after the Rytm's 1.0 tag: nothing merges to `main` until Alan reviews it,
and the Rytm's golden vectors and full suite must be unchanged.

## 3. Tasks

### S0. Research (no product code; may run before the Rytm's 1.0)

| Task | What | Done when |
|---|---|---|
| S0.1 | Alan: the OS on his Syntakt (**1.42**, 2026-11-18); the stock `Syntakt_OS1.42.syx` in `Official Firmwares/Syntakt OS 1.42/Syntakt OS/` | **done 2026-11-18:** sha256 `ded3c59f…37c1`, as Synfact's profile (`elekloader/devices/syntakt.py`) and analog-emu's notes give for 1.42 (the `8e2488…` in `core-st`'s README is 1.41's) |
| S0.2 | analog-emu on the Syntakt: boot stock 1.42 with `dtboot`, drive keys by name, save and load snapshots, power off to a card | a script in `tools/` does each, from this checkout. **Done 2026-11-18** except power off ([`status/S0.2.md`](status/S0.2.md)): `tools/st_s02.sh` |
| S0.3 | Synfact on the Mac: build `core-st` alone and with a hello mod from `../external/synfact-dev`; boot both in analog-emu; measure what the reserve and the heap give | the sizes in a status file; SD1 and SD2 can be decided. **Done 2026-11-18** ([`status/S0.3.md`](status/S0.3.md)): core-st fills its 64 KiB area by itself (3,680 bytes over with gcc 16); a hook-bus core (440 bytes) and hello boot in analog-emu (local branch `syntakt-modified`); the heap gives 8 MiB. `tools/st_build.sh`, `tools/st_s03.sh` |
| S0.4 | Firmware notes for 1.42, as `firmware-notes-os174.md` for the Rytm: the current track and pattern, the track record (trig flags, note, velocity, length, micro timing, condition, retrig), NOT2–NOT4, track length and scale, the lock store and its limits, grid recording, the Euclidean flag, the TRIG page's sub-pages, the step routines stock uses to write | `docs/firmware-notes-st142.md`, each finding marked measured or inferred, Synfact's findings credited. **Done 2026-11-18** ([`status/S0.4.md`](status/S0.4.md)): `tools/st_peek.py`, `tools/st_s04.sh` (13 PASS) |
| S0.5 | The lockable parameters of each Syntakt machine, as data (`device/st_params.h`) | the table, checked against the emulator's parameter pages. **Done 2026-11-18** ([`status/S0.5.md`](status/S0.5.md)): 44 machines, the shared pages, `st_track_params`; `tools/st_params.py`, `tools/st_s05.sh` |

Gate SG0: Alan takes SD1–SD6. **Passed 2026-11-19** (section 2).

**For Alan's review (rule 1):** `device/st_params.h` is generated by `tools/st_params.py` from the stock file's
parameter table: names as the screen shows them, ids and ranges, as `rytm_params.h` holds for the Rytm (written
there by hand). No firmware bytes or code are copied; whether a table generated this way may be committed and
published is Alan's call before anything leaves the branch.

### S1. Two devices in the code (after the Rytm's 1.0 tag)

| Task | What | Done when |
|---|---|---|
| S1.1 | Split `mods/seedr-ar2/seedr.c` into the device-neutral frame (keys, audition, recipes, drawing) and a Rytm part; the 29 device calls go through one interface | the Rytm's golden vectors are the same; the full Rytm suite passes; the `.syx` it builds differs from 1.0's only where the split moves code. **Done on branch `worktree-s1-split`** ([`status/S1.md`](status/S1.md)): `frame/` and `frame/frame.h`; host tests and the targeted emulator files pass; the full suite waits for the lead's batch |
| S1.2 | Host tests and the fuzzer on a second profile: the Syntakt's capabilities (13 tracks, its fields, its micro-timing range, machine-dependent lock parameters) | `make test` runs every module on both profiles. **Done** with a provisional profile (`lib/props.h`, `-DPROP_DEVICE_ST`), to switch to S0.5's tables |
| S1.3 | `tools/emu.sh` per device: key names, addresses and the fixtures chosen by the firmware loaded | the Rytm suite passes unchanged. **Not needed yet:** S0.2's `tools/emu_st.sh` sources `emu.sh` unchanged and replaces what differs |

### S2. The Syntakt device layer

| Task | What | Done when |
|---|---|---|
| S2.1 | `device/syntakt_st.h`: read, write, snapshot and restore, the lock-room check and the change notice, as `rytm_ar2.h` does, through stock's step routines | a host test with a model of stock's memory, as `test_rytm_ar2.c` (I6–I8). **Done 2026-11-19** ([`status/S2.md`](status/S2.md)): `device/test_syntakt_st.c` 13,243 checks; in analog-emu `tools/st_s2.sh` 10 PASS |
| S2.2 | `device/cf_syntakt_st.c`: the ColdFire build check | no undefined symbols. **Done 2026-11-19**: 12,506 bytes of code, 4 of data |

### S3. The Syntakt mod

| Task | What | Done when |
|---|---|---|
| S3.1 | `mods/seedr-st/`: `mod.json` for the core SD1 picks, the device part of the frame, opening and closing (SD3), the messages | it boots in analog-emu and opens. **Done 2026-11-19** ([`status/S3.md`](status/S3.md)): `tools/st_s3.sh`, 18 PASS: opens as the third TRIG page, every page, knobs, RECORD TO GENERATE, YES writes a take and NO restores record and lock store byte for byte, NOT ON THE FX TRACK |
| S3.2 | The pages: LOCK's lists per machine (SD4); every other page as on the Rytm | `test_pages` and `test_interplay` pass for the Syntakt. **Done 2026-11-19** (S3, then S3b, [`status/S3b.md`](status/S3b.md)): LOCK lists the track's machine's SYN controls, then the shared pages; a lane follows its parameter through a machine change, and one the new machine lacks says SETTING OUT OF RANGE (`tools/st_s3b.sh`, 28 PASS). `test_interplay` on the Syntakt profile (now `st_params.h`'s list) 560,288 checks, 0 failed |
| S3.3 | Memory as SD2 decides | the build fits, with the margin a status file states. **Done**: core 456 + `.run` 53,788 + `.bss` 10,420 in the 64 KiB area after S3b, **836 bytes spare**; about 163 KB from the native heap (S3b adds ROTATE ALL's 10.6 KB scratch) |

| S3b | S3's open points: the track's machine (LOCK per machine), VIEW without retrig, LOCK's RNGE default, `lib/props.h`'s Syntakt parameters, ROTATE ALL, the modified mark | **Done 2026-11-19** ([`status/S3b.md`](status/S3b.md)); stock's popups over the screen (`sd_stock_overlay`) still not found |

### S4. Emulator tests on the Syntakt

The Rytm's checks, on the Syntakt: generate, NO, apply (01, 13), save and reload, power cycle (03, I9), the lock
limit (12, I8), the reference comparison (I2), the refusals on the FX track and in Euclidean mode (44), the fuzz
sample (41), YOLO (43). Each file runs on both devices where it can; a Syntakt-only check has its own file.

**Done 2026-11-19** ([`status/S4.md`](status/S4.md)): `tools/st_run_tests.sh` over `tools/tests-st/` (01 generate, 03
save and reload, 13 apply, 14 fields and locks, 15 playback, 41 fuzz sample, 43 YOLO, 44 refusals), every take against
`tools/seedref --device st`. Not tested: the working state across a power cycle (analog-emu), the lock-row limit
(the Syntakt has none).

### S5. Documentation

The manual says where the Syntakt differs (keys, LOCK's lists, chord notes); the site's install and back-to-stock
pages cover the Syntakt (its STARTUP menu, the stock file over DIN MIDI); `requirements.md` names each
requirement's tests on each device; `testing.md` gives both devices' counts.

**Done 2026-11-19, for the emulator milestone:** the manual's page *The Syntakt* (`site/.../manual/syntakt.md`) and
notes on Keys, LOCK, TRIG and DYNAMICS; "in development, emulator only" on the front page, the introduction, the
beta page and the README; `requirements.md`'s Syntakt table; `testing.md`'s Syntakt section. The install and
back-to-stock pages wait for a unit build (S6): no page offers a Syntakt file or install steps.

### S6. On a unit

Alan's Syntakt, in order, each step only after the one before passed ([`flashing-st.md`](flashing-st.md); the files
from `tools/st_canary.sh` and `tools/build_release_st.sh`, prepared 2026-11-19, [`status/S6-prep.md`](status/S6-prep.md)).

| Step | Alan does | Done when |
|---|---|---|
| S6.1 | Practise the way back on stock: the STARTUP menu (FUNC at power-on), TRIG 4 is OS UPGRADE, a 5-pin DIN MIDI interface and the stock file at hand | the menu and TRIG 4 seen on the unit |
| S6.2 | Flash the stock file round-tripped by elektron-firmware-tool (`-r`, byte-identical to stock) | it flashes and boots as stock |
| S6.3 | Flash `build/emu-st/canary-unit.syx`; a session with all 64 sample slots loaded, projects loaded and saved, a power cycle | the box says OK and a growing count throughout; BAD stops S6 until SD2's area moves |
| S6.4 | Flash `build/trig-seedr-<version>-st-os1.42.syx`; `flashing-st.md`'s 15 first checks | every check passes, noted in its *Tested on hardware* table |
| S6.5 | A beta for the Syntakt, on the Rytm's rules for a beta; `flashing-st.md` goes on the site | after S6.4 |

### U. Unit readiness

What a build needs before it can go on a unit. None of these flashes or contacts a unit.

| Task | What | Done when |
|---|---|---|
| U1 | A mods' area the OS never uses, in place of `0x46000000`, which stock's sample blob reaches; room in it | **Done 2026-11-19** ([`status/U1.md`](status/U1.md)): `0x478af2ca`–`0x47c00000` unused (code, layout, canary in analog-emu); `tools/st_build.sh` builds at `ST_AREA`, default `0x47a00000`–`0x47b00000` (1 MiB, 983,876 bytes spare with seedr); every Syntakt check passes there |
| U2 | Power off in analog-emu on the Syntakt: why it saved nothing, and the power-cycle check (I9, A7) | **Done 2026-11-19** ([`status/U2.md`](status/U2.md)): analog-emu-st `5c970d9` models the Syntakt's supercap signal; `tools/tests-st/03-save.sh` checks a kept take and a pending take across a power cycle |
| U3 | The file: the stock 1.42 file rebuilt by elektron-firmware-tool (`../external/elektron-firmware-tool`, MIT) with seedr's patched MAIN only, within the official container's size and with an in-place margin of at least 4096 bytes | **Done 2026-11-19** ([`status/U3.md`](status/U3.md)): `tools/st_build.sh --unit`, `tools/st_unit.sh`. Unit-shaped only: it also needs U1's safe memory area |

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
