---
title: The Syntakt
description: "Trig Seedr on the Syntakt (OS 1.42): in development, tested in an emulator only. Where it differs from the Analog Rytm MKII."
sidebar: { order: 14 }
---

:::caution[In development]
Trig Seedr for the Syntakt is in development. It has been tested in an emulator only, running the stock Syntakt OS
1.42. There is no Syntakt file to flash, and none of the pages here are install instructions.
:::

The rest of the manual describes the Analog Rytm MKII. On the Syntakt, Trig Seedr has the same pages, knobs and
rules (the recipe, generate and apply, the seed, YOLO). This page lists only what differs.

## Opening and closing

The same keys as on the Rytm: stock's TRIG key cycles Trig Parameters, then Euclidean; TRIG on the Euclidean page
opens Trig Seedr as a third TRIG page. TRIG held + PAGE opens it from any page.

| Key | Does |
|---|---|
| TRIG on Trig Seedr | closes it (stock's TRIG page); a pending take is not applied |
| SYN, FLTR, AMP, LFO, FX, GLOBAL or MENU | closes it; a pending take is not applied |
| YES, FUNC + YES, NO, LEFT / RIGHT, UP / DOWN, knobs A–H | as on the Rytm ([Keys and messages](../keys/)) |

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

## TOOLS

ROTATE ALL moves whole steps with their locks, on every track within its length. A track in stock's Euclidean mode
is skipped, as on the Rytm.

## Saving

With Trig Seedr closed, stock's keys save as usual: FUNC + YES saves the pattern and FUNC + NO reloads it. The
Syntakt saves a pattern only into a project that has been saved on its card (FUNC + GLOBAL). With Trig Seedr open,
FUNC + YES applies the take, as on the Rytm.

Whether the Syntakt keeps the working state, a take included, when it is switched off is not known yet: the
emulator does not save it. Recipes are kept in memory only, as on the Rytm.

## Not done yet

- Stock's pop-up messages: Trig Seedr draws over them while it is open.
- A check on a Syntakt unit.
