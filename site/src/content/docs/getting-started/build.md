---
title: Building
sidebar: { order: 2 }
---

Trig Seedr is built into a patched copy of the stock OS. Nothing from Elektron is in the repository: you supply the OS.

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
make -C lib test coldfire        # the algorithms: host tests (sanitizers) and a ColdFire build check
sh tools/run_tests.sh            # builds the mod, patches the OS into build/emu/seedr.syx, runs every check
```

`build/emu/seedr.syx` contains Elektron's firmware: keep it to yourself. The details of each system (the Mac, the
Linux shell) are in the developer pages' Development environment.

## Grooves

Out of the box the TIME page has five MPC-style swing amounts. To build in your own Live grooves, see [Grooves](../../using/groove/).
