---
title: The layers (RHYTHM to TRIG)
sidebar: { order: 2 }
---

A take is the track's recipe rendered in this order: **RHYTHM** decides which steps have trigs, **PLACE** where they end up, then **DYNAMICS**, **NOTE**, **LOCK**, **TRIG** and **TIME** set the trigs' fields. Each page owns its fields: changing one page's settings changes only those fields. A page at 0 (or DEF) leaves them as they are.

Every share knob (DENS, ACC, COND, RTRG, a lane's %) gives each step a key and takes the steps with the highest keys. Raising the knob adds trigs to the ones already chosen; lowering it removes them in reverse. So turning a share up and down moves through one family of takes.

Ranges: a knob sets the high end, **FUNC** + knob the low end. Turning an end of a DEF range below its minimum goes back to DEF.

## RHYTHM

![RHYTHM](../../../assets/screens/rhythm.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | SRC | TRACK, RAND | TRACK: the trigs on the track; RAND: new trigs from the seed |
| B | GRID | 1/16, 1/8, 1/4, 1/2 | RAND: trigs only on every step, every 2nd, 4th or 8th |
| C | DENS | 0–100 % | RAND: the share of grid steps with a trig, exactly (50 % of 16 grid steps is 8 trigs) |
| D | BIAS | −100…+100 % | RAND: + puts trigs on the strong steps of the beat (the bar's first step, the half bar, the beats, the "&"), − on the weak ones; 0 ignores position |

With SRC = TRACK the track's trigs keep everything they carry, parameter locks included; the later pages change only what they are set to change.

## PLACE

![PLACE](../../../assets/screens/place.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | ROT | −63…+63 | rotate whole steps (with their locks) by this many steps; swing stays with the position |
| E | SKIP | 0–64 | the first SKIP steps are left empty |
| F, G, H | FLD, FROT, FMOD | | rotate one field against the trigs: [Rotating](../rotate/) |

SKIP and the velocity shapes refer to where steps end up after ROT.

## DYNAMICS

![DYNAMICS](../../../assets/screens/dynamics.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | VEL | DEF, 1–127 | a random velocity in the range; DEF: the trig's own velocity (a new trig: the track's default) |
| B | SHAPE | FLAT, STRONG, WEAK, OFF, NOT 1, NOTBAR, EVRY | which steps play loudest. STRONG: by metric weight; WEAK: the reverse; OFF: the "&" loudest, the beats softest; NOT 1: every beat soft; NOTBAR: the bar's first step soft; EVRY: every N-th trig loud |
| C | AMT | 0–100 % | how far velocities follow the shape (with VEL = DEF the shape runs from half the trig's velocity up to it) |
| D | N | 1–64 | EVRY: every N-th trig, counted from the first |
| E | ACC | 0–100 % | the share of trigs with the accent flag; at 0 the flags are left as they are |
| G | HUM | 0–100 | a random velocity spread per step, up to ±32 at 100 |

## NOTE

![NOTE](../../../assets/screens/note.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | LEN | DEF, 0–127 | the length code (14 = 1/16, 127 = INF), random in the range |
| C | NOTE | DEF, −24…+24 | semitones from the track's default note, random in the range |

## LOCK

![LOCK](../../../assets/screens/lock.png)

Two lanes, each a parameter lock on a parameter of your choice; **UP / DOWN** picks the lane.

| Knob | Name | Range | What |
|---|---|---|---|
| A | PARM | SMP, TUN, FIN, … | the parameter: the SMPL, FLTR and AMP pages and the LFO's speed, fade, phase and depth. Lane 1 starts on SMP (the sample slot), lane 2 on FRQ (filter frequency) |
| B | % | 0–100 % | the share of trigs that get a lock; at 0 the lane is left as it is. Above 0 the lane is cleared on every step and the chosen trigs get a value |
| C | RNGE | 0–127 | the values, random in the range (SMP: the slot, 0 = OFF) |

A lock the pattern has no room for (the Rytm keeps 72 lock rows per pattern) is refused before anything is written: the screen says NO LOCK ROWS LEFT.

## TRIG

![TRIG](../../../assets/screens/trig.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | COND | 0–100 % | the share of trigs with a trig condition; at 0 conditions are left as they are |
| B | CTYP | X%, A:B, 1ST, FILL, MIX | the kind of condition (MIX: any of the four) |
| C | CVAL | 1 %…100 %; 2–8 | X%: the probability range; A:B: the cycle length B (A is random) |
| D | RTRG | 0–100 % | the share of trigs with a retrig |
| E | RATE | 1/1…1/80 | the retrig rate range |
| F | RCUR | −128…+127 | the retrig velocity curve |
