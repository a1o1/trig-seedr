---
title: DYNAMICS
description: "Velocity (VEL), velocity shapes by the meter, rank, noise, every N-th trig or the map (SHAPE, AMT, N), the accent flag (ACC, AMOD) and a random spread (HUM)."
sidebar: { order: 5 }
---

DYNAMICS sets each trig's velocity and the Rytm's accent flag, at the trigs' final positions (after PLACE).

![DYNAMICS](../../../assets/screens/dynamics.png)

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | VEL | DEF, or a range 1–127 | DEF | always |
| B | SHAPE | FLAT, STRONG, WEAK, OFF, NOT 1, NOTBAR, RANK, NOISE, EVRY, MAP | FLAT | always |
| C | AMT | 0–100 % | 0 % | while SHAPE is not FLAT |
| D | N | 1–64 | 4 | while SHAPE is EVRY or NOISE |
| E | ACC | 0–100 % | 0 % | always |
| F | AMOD | RAND, TOP | RAND | while ACC is above 0 |
| G | HUM | 0–100 | 0 | always |

## Velocity: VEL, SHAPE, AMT, HUM

A trig's velocity is worked out in three steps:

1. **VEL.** A random value in VEL's range. At DEF the trig keeps its own velocity (a new trig: the track's default).
2. **SHAPE and AMT.** The shape gives each trig a level from quiet to loud, placed in VEL's range: the quietest at the
   low end, the loudest at the high end. With VEL at DEF the shape runs from half the trig's velocity up to its
   velocity. AMT is how far the velocity moves from step 1's value towards the shape's: 0 % not at all, 100 % all
   the way.
3. **HUM.** A random spread on every trig, up to ±32 at 100.

VEL is a range knob: the knob sets the high end, FUNC + knob the low end ([Range knobs](../concepts/#range-knobs)).

| SHAPE | Loudest | Quietest |
|---|---|---|
| FLAT | no shape (AMT is hidden) | |
| STRONG | the bar's first step, then the half bar, the beats, the "&"s | the sixteenths |
| WEAK | the reverse of STRONG | the bar's first step |
| OFF | the "&"s | the beats and stronger; the sixteenths sit in between |
| NOT 1 | everything off the beat | the beats and stronger |
| NOTBAR | every step but the bar's first | the bar's first step |
| RANK | the bar's strongest step, every step in between in rank order (16 levels in a 4/4 bar) | the bar's weakest step |
| NOISE | a slow contour that rises and falls; N steps from one random level to the next | |
| EVRY | every N-th trig, counted from the track's first trig | the others |
| MAP | the map's level at each trig (below) | |

STRONG to NOTBAR and RANK follow TAKE's [METR and BEAT](../take/#metr-and-beat-the-meter); RANK always uses the
METER order, whatever RHYTHM's ROLE says. NOISE works as NOTE's [noise contour](../note/#noise-a-contour-over-the-steps),
with N as its ZOOM.

## The accent flag: ACC and AMOD

ACC is the share of trigs that get the Rytm's accent flag. At 0 % the flags are left as they are. Above 0 %, DYNAMICS
owns the flag on every trig: the chosen trigs get it and every other trig loses it. ACC is a share knob: raising it
keeps the accents already chosen and adds more.

| AMOD | Accents |
|---|---|
| RAND | trigs chosen at random (from the seed) |
| TOP | the trigs on the strongest steps of the meter (METER order). The seed does not change which. With SRC = MAP, the trigs with the highest map level (below) |

## MAP: velocity and accents from the map

These use the map of [RHYTHM's MAP source](../rhythm/#map), at the same X and Y and the track's PART.

**SHAPE = MAP** sets each trig's level from the map's level at its step: the map's level 0 is VEL's low end, 255
its high end. It works with any source, so the map's levels can shape a rhythm from RAND, EUCLID or the track's own
trigs. RHYTHM shows PART, X and Y only while SRC = MAP: with another source, SHAPE = MAP reads the values they were
last set to there. This is as built in 0.7.

**ACC with AMOD = TOP on a MAP track** ranks the trigs by their map level, CHAOS lift included, and accents the
ACC % with the highest levels (equal levels: the earlier step first). The original drum sequencer accents a step
whose level, with the lift, is above 192. Those trigs are always the top ones, so the ACC % that counts them writes
exactly the original's accents; more ACC accents the next strongest trigs.

Example: PART 1, DENS 200, X 60, Y 190, CHAOS 0 on a 16-step track gives 5 trigs (steps 1, 5, 7, 13, 15), 2 of them
above 192 (steps 1 and 5). ACC 30 % accents those 2 (30 % of 5 trigs, rounded).

Both read the level at each trig's final position: after ROT or SYNC a trig takes the level of the step it lands on.
