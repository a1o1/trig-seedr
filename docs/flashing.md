# Flashing trig-seedr to an Analog Rytm MKII

trig-seedr 0.7 is a beta ([what that means](https://a1o1.github.io/trig-seedr/beta/beta/)). Every release is tested
in full in analog-emu; the releases that have also run on a unit are listed under [Tested on hardware](#tested-on-hardware).
Flash it when a failed flash would not cost you anything, with the recovery route below ready, and work on projects
you have backed up.

## Before you start

- **The unit runs OS 1.74.** trig-seedr is built for 1.74 only (GLOBAL > SYSTEM > OS INFO, or the startup screen).
- **Back up.** Save the project, and copy your projects and samples to the computer with Elektron Transfer. An OS
  update keeps the +Drive, but a backup costs nothing.
- **Have the recovery route ready:** the stock `Analog-Rytm_MKII_OS1.74.syx`, Elektron Transfer, and a **5-pin DIN
  MIDI interface** (USB cannot be used for recovery on the Rytm). Recovery is the Rytm's own STARTUP menu, in its
  bootloader, which no elekloader build touches.

## Build

```sh
sh tools/run_tests.sh           # every file should pass
sh tools/build_release.sh       # -> build/trig-seedr-0.7-os1.74.syx, OS version field TS07
```

`build_release.sh` builds core-ar2 and seedr, patches them into your stock OS and has elekloader verify the result
(only the main OS section changed, every checksum right, the image unpacks as the bootloader will unpack it). Keep
the file to yourself: it contains Elektron's firmware.

## Flash

As for any OS update:

1. Connect the Rytm over USB and open Elektron Transfer.
2. Select the Rytm and **Connect**.
3. Drag `trig-seedr-0.7-os1.74.syx` onto **Drop files here**.
4. Press **YES** on the Rytm. Don't turn it off until the update is done and it has restarted.

## First checks on the unit

The checks below are for each release and each unit.

### Tested on hardware

Every hardware pass, newest first. Each release is tested in analog-emu in full (`docs/testing.md`); this table is
what has run on a real unit.

| Version | Date | Unit | OS | Result |
|---|---|---|---|---|
| 0.6.0 | 2026-11-16 | Analog Rytm MKII | 1.74 | played in use: works; UX notes became 0.7's range knobs, YES after a revert and the pending dot |
| 0.5.1 | 2026-11-16 | Analog Rytm MKII | 1.74 | first pass: every feature worked as expected; its notes became 0.5.1's generate and apply |

0.7.0 has not had a hardware pass yet; checks 13 to 15 below are new in it.

1. The boot screen shows the version (**TS07** for 0.7) while the Rytm starts (OS 1.74 has no menu that shows it;
   GLOBAL > SYSTEM > OS UPGRADE shows only WAITING FOR SYSEX).
2. The Rytm plays as before: load a project, play a pattern, turn some knobs.
3. **TRIG**, **TRIG** (EUCLIDEAN), **TRIG**: trig-seedr's page. **TRIG** again: back to stock's TRIG page.
4. With RECORD off: **YES** writes nothing and the screen says **RECORD TO GENERATE**; the preview strip still
   follows the knobs.
5. On a scratch pattern with an empty track, RECORD on: **YES** (a take: an empty track starts on RAND), **YES**
   (the next seed), **NO** (the original back), **YES** (the same take again), **FUNC + YES** (**APPLIED**). The steps change on the
   grid and play.
6. A take not applied comes back to the original (**NOT APPLIED**) when you: select another track (with the pads),
   select another pattern, close Trig Seedr, or turn RECORD off.
7. Save the project, power off and on: the applied take is there.
8. LEFT / RIGHT through the pages; TIME (a groove), LOCK (with samples in the project) and TOOLS (ROTATE ALL), the
   same way. While Trig Seedr is open, PAGE and the TRIG keys work as on stock.
9. With samples in the project: a take's new trigs play their sample, as trigs from the TRIG keys do.
10. TAKE CVAR = AAAB on a take: take n's trigs play on loops 1–3, take n + 1's on loop 4 (`docs/release-0.5.md`).
11. A 64-step track with NOTE as a LIST: the preview strip follows the knobs without a lag you notice.
12. With Trig Seedr open on VIEW (0.6 draws its screen only when something changed): record trigs with the TRIG keys,
    change track and pattern, and let a message run out (about 2 s). The screen follows each at once and nothing
    stays stale. LOCK: turning one lane's PARM never stops on the other lane's parameter.
13. A take pending shows a dot before the source in the title bar (`● S42`); FUNC + YES, NO or turning RECORD off
    clears it. After NO, YES brings the same take back (the same seed).
14. A range (DYNAMICS VEL): the knob moves only the high end, FUNC + knob only the low end; each stops at the other.
15. TOOLS, TOOL = YOLO, RECORD on: YES rolls a recipe and plays it (ROLL n on the strip), YES again the next; NO puts
    the track and every page's settings back; a roll, then FUNC + YES, keeps it. Try BD, SD and CH: each should
    sound like its part (kick, backbeat, hats).

Please note what you saw, good or bad, in a
[hardware report](https://github.com/a1o1/trig-seedr/issues/new?template=hardware-report.yml): the version from the
boot screen, the unit, the OS version, the checks by number, and for anything that went wrong the steps and what
happened ([Feedback](https://a1o1.github.io/trig-seedr/beta/feedback/)).

## Back to stock

- **The unit works:** flash the stock `Analog-Rytm_MKII_OS1.74.syx` with Transfer, as above.
- **It does not start:** hold **FUNC** while powering on for the STARTUP menu, press **TRIG 4** (OS UPGRADE), and
  send the stock `.syx` over **5-pin DIN MIDI** with Transfer's legacy OS upgrade (not USB).
