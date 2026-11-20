---
title: RHYTHM
description: "Where the trigs come from: the track's own (TRACK), random with roles (RAND), Euclidean (EUCLID), a map of drum patterns (MAP), or another track (COPY)."
sidebar: { order: 3 }
---

RHYTHM decides which steps have trigs. It is the only page that makes trigs; every later page shapes or dresses
the trigs it gives. **SRC** (knob A) picks the source, and the other knobs are the source's own.

| SRC | Title bar | Knobs | Trigs |
|---|---|---|---|
| TRACK | `TRK` | none | the trigs that are on the track, with everything they carry |
| RAND | `S` + seed | GRID, DENS, BIAS, ROLE | an exact share of the grid's steps, chosen by chance and the meter |
| EUCLID | `EUC` | STEPS, PULS | pulses spread evenly over a cycle, as stock's Euclidean mode |
| MAP | `MAP`, or `S` + seed from CHAOS 4 | PART, DENS, X, Y, CHAOS | a rhythm read from a map of drum patterns |
| COPY | `CPY` | TRK | the trig positions of another track |

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | SRC | TRACK, RAND, EUCLID, MAP, COPY, TURG | TRACK on a track with trigs, RAND on an empty one | always |

A new recipe starts on TRACK when the track has trigs and on RAND when it is empty. Each source's settings are kept
while another source is chosen.

A trig a source makes (every source but TRACK) starts with the track's defaults: note, velocity, length, no
condition, no locks, and SYN, SMP and ENV on, as a trig from the TRIG keys. The later pages fill its fields.

## TRACK

The trigs on the track as they are, parameter locks and all. The later pages change only what they are set to
change: with every page at 0, YES leaves the track byte for byte as it was. Use it to dress a beat you programmed:
accents, conditions, a groove, sample locks.

## RAND

![RHYTHM with SRC = RAND](../../../assets/screens/random.png)

| Knob | Name | Values | Default | Shown | Does |
|---|---|---|---|---|---|
| B | GRID | 1/16, 1/8, 1/4, 1/2 | 1/16 | with SRC = RAND | trigs land only on multiples of the grid |
| C | DENS | 0–100 % | 50 % | with SRC = RAND | the share of the grid's steps that get a trig, exactly (rounded) |
| D | BIAS | −100…+100 % | 0 % | with SRC = RAND | towards the strong steps (+) or the weak steps (−) of the meter |
| E | ROLE | METER, KICK, BACK, HAT, PERC | METER | with SRC = RAND, while BIAS is not 0 | which steps count as strong |

DENS is a share knob: raising it keeps the trigs that were there and adds more, lowering it removes them in
reverse. BIAS mixes the meter's rank with chance: at 0 every step is equally likely, at +100 DENS fills the steps in
rank order, at −100 in reverse rank order. DENS still gives its exact count.

ROLE picks the rank order BIAS follows. The meter is TAKE's METR and BEAT ([the rank](../take/#metr-and-beat-the-meter)).

| ROLE | 4/4, BEAT 4: the steps it fills first |
|---|---|
| METER | the bar's ranks: 1, 9, 13, 5 |
| KICK | the bar, the half bar, then the "&"s and sixteenths, beats 2 and 4 last: 1, 9, 15, 7 |
| BACK | beats 2 and 4, then sixteenths as ghost notes, the bar's first step last: 13, 5, 16, 8 |
| HAT | the "&"s, then the beats, then the sixteenths: 15, 7, 11, 3 |
| PERC | the sixteenths, pickups first: 16, 8, 12, 4 |

Each role works the same way in every meter: in 6/8 BACK starts on the second dotted quarter and HAT on the eighths
between them. These orders are as built in 0.7; beta feedback may change them before 1.0.

## EUCLID

![RHYTHM with SRC = EUCLID: 5 pulses over 13 steps](../../../assets/screens/euclid.png)

| Knob | Name | Values | Default | Shown | Does |
|---|---|---|---|---|---|
| B | STEPS | 1–64 | 16 | with SRC = EUCLID | the cycle's length in steps |
| C | PULS | 0–64 | 4 | with SRC = EUCLID | pulses in each cycle; above STEPS, every step has a trig |

The pattern is the one the Rytm's own Euclidean mode plays (the EUCLIDEAN page) with LEN = STEPS, PL1 = PULS, PL2 at
0 and no rotation. The cycle's first step always has a pulse, and the gaps between pulses differ by at most one
step.

| STEPS | PULS | Pattern |
|---|---|---|
| 16 | 4 | `x...x...x...x...` |
| 16 | 5 | `x...x..x..x..x..` |
| 16 | 7 | `x..x.x.x..x.x.x.` |
| 8 | 3 | `x..x..x.` |
| 13 | 5 | `x..x..x.x..x.` |

A track longer than STEPS repeats the cycle (STEPS 12 on 64 steps plays it five times, then its first four steps);
a shorter track plays the cycle's start. Use PLACE ROT to rotate it.

EUCLID draws no random numbers: SEED and VARY do not change the rhythm, only the random values later pages draw.

Differences from stock's Euclidean mode:

- Trig Seedr writes ordinary trigs. The track is not in Euclidean mode, and the trigs can be edited by hand.
- Stock's second generator (PL2, RO2) and its operator are not part of EUCLID.
- STEPS is independent of the track's length.
- Trig Seedr does not write to a track that is in Euclidean mode (EUCLID IS ON: TURN IT OFF).

## MAP

![RHYTHM with SRC = MAP](../../../assets/screens/rhythm.png)

SRC = MAP reads a rhythm from a map of drum patterns. The map and the way it is read come from an open source drum
sequencer, credited in the [acknowledgements](../../about/acknowledgements/).

