---
title: Building
description: "Build the firmware from source against your own copy of the stock OS, and build in your own grooves."
sidebar: { order: 5 }
---

Trig Seedr is built into a patched copy of the stock OS. Nothing from Elektron is in the repository: you supply the
OS. Development and releases are on a Mac; the tools, versions and variables are in
[`docs/dev-environment.md`](https://github.com/a1o1/trig-seedr/blob/main/docs/dev-environment.md).

## What you need

Next to the `trig-seedr` folder (or point `WORKSPACE` elsewhere):

| Folder | What |
|---|---|
| `elekloader/` | [elekloader](https://github.com/elekloader/elekloader): the mod loader, the Rytm core (core-ar2 0.13), its build and patch tools |
| `analog-emu/` | [analog-emu](https://github.com/trigcondition/analog-emu), built with `cargo build --release` |
| `Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx` | the stock OS 1.74 from Elektron (or set `STOCK`) |

and an m68k-elf cross compiler (binutils and gcc, C only), with `ELEKLOADER_CROSS=m68k-elf-`.

## Build and test

```sh
sh tools/doctor.sh               # the environment
make -C lib test coldfire        # the engine: host tests (sanitizers) and a ColdFire build check
sh tools/run_tests.sh            # builds the mod, patches the OS into build/emu/seedr.syx, runs every check
sh tools/build_release.sh        # the firmware to flash: build/trig-seedr-<version>-os1.74.syx
```

Every file the build makes under `build/` contains Elektron's firmware: keep it to yourself. To flash the result,
see [Flashing a unit](../flashing/); to try it first, [the emulator](../emulator/). What each check covers:
[How it is tested](../../about/testing/).

## Your own grooves

Out of the box TIME has five MPC-style swing amounts (SWING 54 … SWING 71, `mods/seedr-ar2/grooves-swing.h`).
`tools/groove2h.py` turns grooves from your own Ableton Live installation into tables built into the firmware; when
`mods/seedr-ar2/grooves-live.h` exists, the build uses it in place of the swing amounts:

```sh
python3 tools/groove2h.py "/Applications/Ableton Live 11 Suite.app/Contents/App-Resources/Core Library/Grooves/Swing/MPC" \
    MY_GROOVES/ > mods/seedr-ar2/grooves-live.h
```

- **`.agr` files:** Live's plain-XML grooves. Each base-grid line takes the displacement of the groove note nearest
  to it, steps in between move in proportion (a time warp, as Live describes Base), and a step on a grid line takes
  that note's velocity. Files in a protected format are skipped.
- **MIDI files:** any clip exported from Live, for example a protected groove dragged onto a MIDI track and exported.
  Base comes from `--base` or the file name (`8ths` → 1/8); length from `--beats` or the file, rounded up to whole
  bars.

`grooves-live.h` is derived from Ableton's content: it is git-ignored and must stay out of the repository. A
`grooves-live.h` made before 0.4 still builds; making it again writes the new table type.
