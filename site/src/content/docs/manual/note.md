---
title: NOTE
description: "Note length (LEN, LEGATO) and note (NOTE) from a range, a list in an order (LIST) or a noise contour (NOISE)."
sidebar: { order: 6 }
---

NOTE sets each trig's length and note. Notes come from one of three value sources: a random value in a range
(RANGE), a short list played in an order (LIST), or a slow contour (NOISE). LOCK's lanes use the same three sources.

![NOTE with a list](../../../assets/screens/note.png)

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | LEN | DEF, or a range of lengths 1/128 … INF | DEF | always |
| B | LMOD | RANGE, LEGATO | RANGE | always |
| C | NOTE | DEF, or a range −24…+24 semitones | DEF | always |
| D | NSRC | RANGE, LIST, NOISE, TURG | RANGE | always |
| E | ORDR | UP, DOWN, PING, PNG2, ODD, RAND | UP | while NSRC is LIST |
| F | NLEN | 1–16 | 4 | while NSRC is LIST |
| G | ZOOM | 1–64 | 4 | while NSRC is NOISE |
| H | SCALE | OFF, CHR, MAJ, MIN, DOR, PHR, LYD, MIX, LOC, HMIN, MMIN, PMAJ, PMIN, WHOLE, DIM, HIRA, 7SUS4, MAJ7, MIN7; FUNC + knob: ROOT C…B | OFF | always |

## LEN and LMOD

**LEN** is a range of the Rytm's note lengths, shown as note values (1/16, 1/8, 1/4 …) where they are one and in
steps otherwise (1.25), up to INF. Each trig gets a random length in the range. At DEF lengths are left as they are;
the first turn up starts at 1/16. LEN is a range knob: the knob sets the high end, FUNC + knob the low end.

**LMOD = LEGATO** makes each trig last until the next trig: it gets the shortest of the Rytm's lengths that reaches
it (1 step is 1/16, 2 steps 1/8, 4 steps 1/4). The last trig reaches the first one, a loop later; a lone trig lasts
the whole track. A gap of an odd number of steps above 32 has no exact length and gets the next one up (33 steps:
34). Micro timing is not taken into account. LEGATO measures the gaps where the trigs end up after PLACE, and LEN is
not used while it is on.

## NOTE and NSRC

**NOTE** is a range of semitones from the track's default note, −24 to +24. At DEF notes are left as they are,
whatever NSRC says; the first turn up starts at +0. NOTE is a range knob.

**NSRC** picks where the notes in that range come from:

| NSRC | Each trig gets |
|---|---|
| RANGE | a random note in the range |
| LIST | the next value of a short list, in the order ORDR says |
| NOISE | the value of a slow contour at its step |

## LIST: values in an order

A list gives the trigs a short sequence of values in a fixed order, the way an arpeggiator walks its notes.

**The list.** NLEN values are drawn from the range, then sorted from low to high. The seed decides the values; with
VARY above 0 the next take keeps some of them and draws the others again. A list of length 1 gives every trig the
same value; a range narrower than the list gives repeated values.

**The order.** The trigs take the list's values one after another, from the track's first trig to its last; at the
end of the list it starts again. With the sorted list numbered 1 to N:

| ORDR | Trigs get (N = 4) | Repeats every |
|---|---|---|
| UP | 1 2 3 4 1 2 3 4 … | N trigs |
| DOWN | 4 3 2 1 4 3 2 1 … | N trigs |
| PING | 1 2 3 4 3 2 1 2 … (the ends once) | 2N − 2 trigs |
| PNG2 | 1 2 3 4 4 3 2 1 1 … (the ends twice) | 2N trigs |
| ODD | 1 3 2 4 1 3 … (odd places, then even; N = 5: 1 3 5 2 4) | N trigs |
| RAND | a value from the list at random on each trig | |

These orders are as built in 0.7; beta feedback may change them before 1.0.

## NOISE: a contour over the steps

NOISE gives a value that changes slowly from step to step instead of jumping at random. Every ZOOM steps there is a
random level, and the steps between move from one level to the next along an S-curve. The result is a contour: a
note line that drifts up and down, a filter or sample lock that sweeps, a velocity that swells and falls (DYNAMICS'
SHAPE = NOISE, with N as its ZOOM).

| ZOOM | Result |
|---|---|
| 1 | a new random level on every step: the same spread as RANGE |
| 2–8 | a contour that turns every few steps |
| 16–64 | a slow sweep over a page or the whole track |

- The contour's lowest level is the range's low end and its highest the high end.
- The track's first step is always one of the random levels, and the contour wraps at the track's length: the last
  steps lead back to the first step's level, so the loop has no jump. When the length is not a multiple of ZOOM,
  the last stretch is shorter.
- **The step, not the trig.** A trig's value comes from the contour at its step. Raising DENS or a lane's % adds
  trigs or locks without changing the values of those already there, and removing a trig changes no other trig's
  value.
- **SEED and VARY.** Each random level is one of the take's random numbers: with VARY at k %, the next seed keeps
  about k % of the levels. NOTE, each LOCK lane and DYNAMICS' shape draw different contours from the same take, so
  turning one does not change another.

## TURG and SCALE

**NSRC = TURG** takes each trig's note from the RHYTHM page's Turing machine (SRC = TURG there): the value the
machine holds for that step, spread over NOTE's range. As the loop changes, the melody changes with it.

**SCALE** fits every note NOTE writes to a scale on **ROOT** (FUNC + the SCALE knob turns ROOT, C … B, while a
scale is on): the nearest note of the scale, the lower one on a tie. OFF leaves notes as they are. SCALE works with
every value source, and changes nothing while NOTE is DEF.
