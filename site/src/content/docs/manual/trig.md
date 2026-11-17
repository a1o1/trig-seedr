---
title: TRIG
description: "Trig conditions (COND, CTYP, CVAL) and retrigs (RTRG, RATE, RCUR)."
sidebar: { order: 8 }
---

TRIG writes the Rytm's trig conditions and retrigs.

![TRIG](../../../assets/screens/trig.png)

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | COND | 0–100 % | 0 % | always |
| B | CTYP | X%, A:B, 1ST, FILL, MIX | X% | while COND is above 0 |
| C | CVAL | X% and MIX: a range of probabilities, 1 % … 100 %; A:B: the cycle, /2 … /8 | 50 % … 100 %; /8 | while COND is above 0 and CTYP is X%, A:B or MIX |
| D | RTRG | 0–100 % | 0 % | always |
| E | RATE | a range of retrig rates, 1/1 … 1/80 | 1/16 | while RTRG is above 0 |
| F | RCUR | −128…+127 | 0 | while RTRG is above 0 |

## Conditions: COND, CTYP, CVAL

**COND** is the share of trigs that get a trig condition. At 0 % conditions are left as they are. Above 0 %, TRIG
owns the conditions: the chosen trigs get one and every other trig has none. COND is a share knob: raising it keeps
the trigs already chosen and adds more.

| CTYP | The chosen trigs get |
|---|---|
| X% | a probability, random in CVAL's range (the Rytm's steps: 1, 3, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99, 100 %) |
| A:B | A:B with B = CVAL and A random from 1 to B |
| 1ST | 1ST |
| FILL | FILL |
| MIX | one of the four above, at random per trig |

CVAL is a range knob for X% (the knob the high end, FUNC + knob the low end) and a single value for A:B. With MIX,
CVAL shows the X% range; the A:B conditions MIX writes take their B from CVAL's low end, counted as a position from
2 to 8 (at the default, B is 8).

TAKE's [CVAR](../take/#cvar-variation-as-trig-conditions) also writes conditions, to alternate two takes.

## Retrigs: RTRG, RATE, RCUR

**RTRG** is the share of trigs with a retrig. At 0 % retrigs are left as they are. Above 0 %, TRIG owns the retrig
switch: the chosen trigs get a retrig and every other trig loses its own (its retrig settings stay, switched off).

- **RATE** is a range of the Rytm's retrig rates: 1/1, 1/2, 1/3, 1/4, 1/5, 1/6, 1/8, 1/10, 1/12, 1/16, 1/20, 1/24,
  1/32, 1/40, 1/48, 1/64, 1/80. Each chosen trig gets a random rate in the range. A range shows as `1/16-32`.
- **RCUR** is the retrig velocity curve, as stock's retrig VEL: negative fades the repeats out, positive in.
