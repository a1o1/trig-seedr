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
| A | SRC | TRACK, RAND, EUCLID, MAP, COPY | where the trigs come from |
| B–E | GRID, DENS, BIAS, ROLE | RAND | GRID 1/16…1/2; DENS 0–100 % of grid steps, exactly; BIAS −100…+100 % towards strong or weak steps; ROLE METER, KICK, BACK, HAT, PERC: which steps count as strong ([Meters and roles](../../features/meter/)) |
| B, C | STEPS, PULS | EUCLID | PULS pulses spread over STEPS steps, as stock's Euclidean mode ([EUCLID](../../features/euclid/)) |
| B–F | PART, DENS, X, Y, CHAOS | MAP | a map of drum patterns: PART 1–3, DENS 0–255, X and Y 0–255 (shared by the pattern's tracks), CHAOS 0–255 ([MAP](../../features/map/)) |
| B | TRK | COPY | the track whose trigs are taken ([COPY and MASK](../../features/copy-mask/)) |

With SRC = TRACK the track's trigs keep everything they carry, parameter locks included; the later pages change only what they are set to change.

## PLACE

![PLACE](../../../assets/screens/place.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | ROT | −63…+63 | rotate whole steps (with their locks) by this many steps; swing stays with the position |
| B | SYNC | 0–100 % | the share of possible anticipations made: a trig moves onto the weaker step before it ([SYNC](../../features/sync/)) |
| C, D | MASK, MTRK | OFF, AVOID, ONLY; a track | keep only the trigs that avoid, or only those that meet, another track's trigs ([COPY and MASK](../../features/copy-mask/)) |
| E | SKIP | 0–64 | the first SKIP steps are left empty |
| F, G, H | FLD, FROT, FMOD | | rotate one field against the trigs: [Rotating](../rotate/) |

SKIP and the velocity shapes refer to where steps end up after ROT.

## DYNAMICS

![DYNAMICS](../../../assets/screens/dynamics.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | VEL | DEF, 1–127 | a random velocity in the range; DEF: the trig's own velocity (a new trig: the track's default) |
| B | SHAPE | FLAT, STRONG, WEAK, OFF, NOT 1, NOTBAR, RANK, NOISE, EVRY, MAP | which steps play loudest: by metric weight (STRONG … NOTBAR), by the meter's full rank (RANK), a slow contour (NOISE), every N-th trig (EVRY), the map's level (MAP) |
| C | AMT | 0–100 % | how far velocities follow the shape (with VEL = DEF the shape runs from half the trig's velocity up to it) |
| D | N | 1–64 | EVRY: every N-th trig; NOISE: steps per contour cell |
| E | ACC | 0–100 % | the share of trigs with the accent flag; at 0 the flags are left as they are |
| F | AMOD | RAND, TOP | RAND: random trigs; TOP: the strongest steps (the meter's rank; with MAP the map's own accents) |
| G | HUM | 0–100 | a random velocity spread per step, up to ±32 at 100 |

## NOTE

![NOTE](../../../assets/screens/note.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | LEN | DEF, 0–127 | the length code (14 = 1/16, 127 = INF), random in the range |
| B | LMOD | RANGE, LEGATO | LEGATO: each note lasts until the next trig ([Value lists and LEGATO](../../features/lists/)) |
| C | NOTE | DEF, −24…+24 | semitones from the track's default note |
| D | NSRC | RANGE, LIST, NOISE | the notes: random in the range, a list played in an order, or a noise contour ([Value lists](../../features/lists/), [Noise](../../features/noise/)) |
| E, F | ORDR, NLEN | UP, DOWN, PING, PNG2, ODD, RAND; 1–16 | LIST: the order and the list's length |
| G | ZOOM | 1–64 | NOISE: steps per contour cell |

## LOCK

![LOCK](../../../assets/screens/lock.png)

Two lanes, each a parameter lock on a parameter of your choice; **UP / DOWN** picks the lane.

| Knob | Name | Range | What |
|---|---|---|---|
| A | PARM | SMP, TUN, FIN, … | the parameter: the SMPL, FLTR and AMP pages and the LFO's speed, fade, phase and depth. Lane 1 starts on SMP (the sample slot), lane 2 on FRQ (filter frequency) |
| B | % | 0–100 % | the share of trigs that get a lock; at 0 the lane is left as it is. Above 0 the lane is cleared on every step and the chosen trigs get a value |
| C | RNGE | 0–127 | the values' range (SMP: the slot, 0 = OFF) |
| D | SRC | RANGE, LIST, NOISE | the values: random, a list in an order, or a noise contour |
| E, F, G | ORDR, LLEN, ZOOM | | LIST's order and length; NOISE's steps per cell |

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
