---
title: What beta means
description: "How 0.7 is tested, what that does and does not cover, and how to use it safely."
sidebar: { order: 1 }
---

Trig Seedr 0.7 is a beta. Its pages and features are the ones version 1's design describes, built and tested, but
they are not final: it is given out so that people use it on their own units and say what works and what does not,
and that feedback decides what changes before 1.0. Settings may be retuned, behaviour changed, and features added
or taken out. More is planned after 1.0 (a TURING page; the Analog Four MKII and Syntakt).

## How it is tested

| Where | What | For 0.7 |
|---|---|---|
| On a computer | every part of the engine, with random recipes in the millions, against the design's rules | every release |
| In analog-emu | the patched OS in an emulator that runs Elektron's own firmware, driven through the Rytm's keys and knobs: generate, NO, apply, save, reload, power cycle, the lock-row limit, every page | every release, in full |
| On a Rytm MKII with OS 1.74 | the checks in [Flashing a unit](../../getting-started/flashing/#first-checks-on-the-unit), by hand | 0.5.1 and 0.6.0 passed; **0.7.0 has not had a hardware pass yet** |

Every hardware pass is listed in [Tested on hardware](../../getting-started/flashing/#tested-on-hardware). The
details, and what no test covers (sound, other units, long sessions): [How it is tested](../../about/testing/).

## What can change before 1.0

- **Behaviour.** Some choices were left as built until people have played them: RHYTHM's role orders, how SYNC
  resolves runs, the list orders on NOTE and LOCK, DRFT's scale, CVAR's AAAB, MAP under PHRS, and SHAPE = MAP with
  another source. The manual says where each applies. If one does not work for you, say so.
- **Seeds.** A seed number gives the same take within a release. A fix before 1.0 can change which take a seed gives;
  takes you applied are ordinary pattern data and do not change.
- **Recipes** are kept in memory only, never in the project: power off forgets them (see
  [Concepts](../../manual/concepts/#recipe-and-take)).

## Using it safely

- **Back up first.** Save your projects and samples to a computer with Elektron Transfer before flashing.
- **Work on a backed-up project.** A take is written into the pattern; FUNC + YES keeps it, and a save stores
  whatever is on the track at that moment.
- **Keep the way back to stock at hand:** the stock `Analog-Rytm_MKII_OS1.74.syx`, Elektron Transfer and a 5-pin DIN
  MIDI interface. Flashing the stock OS removes Trig Seedr and leaves your projects as they are:
  [Back to stock](../../getting-started/flashing/#back-to-stock).
- Modified firmware is used at your own risk. Trig Seedr is not affiliated with or endorsed by Elektron.

## Telling us what you found

Reports from units are what a beta is for, good or bad: see [Feedback](../feedback/).
