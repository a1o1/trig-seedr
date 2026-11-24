---
title: Syntakt
description: "Trig Seedr on the Syntakt (OS 1.42): ready for testing. Where it differs from the Analog Rytm MKII."
sidebar: { order: 3 }
---

:::note[Ready for testing]
Trig Seedr for the Syntakt (OS 1.42) is ready for testing, as a beta. Every release is tested in full in an
emulator running the stock Syntakt OS 1.42, and it has passed hardware passes on one unit (0.7.1, 2026-11-19; 0.9.0, 2026-11-24).
How to build and flash it, the way back to stock and the first checks: [Flashing a Syntakt](../../getting-started/flashing-syntakt/).
:::

The manual describes the Analog Rytm MKII. On the Syntakt, Trig Seedr has the same pages, knobs and rules (the
recipe, generate and apply, the seed, YOLO). This page lists only what differs.

## Opening and closing

The same keys as on the Rytm: stock's TRIG key cycles Trig Parameters, then Euclidean; TRIG on the Euclidean page
opens Trig Seedr as a third TRIG page. TRIG held + PAGE opens it from any page.

| Key | Does |
|---|---|
| TRIG on Trig Seedr | closes it (stock's TRIG page); a pending take is not applied |
| SYN, FLTR, AMP, LFO, FX, GLOBAL or MENU | closes it; a pending take is not applied |
| YES, FUNC + YES, NO, LEFT / RIGHT, UP / DOWN, knobs A–H | as on the Rytm ([Keys and messages](../../manual/keys/)) |

## Tracks

| Tracks | Trig Seedr |
|---|---|
| 1–8 (digital) | generates |
| 9–12 (analog) | generates |
| FX | YES says NOT ON THE FX TRACK and writes nothing, as on the Rytm |

A track in stock's Euclidean mode is refused as on the Rytm: EUCLID IS ON: TURN IT OFF.

## What a take writes

| Field | On the Syntakt |
|---|---|
| NOTE, VEL, LEN, micro timing (−23 to +23), trig conditions | written, as on the Rytm |
| chord notes (NOT2–NOT4) | kept as they are; Trig Seedr writes the trig's NOTE only |
| retrigs, the accent flag, slide | the Syntakt's trigs have none: the knobs for them (TRIG's RTRG, RATE, RCUR; DYNAMICS' ACC, AMOD) are hidden, and VIEW shows no retrig row |
| parameter locks | written, two lanes, as on the Rytm (below) |

## LOCK

The lanes follow the track's machine:

- **The list.** PARM lists the machine's SYN controls first, in knob order, then the pages every machine shares
  (filter, amp, LFO). A machine change gives the track a different list.
- **The defaults.** On a new recipe, lane 1 starts on the machine's knob A (SMP1 on SP TWINSHOT) and lane 2 on the
  filter's frequency. RNGE starts on the parameter's whole range.
- **After a machine change.** A lane keeps its parameter when the new machine has it. A lane on a parameter the
  new machine lacks is refused: YES and the strip say SETTING OUT OF RANGE until PARM is turned.

The Syntakt keeps every lockable parameter of every step, with no shared pool of lock rows. NO LOCK ROWS LEFT does
not occur on the Syntakt.

## TURING (0.9)

The TURG source and the live page as on the Rytm ([TURING](../../manual/turing/)). The live page reads the
Syntakt's playhead each screen refresh; a step takes about 0.3 ms to write. FUNC + the SCALE knob (ROOT) has not been
tried on a Syntakt yet: the emulator cannot hold FUNC there.

## Grooves

TIME's GRV offers the swing grooves on the Syntakt. A build with your own Live grooves (`grooves-live.h`) does not
fit the Syntakt's OS file from 0.9.0, so the Syntakt's build leaves them out; the Rytm keeps them.

## TOOLS

ROTATE ALL moves whole steps with their locks, on every track within its length. A track in stock's Euclidean mode
is skipped, as on the Rytm.

## Saving

With Trig Seedr closed, stock's keys save as usual: FUNC + YES saves the pattern and FUNC + NO reloads it. The
Syntakt saves a pattern only into a project that has been saved on its card (FUNC + GLOBAL). With Trig Seedr open,
FUNC + YES applies the take, as on the Rytm.

The Syntakt keeps its working state, a take included, when it is switched off, as the Rytm does: an applied take
is there after the next start, and a take still pending is on the track as it stood, with nothing pending. Recipes
are kept in memory only, as on the Rytm.

## Stock's pop-ups

Trig Seedr steps aside while one of stock's menus or pop-ups is on the screen (TEMPO, pattern and track select,
FUNC menus), as on the Rytm. While FUNC is held it keeps its screen, so FUNC + YES and FUNC + knob work. Pop-ups that
only a unit shows (USB or card messages) were not seen in the emulator.

## Tested

| Where | What |
|---|---|
| In analog-emu (OS 1.42) | every release: the Syntakt's emulator suite, every take compared with the reference renderer; the sequencer plays what Trig Seedr writes |
| On a Syntakt with OS 1.42 | 0.7.1 (2026-11-19): the way back to stock, the canary with all 64 sample slots loaded, and the first checks 1–15, passed on one unit ([Tested on hardware](../../getting-started/flashing-syntakt/#tested-on-hardware)) |
