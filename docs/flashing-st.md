# Flashing trig-seedr to a Syntakt (draft, for S6)

**Not on the site.** trig-seedr for the Syntakt is in development: every check so far is in analog-emu
(`docs/status/syntakt-milestone.md`). This page is the plan for the first passes on Alan's unit (`docs/plan-syntakt.md`,
S6). It goes on the site only after a pass on a unit. Anything marked *unchecked* has not been seen on a unit.

## Before anything

- **The unit runs OS 1.42.** Every build here is made from the stock `Syntakt_OS1.42.syx` (sha256 `ded3c59f…37c1`)
  and refused for any other file.
- **Back up.** Save the project, and copy your projects and samples to the computer with Elektron Transfer.
- **Have the way back ready:** the stock `Syntakt_OS1.42.syx`, Elektron Transfer, and a **5-pin DIN MIDI interface**.
  Recovery is the Syntakt's STARTUP menu, in its bootloader, which no build here touches (elekloader never writes the
  bootloader; `tools/st_unit.sh` checks that every section but MAIN is stock's).
- **Practise the way back first** (S6.1 below), while the unit still runs stock.

## Back to stock

- **The unit works:** flash the stock `Syntakt_OS1.42.syx` with Transfer, as any OS update.
- **It does not start:** hold **FUNC** while powering on for the STARTUP menu, press **TRIG 4** (OS UPGRADE), and
  send the stock `.syx` over **5-pin DIN MIDI** with Transfer. The STARTUP menu's keys (TRIG 1 test mode, 2 empty
  reset, 3 factory reset, 4 OS upgrade) are from analog-emu's notes (`docs/SYNTAKT-PORT.md` section 2) and Synfact's
  recovery steps (FUNC + power): *unchecked on a unit*. Whether the STARTUP menu also takes the file over USB:
  *unchecked*; plan on DIN.

## The builds

```sh
sh tools/st_run_tests.sh       # every Syntakt file should pass (analog-emu-st)
sh tools/st_canary.sh          # the canary: build/emu-st/canary-unit.syx
sh tools/build_release_st.sh   # trig-seedr: build/trig-seedr-<version>-st-os1.42.syx, OS version field T071
```

Each unit file is the stock file rebuilt by elektron-firmware-tool with only the main OS section replaced, then
verified by elekloader (the container within the official file's size, an in-place margin of at least 4,096 bytes,
every checksum right). Keep the files to yourself: they contain Elektron's firmware.

## The order of the first flashes (S6)

| Step | Flash | What it shows | If it goes wrong |
|---|---|---|---|
| S6.1 | Nothing new: practise the way back. With the unit on stock, enter the STARTUP menu (FUNC at power-on), check TRIG 4 is OS UPGRADE, and leave it with a power cycle | the recovery route is real on this unit | stop: no further step without it |
| S6.2 | The stock file round-tripped by the tool: `build/eft/elektron-firmware-tool -i Syntakt_OS1.42.syx -r -o build/stock-rt.syx`, then `cmp` with the stock file: byte-identical, so the same as flashing stock | Transfer and the unit take a file the tool wrote | flash stock again |
| S6.3 | `build/emu-st/canary-unit.syx` (the memory canary, `mods/canary-st`) | the memory U1 found unused stays as written on the unit, also with **all 64 sample slots loaded**: the box in the bottom right says OK and a growing count | BAD and an address: write both down, flash stock; trig-seedr's area (SD2) must move |
| S6.4 | `build/trig-seedr-<version>-st-os1.42.syx` | the first checks below | flash stock (Back to stock) |

The canary writes a pattern over `0x478b0000`–`0x47bf0000` and checks it about every 7 seconds. Let it run through a
session: load all 64 sample slots, load and save projects, play patterns, change kits, power off and on. FILL shows
for about 2 s after boot, then OK and the number of whole passes. If the unit misbehaves with it, power off and
flash stock: the OS may use that memory after all, which is what the canary is there to find.

## First checks on the unit (S6.4)

Adapted from `docs/flashing.md`'s checks for the Rytm, with the Syntakt's keys.

1. The boot screen shows the OS version field (**T071** for 0.7.1). *Unchecked* where the Syntakt shows it.
2. The Syntakt plays as before: load a project, play a pattern, turn some knobs.
3. **TRIG** (Trig Parameters), **TRIG** (Euclidean), **TRIG**: trig-seedr's page. **TRIG** again: back to stock's
   Trig Parameters. **TRIG held + PAGE** opens it from any page.
4. With RECORD off: **YES** writes nothing and the screen says **RECORD TO GENERATE**.
5. On a scratch pattern with an empty track, RECORD on: **YES** (a take), **YES** (the next seed), **NO** (the
   original back), **YES** (the same take again), **FUNC + YES** (**APPLIED**). The steps change on the grid and play.
6. A take not applied comes back to the original (**NOT APPLIED**) when you select another track or pattern, close
   Trig Seedr (TRIG, or SYN, FLTR, AMP, LFO, FX, GLOBAL, MENU), or turn RECORD off.
7. **The FX track:** YES says **NOT ON THE FX TRACK** and writes nothing.
8. **Euclidean mode:** turn EUC on (TRIG, TRIG, knob D), open Trig Seedr: YES says **EUCLID IS ON: TURN IT OFF**.
9. **LOCK follows the machine:** on a track with SP TWINSHOT, LOCK lane 1 starts on SMP1; change the machine (with
   Trig Seedr closed) and open it again: the lanes list the new machine's controls. A lane on a parameter the
   machine lacks says **SETTING OUT OF RANGE**.
10. The pages hide what the Syntakt has not got: no retrigs on TRIG, no ACC or AMOD on DYNAMICS, no RTRG or ACC in
    PLACE's FLD, no retrig row on VIEW.
11. TOOLS: ROTATE ALL moves whole steps with their locks; YOLO rolls and plays a recipe; NO puts the track back.
12. **Save:** save the project (FUNC + GLOBAL), then FUNC + YES saves the pattern; change the track; FUNC + NO
    reloads it: the applied take is back.
13. **Power off and on with a take applied (I9):** the take is there after the cold boot. analog-emu needed a model
    of the Syntakt's supercap signal for this (status/U2.md): this check is the first on a unit.
14. **Power off with a take pending (A7):** after the cold boot the take is on the track, nothing is pending, and NO
    only closes Trig Seedr.
15. The canary's check, once more, with trig-seedr: load all 64 sample slots and use Trig Seedr; nothing changes
    or crashes.

Note what you saw, good or bad, with the checks by number, the version field and the OS version.

## Tested on hardware

| Version | Date | Unit | OS | Step | Result |
|---|---|---|---|---|---|
| | | Syntakt | 1.42 | | none yet |
