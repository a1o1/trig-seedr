---
title: Generator (GEN 1–3)
sidebar: { order: 2 }
---

GENERATOR replaces the track's steps, over the track's current length, with a new take. Other tracks are not touched.

## GEN 1: rhythm and notes

![GEN 1](../../../assets/screens/gen1.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | GRID | 1/16, 1/8, 1/4, 1/2 | trigs only land on the grid |
| B | DENS | 0–100 % | share of grid positions that get a trig (an exact count) |
| C | VEL | 1–127 | velocity range |
| D | LEN | 1/128 … 8/1, INF | note length range; DEF leaves the track default. The bottom line shows the range while you turn it |
| E | NOTE | −24…+24 | offset from the track's default note |
| F | UTIM | 0–100 % | random micro timing, up to ±23/24 of a step |
| G | SEED | FREE, 0–9999 | which take; FREE picks a new one every time |
| H | ROT | −N…+N | rotate the result before writing |

## GEN 2: Elektron extras

![GEN 2](../../../assets/screens/gen2.png)

| Knob | Name | What |
|---|---|---|
| A | ACC | share of trigs with an accent |
| B | COND | share of trigs with a trig condition |
| C | CTYP | which conditions: X % (probability), A:B, 1ST, FILL, MIX |
| D | CVAL | X %: the probability range; A:B: the cycle (2–8) |
| E | RTRG | share of trigs with a retrig |
| F | RATE | retrig rate range (1/1 … 1/80) |
| G | RCUR | retrig velocity curve: negative fades the repeats out, positive in |

## GEN 3: sample locks

![GEN 3](../../../assets/screens/gen3.png)

| Knob | Name | What |
|---|---|---|
| A | SMP% | share of trigs that get a sample-slot lock. At 0 the track's sample locks are left alone; above 0 GENERATOR replaces them |
| B | SMPL | the slot range a locked trig picks from (0 = OFF) |

Sample locks use one of the pattern's 72 lock rows per track, however many trigs carry them. If none is free, GENERATOR shows **NO LOCK ROWS LEFT** and writes nothing: stock would otherwise keep the new lock only until the next save.
