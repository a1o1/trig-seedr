# Flashing trig-seedr to a Syntakt

trig-seedr for the Syntakt is **ready for testing** (beta, [what that means](https://a1o1.github.io/trig-seedr/beta/beta/)).
It is tested in full in analog-emu, and it passed a first hardware pass on one unit (below, under
[Tested on hardware](#tested-on-hardware)). Flash it when a failed flash would not cost you anything, with the way
back to stock ready, and work on projects you have backed up. What differs from the Analog Rytm MKII:
[the Syntakt](https://a1o1.github.io/trig-seedr/devices/syntakt/).

## Before you start

- **The unit runs OS 1.42.** Every build is made from the stock `Syntakt_OS1.42.syx` (sha256 `ded3c59f…37c1`) and
  refused for any other file.
- **Back up.** Save the project, and copy your projects and samples to the computer with Elektron Transfer.
- **Have the way back ready:** the stock `Syntakt_OS1.42.syx`, Elektron Transfer, and a **5-pin DIN MIDI interface**.
  Recovery is the Syntakt's STARTUP menu, in its bootloader, which no build here touches (elekloader never writes the
  bootloader; `tools/st_unit.sh` checks that every section but MAIN is stock's).
- **Practise the way back first** (step 1 below), while the unit still runs stock.

## Back to stock

- **The unit works:** flash the stock `Syntakt_OS1.42.syx` with Transfer, as any OS update.
- **It does not start:** hold **FUNC** while powering on for the STARTUP menu, press **TRIG 4** (OS UPGRADE), and
  send the stock `.syx` over **5-pin DIN MIDI** with Transfer. The STARTUP menu and TRIG 4 were checked on a unit
  (2026-11-19). Whether the STARTUP menu also takes the file over USB is not checked: plan on DIN.

## Build

```sh
sh tools/st_run_tests.sh       # every Syntakt file should pass (analog-emu-st)
sh tools/st_canary.sh          # the memory canary: build/emu-st/canary-unit.syx
sh tools/build_release_st.sh   # trig-seedr: build/trig-seedr-<version>-st-os1.42.syx
```

Each file is the stock file rebuilt by elektron-firmware-tool with only the main OS section replaced, then verified
by elekloader (the container within the official file's size, an in-place margin of at least 4,096 bytes, every
checksum right). The build needs the stock file, `../analog-emu-st` and `../external/synfact-dev`
([Building](https://a1o1.github.io/trig-seedr/getting-started/build/)). Keep the files to yourself: they contain
Elektron's firmware.

## The order of the first flashes

| Step | Flash | What it shows | If it goes wrong |
|---|---|---|---|
| 1 | Nothing new: practise the way back. With the unit on stock, enter the STARTUP menu (FUNC at power-on), check TRIG 4 is OS UPGRADE, and leave it with a power cycle | the recovery route is real on your unit | stop: no further step without it |
| 2 | The stock file round-tripped by the tool: `build/eft/elektron-firmware-tool -i Syntakt_OS1.42.syx -r -o build/stock-rt.syx`, then `cmp` with the stock file: byte-identical, so the same as flashing stock | Transfer and the unit take a file the tool wrote | flash stock again |
| 3 | `build/emu-st/canary-unit.syx` (the memory canary, `mods/canary-st`) | the memory trig-seedr uses stays as written on your unit, also with **all 64 sample slots loaded**: the box in the bottom right says OK and a growing count | BAD and an address: write both down, flash stock, and report it |
| 4 | `build/trig-seedr-<version>-st-os1.42.syx` | the first checks below | flash stock (Back to stock) |

The canary writes a pattern over `0x478b0000`–`0x47bf0000` and checks it about every 7 seconds. Let it run through a
session: load all 64 sample slots, load and save projects, play patterns, change kits, power off and on. FILL shows
for about 2 s after boot, then OK and the number of whole passes. If the unit misbehaves with it, power off and
flash stock: the OS may use that memory after all, which is what the canary is there to find.

## First checks on the unit

The Analog Rytm MKII's checks ([Flashing a Rytm MKII](https://a1o1.github.io/trig-seedr/getting-started/flashing/)),
with the Syntakt's keys.

1. The boot screen shows the OS version field (**TS09** for 0.9.0, **TS08** for 0.8.0, **T071** for 0.7.1).
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
13. **Power off and on with a take applied:** the take is there after the cold boot.
14. **Power off with a take pending:** after the cold boot the take is on the track, nothing is pending, and NO
    only closes Trig Seedr.
15. The canary's check, once more, with trig-seedr: load all 64 sample slots and use Trig Seedr; nothing changes
    or crashes.

### TURING (0.9.0)

16. **TURG, the source:** on an empty track, RECORD on, RHYTHM SRC = TURG, FLAV DJVU, LOOP 8, AMT 25: YES writes a
    take whose 8-step loop drifts pass by pass down the track. AMT 50: the loop repeats exactly. AMT 100: the same
    steps in a shuffled order.
17. FLAV CRPT: AMT 0 repeats exactly; AMT 40 keeps the rhythm while values (with NOTE's NSRC = TURG, the notes)
    drift; AMT 100 flips trigs too. FLAV SHFT: AMT 100 repeats, AMT 0 inverts each pass (it repeats every two),
    AMT 50 is a coin flip.
18. **STRT = TRACK** on a track with trigs: the first pass is the track's own first LOOP steps, and later passes
    drift from them.
19. **SCALE and ROOT** (NOTE, knob H): NSRC = TURG, a NOTE range of −12…+12, SCALE MAJ: every note is in C major.
    FUNC + the SCALE knob turns ROOT (C … B) while a scale is on; the cell shows it (`C# MAJ`). *Unchecked on a Syntakt so far: the emulator cannot hold FUNC there.* With ROOT C#, the notes are in C# major. SCALE OFF: the notes are as before.
20. **The live page:** RIGHT from TOOLS reaches TURING. RECORD on, PLAY, YES: the strip shows LIVE and the step;
    the steps change as the playhead passes them, heard on the next loop. Turning AMT while it runs changes how much.
21. FUNC + YES keeps the track as it stands and stops (APPLIED). Start again, then NO: the original is back.
22. Start, FUNC + UP (REMEMBERED), let it drift, FUNC + DOWN (RECALLED): the remembered loop is back.
23. With the live page running: close Trig Seedr (TRIG), change track, or turn RECORD off: the original is back
    (NOT APPLIED).
24. On the FX track YES says NOT ON THE FX TRACK and nothing is written.
25. **Note what you hear:** the live page at slow and fast tempos (up to 300 BPM if you can), on drums and on a
    melodic track, with each flavour; whether the screen and keys stay responsive; a pattern change while it runs;
    a track at another speed. These have not been tried on a unit.

Please note what you saw, good or bad, in a
[hardware report](https://github.com/a1o1/trig-seedr/issues/new?template=hardware-report.yml): the device and its OS,
the version from the boot screen, the checks by number, and for anything that went wrong the steps and what
happened ([Feedback](https://a1o1.github.io/trig-seedr/beta/feedback/)).

## Tested on hardware

| Version | Date | Unit | OS | Steps | Result |
|---|---|---|---|---|---|
| 0.7.1 (T071; `trig-seedr-0.7.1-st-os1.42.syx`, sha256 `6f7444cc…`, built from commit `48691f5`) | 2026-11-19 | Syntakt | 1.42 | 1–4 above: the way back to stock (STARTUP menu, TRIG 4 = OS UPGRADE), the round-tripped stock file, the canary with all 64 sample slots loaded (OK), and first checks 1–15 | passed |
