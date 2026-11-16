# Meter, rank and roles

trig-seedr ranks every step of a bar by how strongly it belongs to the meter, using Clarence Barlow's
*indispensability* ("Two Essays on Theory", Computer Music Journal 11(1), 1987). The rank decides where BIAS
places trigs, how the velocity shapes weigh steps, and which trigs AMOD = TOP accents.

## TAKE: METR and BEAT

| Knob | Values | Meaning |
|---|---|---|
| METR | 4/4, 3/4, 6/8, 12/16, 5/8, 7/8 | the meter of the track |
| BEAT | 1..16 | steps per note value of the meter: a quarter in 4/4 and 3/4, an eighth in 6/8, 5/8 and 7/8, a sixteenth in 12/16 |

A bar is the meter's count times BEAT steps:

| METR | Usual BEAT | Bar | Grouping |
|---|---|---|---|
| 4/4 | 4 | 16 steps | 2 × 2 quarters |
| 3/4 | 4 | 12 steps | 3 quarters |
| 6/8 | 2 | 12 steps | 2 dotted quarters of 3 eighths |
| 12/16 | 1 | 12 steps | 4 dotted eighths of 3 sixteenths |
| 5/8 | 2 | 10 steps | 5 eighths (Barlow's order: 2 + 3) |
| 7/8 | 2 | 14 steps | 7 eighths (Barlow's order: 2 + 2 + 3) |

METR and BEAT belong to the track. A track's first recipe takes them from the recipe used last, so a pattern in
6/8 stays in 6/8 as you move between tracks.

## The rank

In 4/4 with BEAT 4 the sixteen steps rank, strongest first:

```
step   1  9 13  5 15  7 11  3 16  8 12  4 14  6 10  2
```

The bar's first step, then the half bar, beat 4, beat 2, then the "&"s, the pickup before a beat ahead of the
step after it. A track of several bars uses the same order in each bar; between equal steps of different bars,
bar 1 comes first, then 3, 4 and 2.

## RHYTHM: BIAS and ROLE (SRC = RAND)

BIAS mixes the rank with chance: at 0 every step is equally likely, at +100 DENS fills the steps in rank order,
at −100 in reverse rank order. DENS still gives its exact count.

ROLE picks the rank order. It is read only through BIAS, so the RHYTHM page shows it while BIAS is not 0:

| ROLE | Order (4/4, BEAT 4: the first steps it fills) |
|---|---|
| METER | the bar's ranks as above: 1, 9, 13, 5 |
| KICK | the bar, the half bar, then the "&"s and sixteenths, beats 2 and 4 last: 1, 9, 15, 7 |
| BACK | beats 2 and 4, then sixteenths as ghost notes, the bar's first step last: 13, 5, 16, 8 |
| HAT | the "&"s, then the beats, then sixteenths: 15, 7, 11, 3 |
| PERC | the sixteenths, pickups first: 16, 8, 12, 4 |

Each role works the same way in every meter: in 6/8 BACK starts on the second dotted quarter, HAT on the eighths
between them.

## DYNAMICS

- **STRONG, WEAK, OFF, NOT 1, NOTBAR** follow the meter: the bar's first step, a grouping above the beat, a beat,
  the beat's first division, and finer steps. In 4/4 this is the same as in 0.3.
- **SHAPE = RANK** sets velocity by rank: the bar's strongest step at the top of VEL, its weakest at the bottom,
  and every other step in between in rank order (16 levels in a 4/4 bar). It always uses the METER order, whatever
  ROLE says.
- **AMOD = TOP** gives the accent flag to the ACC % of trigs with the highest rank (METER order). AMOD is shown
  while ACC is above 0. The seed does
  not change which trigs these are. With SRC = MAP, TOP ranks by the map's level instead (the MAP page).
