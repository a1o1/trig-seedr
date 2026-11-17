---
title: Introduction
description: "What Trig Seedr is, what it writes, and its status."
sidebar: { order: 1 }
---

Trig Seedr writes patterns for Elektron machines: pages on the machine's own screen that make and shape a track's
trigs for you. The first version is for the Analog Rytm MKII with OS 1.74, as a mod for
[elekloader](https://github.com/elekloader/elekloader); the Analog Four MKII and the Syntakt are planned next.

Everything it writes is ordinary Rytm data (trigs, velocities, lengths, notes, micro timing, accents, conditions,
retrigs, parameter locks), written through the OS's own routines. The patterns play, save and reload on a stock OS,
and you can edit them by hand as usual.

## What it does

Each track has a recipe: a **source** of trigs, then pages that shape them.

- **The trigs**: the track's own, or new ones: random with roles (kick, backbeat, hats, percussion), Euclidean, a
  map of drum patterns, or another track's.
- **Where they sit**: rotation, syncopation, a mask against another track.
- **What they carry**: velocity and accents, length and notes (a range, a list in an order, a slow contour), two
  lanes of parameter locks, trig conditions and retrigs, micro timing from a groove, at random or drifting.
- **Variation**: related takes, phrases over the pages, and two takes alternating through trig conditions.
- **Tools**: rotate the whole pattern; YOLO, a recipe rolled for the track.

## How it feels

You **generate** and then **apply**. With RECORD on, YES writes a take so you can hear it and keeps your original;
each YES after that starts again from the original, so you choose between takes instead of piling changes on
changes. **FUNC + YES** applies the take, the only way it stays; NO, or leaving the track or pattern, puts the
original back. With RECORD off, YES writes nothing, so looking around is always safe. The
[Quick start](../quick-start/) walks through it.

## Status

Version 0.7 is a **beta**. Every release passes the full set of checks in
[analog-emu](https://github.com/trigcondition/analog-emu), a whole-device emulator running the stock OS. Versions
0.5.1 and 0.6.0 passed tests on a Rytm MKII with OS 1.74; 0.7.0 has not had a hardware pass yet. The list of
hardware passes is in [Flashing a unit](../flashing/#tested-on-hardware). What beta means for you, and how to report
what you find: [Beta](../../beta/beta/).
