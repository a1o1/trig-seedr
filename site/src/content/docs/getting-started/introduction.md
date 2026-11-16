---
title: Introduction
sidebar: { order: 1 }
---

Trig Seedr writes pattern seeds for Elektron machines: pages on the machine's own screen that write patterns for you. The first version is for the Analog Rytm MKII; the Analog Four MKII and the Syntakt are planned next. It runs on the Rytm itself, as a mod for [elekloader](https://github.com/elekloader/elekloader), on OS 1.74.

Everything it writes is ordinary Rytm data (trigs, velocities, lengths, micro timing, conditions, retrigs, sample locks), written through the OS's own routines. The patterns play, save and reload on a stock OS, and you can edit them by hand as usual.

## What it does

- **Generate** a track: trigs on a grid at a density you set, with velocity, length, note and micro-timing ranges, accents, trig conditions, retrigs and sample-slot locks.
- **Groove** a track: micro timing and velocity from Ableton Live grooves (`.agr`) or MIDI files.
- **Rotate** a track or the whole pattern: whole steps with their locks, or one field (velocity, note, micro timing …) against trigs that stay put.

## How it feels

You **generate** and then **apply**. With RECORD on, YES writes a take so you can hear it and keeps your original; each YES after that starts again from the original, so you are always choosing between takes, never piling changes on changes. **FUNC + YES applies** the take, the only way it stays; NO, or leaving the track or pattern, puts the original back. With RECORD off, YES writes nothing, so looking around is always safe. ([Generate and apply](../../using/pages/#generate-and-apply))

## Status

Version 0.6 runs in [analog-emu](https://github.com/trigcondition/analog-emu), a whole-device emulator running the stock OS, where every scripted check passes (`tools/run_tests.sh`). Version 0.5 passed a **first test on an Analog Rytm MKII running stock OS 1.74**: every feature worked as expected. To try it on yours, see [Flashing](../flashing/).