| Knob | Name | Values | Default | Shown | Does |
|---|---|---|---|---|---|
| B | PART | 1–3 | 1 | with SRC = MAP | the map's part: 1 is written for kick drums, 2 for snares, 3 for hats (per track) |
| C | DENS | 0–255 | 128 | with SRC = MAP | how many steps fire: 0 none, 255 every step the part plays at all (per track) |
| D | X | 0–255 | 128 | with SRC = MAP | the place on the map, across (shared by all tracks) |
| E | Y | 0–255 | 128 | with SRC = MAP | the place on the map, down (shared by all tracks) |
| F | CHAOS | 0–255 | 0 | with SRC = MAP | a random lift of the levels, from the seed (shared by all tracks) |

X, Y and CHAOS are shared: turning X on BD's page turns it for SD too. PART and DENS belong to each track, so BD on
part 1, SD on part 2 and a hat on part 3 at the same X and Y make a kit from one place on the map, and DENS thins or
fills each one.

**The map** is a grid of 5 × 5 patterns of 32 steps, each with three parts. Every step of a part has a level from 0
to 255: how strongly that step belongs to the rhythm. X and Y move across the grid; between patterns the four
nearest are blended, so turning X or Y changes the rhythm gradually. A step fires when its level is above
255 − DENS: raising DENS only adds steps, the strongest first. A track longer than 32 steps repeats the 32 (64 steps
plays them twice); a shorter track plays their start.

**CHAOS** lifts every level of a part by a random amount, the same over each 32 steps, up to CHAOS / 4 (62 at
CHAOS 255). A lifted step can fire at a DENS where it did not before, so CHAOS adds steps near the threshold. The
lift comes from the seed: YES with no knob turned takes the next seed and a new lift, and on a 64-step track each
half gets its own. With CHAOS below 4 there is no lift and the seed does not change the rhythm; the title bar then
shows `MAP`, and from CHAOS 4 on `S` and the seed.

The map's levels are also used on DYNAMICS: SHAPE = MAP sets velocity from them, and ACC with AMOD = TOP accents the
same trigs the original sequencer accents ([DYNAMICS](../dynamics/#map-velocity-and-accents-from-the-map)).

Differences from the original sequencer:

- Trig Seedr writes ordinary trigs; the map is read once per YES, not as the pattern plays.
- The original's swing and its Euclidean mode are not part of MAP (TIME's grooves and SRC = EUCLID cover them).
- Under TAKE's PHRS, each page plays the map's first 16 steps ([PHRS](../take/#phrs-a-phrase-over-the-pages)).

## COPY

| Knob | Name | Values | Default | Shown | Does |
|---|---|---|---|---|---|
| B | TRK | the pattern's tracks, BD … FX | BD | with SRC = COPY | the track whose trig positions are taken |

Step i gets a trig when TRK has a trig on step i. Only the positions are taken: TRK's velocities, notes, locks and
conditions are not copied, and the later pages fill the new trigs' fields.

- **Lengths.** A TRK shorter than the track repeats, as it does when the pattern plays (a 12-step TRK on a 16-step
  track gives its 12 steps, then its first 4 again). A longer TRK gives its first steps.
- **TRK = the track itself** gives the track's own trig positions with every field back at its default: a way to
  start again on the same rhythm.
- **Trigs as they play.** A muted trig counts. A track in stock's Euclidean mode gives the pattern its Euclidean
  mode plays, not the trigs stored in it.
- COPY draws no random numbers: SEED and VARY change only the later pages.
- COPY reads the other track as it is now; the strip follows it when you change that track. A take already written
  does not change when the other track changes later; YES renders it again.

## TURG

| Knob | Name | Values | Default | Shown | Does |
|---|---|---|---|---|---|
| B | FLAV | DJVU, CRPT, SHFT | DJVU | with SRC = TURG | how the loop changes from one pass to the next |
| C | LOOP | 1–16 | 8 | with SRC = TURG | the loop's length in steps (SHFT: at least 2) |
| D | AMT | 0–100 | 25 | with SRC = TURG | how much the loop changes each pass; what it does depends on FLAV |
| E | BIAS | 0–100 % | 50 % | with SRC = TURG | the chance a new step of the loop gets a trig |
| F | STRT | SEED, TRACK | SEED | with SRC = TURG | the first pass: random steps from the seed, or the track's own first LOOP steps |

TURG is a Turing machine: a loop of LOOP steps that plays again and again down the track, changing a little each
time round. Each step of the loop holds a trig (on or off) and a value; the value can drive NOTE, VEL and LOCK's
lanes through their value source TURG, so pitch, dynamics and locks change with the rhythm. On a 64-step track with
LOOP 8 you hear eight passes, each drawn from the one before.

![RHYTHM with SRC = TURG: CRPT, a loop of 8, AMT 60](../../../assets/screens/turg.png)

| FLAV | AMT 0 | AMT 50 | AMT 100 |
|---|---|---|---|
| DJVU | every pass new | the loop repeats exactly | the loop's steps play in a shuffled order |
| CRPT | the loop repeats exactly | values change (up to 25 % a pass), trigs stay | values change more and trigs flip, up to half a pass |
| SHFT | the loop inverts each pass, so it repeats every two | a coin flip for each step | the loop repeats exactly |

- **STRT = TRACK** starts from the track's own first LOOP steps, so the machine evolves your pattern.
- The machine is seeded as every source is: the same SEED gives the same take, VARY works as on RAND.
- With SRC set to another source, a value source TURG still uses the machine as set here.
- The live [TURING page](../turing/) changes the loop while the pattern plays.
