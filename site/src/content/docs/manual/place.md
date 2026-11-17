---
title: PLACE
description: "Where the trigs end up: rotation (ROT), syncopation by anticipation (SYNC), a mask against another track (MASK), the first steps left empty (SKIP), and one field rotated against the trigs (FLD)."
sidebar: { order: 4 }
---

PLACE moves and removes the trigs RHYTHM gave, in this order: **ROT**, then **SYNC**, then **MASK**, then **SKIP**.
FLD, FROT and FMOD are different: they rotate one field against the trigs, after every other page has written.

![PLACE](../../../assets/screens/place.png)

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | ROT | −63…+63 steps | 0 | always |
| B | SYNC | 0–100 % | 0 % | always |
| C | MASK | OFF, AVOID, ONLY | OFF | always |
| D | MTRK | the pattern's tracks, BD … FX | BD | while MASK is not OFF |
| E | SKIP | 0–64 steps | 0 | always |
| F | FLD | OFF, VEL, NOTE, LEN, UTIM, COND, RTRG, ACC, LANE1, LANE2 | OFF | always |
| G | FROT | −63…+63 | 0 | while FLD is not OFF |
| H | FMOD | TRIG, GRID | TRIG | while FLD is not OFF |

SKIP, MASK and DYNAMICS' shapes refer to where the steps end up, so no later knob can undo what they promise.

## ROT

Rotates the take's whole steps by ROT steps, wrapping at the track's length; negative is earlier. A step moves with
everything it carries, parameter locks included; the swing bits stay with the positions. With SRC = TRACK it
rotates the track's own trigs.

## SYNC: syncopation by anticipation

SYNC moves trigs one grid step earlier, onto a weaker step, so they arrive before the beat they belonged to and the
beat itself is left silent. SYNC is the share of the possible anticipations that is made. At 0 % it changes
nothing. The method follows George Sioros and Carlos Guedes' syncopation transformations.

A trig can be anticipated when:

- the step one grid step before it is empty, and no other trig lies in between;
- that step is weaker in the meter (TAKE's METR and BEAT; ROLE does not change it);
- neither step is one that SKIP clears.

The grid step is RHYTHM's GRID with SRC = RAND, and one step for every other source. The loop wraps: the bar's first
step can be anticipated onto the track's last step.

**Runs resolve onto a strong step.** Two or more consecutive trigs that are all anticipated form a run. A run is
allowed only when the trig after it stays where it is and is stronger than every step the run left. So the rhythm
always lands again on a stronger step, and SYNC never moves every trig (that would only shift the track by one
step). A single anticipation followed by a trig that stays is always allowed. This rule is as built in 0.7; beta
feedback may change it before 1.0.

Examples in 4/4 with BEAT 4 (steps numbered from 1):

| Trigs | SYNC 100 % gives | Why |
|---|---|---|
| 5, 10 | 4, 10 | 10 cannot move: step 9 is stronger |
| 1, 5, 9, 13 | 1, 4, 8, 12; 1, 5, 8, 12; 1, 4, 9, 12; or 5, 8, 13, 16 | at most three of the four move; which ones depends on SEED |
| 1, 5, 6 | 1, 4, 6 or 5, 6, 16 | 1 and 5 together would be a run ending on 6, a weak step |

The anticipations are tried in an order that comes from the seed; SYNC takes the first share of those allowed, so
raising SYNC only adds anticipations and lowering it removes them in reverse.

What SYNC keeps:

- **The count of trigs.** SYNC never adds or removes a trig.
- **Whole steps.** On SRC = TRACK the step moves with all its parameter locks, as with ROT.
- **Swing** stays with the position.

Moving a step with parameter locks needs room in the pattern's lock rows; when there is none, YES says NO LOCK ROWS
LEFT and writes nothing.

## MASK and MTRK

| MASK | Keeps |
|---|---|
| OFF | every trig |
| AVOID | the trigs where MTRK has no trig |
| ONLY | the trigs where MTRK has a trig |

MASK works with every source, SRC = TRACK included. It runs after ROT and SYNC, so it holds on the trigs' final
positions: with AVOID, no trig of the take sits on a step where MTRK has a trig, whatever ROT does.

- **It only removes.** A trig MASK keeps is unchanged. A trig it removes is cleared as SKIP clears it: no trig, the
  step's fields and locks back to their defaults. Steps without a trig are not touched.
- **AVOID and ONLY split the track:** with the same recipe, the trigs AVOID keeps and the trigs ONLY keeps are all
  the unmasked trigs, each once.
- **Lengths:** a shorter MTRK repeats, a longer one gives its first steps. A track that is not in the pattern has no
  trigs (AVOID keeps everything, ONLY nothing). A muted trig counts; a track in stock's Euclidean mode counts as its
  Euclidean mode plays, as for RHYTHM's [COPY](../rhythm/#copy).
- **MTRK = the track itself** masks against the track's original trigs: ROT with ONLY keeps the rotated trigs that
  land where the track already had one.
- MASK reads the other track as it is now; the strip follows it.

On a 16-step track with MTRK = `x...x...x...x...`:

| Before MASK | AVOID | ONLY |
|---|---|---|
| `x.x.x.x.xxxx....` | `..x...x..xxx....` | `x...x...x.......` |

## SKIP

The first SKIP steps of the track are cleared and get no trig: room for a kick on another track, or a pickup. SKIP
counts from the track's first step, also under PHRS. No trig is moved into or out of the skipped steps by SYNC.

## FLD, FROT, FMOD: one field against the trigs

FLD picks a field; FROT moves its values that many trigs or steps (negative is earlier) while the trigs stay where
they are. It runs last, after every page has written its fields, so it moves the take's values: for example a
velocity accent pattern shifted against the rhythm.

| FMOD | Moves |
|---|---|
| TRIG | from trig to trig: each trig takes the value of the trig FROT trigs away; nothing is lost |
| GRID | by steps: a value landing on a step without a trig is dropped |

| FLD | Field |
|---|---|
| VEL, NOTE, LEN | velocity, note, length |
| UTIM | micro timing |
| COND | trig condition |
| RTRG | retrig |
| ACC | the accent flag |
| LANE1, LANE2 | the lock of LOCK's lane 1 or lane 2 |

To rotate every track of the pattern at once, use TOOLS' [ROT ALL](../tools/#rot-all).
