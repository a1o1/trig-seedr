# Flashing trig-seedr to an Analog Rytm MKII

trig-seedr has been tested in analog-emu only. The first run on a unit is a test: do it when a failed flash would not
cost you anything, with the recovery route below ready.

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
sh tools/build_release.sh       # -> build/trig-seedr-0.5-os1.74.syx, OS version field TS05
```

`build_release.sh` builds core-ar2 and seedr, patches them into your stock OS and has elekloader verify the result
(only the main OS section changed, every checksum right, the image unpacks as the bootloader will unpack it). Keep
the file to yourself: it contains Elektron's firmware.

## Flash

As for any OS update:

1. Connect the Rytm over USB and open Elektron Transfer.
2. Select the Rytm and **Connect**.
3. Drag `trig-seedr-0.5-os1.74.syx` onto **Drop files here**.
4. Press **YES** on the Rytm. Don't turn it off until the update is done and it has restarted.

## First checks on the unit

1. GLOBAL > SYSTEM > OS INFO shows the version **TS05**.
2. The Rytm plays as before: load a project, play a pattern, turn some knobs.
3. **TRIG**, **TRIG** (EUCLIDEAN), **TRIG**: trig-seedr's page. **TRIG** again: back to stock's TRIG page.
4. On a scratch pattern with an empty track: **YES** (a take: an empty track starts on RAND), **YES** (the next
   seed), **NO** (the original back), **YES**, **FUNC + YES** (kept). The steps change on the grid and play.
5. Save the project, power off and on: the kept take is there.
6. LEFT / RIGHT through the pages; TIME (a groove), LOCK (with samples in the project) and TOOLS (ROTATE ALL), the
   same way. While seedr is open, PAGE and the TRIG keys work as on stock.
7. With samples in the project: a take's new trigs play their sample, as trigs from the TRIG keys do.
8. TAKE CVAR = AAAB on a take: take n's trigs play on loops 1–3, take n + 1's on loop 4 (`docs/release-0.5.md`).
9. A 64-step track with NOTE as a LIST: the preview strip follows the knobs without a lag you notice.

Please note what you saw, good or bad, in an issue: the unit, the OS version, the steps and what happened.

## Back to stock

- **The unit works:** flash the stock `Analog-Rytm_MKII_OS1.74.syx` with Transfer, as above.
- **It does not start:** hold **FUNC** while powering on for the STARTUP menu, press **TRIG 4** (OS UPGRADE), and
  send the stock `.syx` over **5-pin DIN MIDI** with Transfer's legacy OS upgrade (not USB).
