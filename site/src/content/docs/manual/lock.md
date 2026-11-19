---
title: LOCK
description: "Two lanes of parameter locks, each on a parameter you choose, with values from a range, a list or a noise contour."
sidebar: { order: 7 }
---

LOCK writes parameter locks: two lanes, each on a parameter you choose. **UP / DOWN** picks the lane in view (lane 1
or lane 2); the knobs show that lane's settings.

On the Syntakt (in development), the lanes follow the track's machine: see [The Syntakt](../syntakt/#lock).

![LOCK, lane 1 on the sample slot with a noise contour](../../../assets/screens/lock.png)

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | PARM | the lockable parameters (below) | lane 1 SMP, lane 2 FRQ | always |
| B | % | 0–100 % | 0 % | always |
| C | RNGE | a range inside the parameter's values (SMP: the slot, 0 = OFF) | 1–8 | always |
| D | SRC | RANGE, LIST, NOISE | RANGE | always |
| E | ORDR | UP, DOWN, PING, PNG2, ODD, RAND | UP | while SRC is LIST |
| F | LLEN | 1–16 | 4 | while SRC is LIST |
| G | ZOOM | 1–64 | 4 | while SRC is NOISE |

## The lanes

- **%** is the share of trigs that get a lock on the lane's parameter. At 0 % the lane is left as it is: the
  track's locks on that parameter stay. Above 0 % the lane owns that parameter: its locks are cleared on every step
  and the chosen trigs get a value. % is a share knob: raising it keeps the locks already chosen and adds more.
- **RNGE** is a range knob (the knob the high end, FUNC + knob the low end). It sets where the values lie, in the
  parameter's own units (0–127).
- **SRC** picks the values inside RNGE: random (RANGE), a list in an order (LIST) or a contour (NOISE). They work as
  on NOTE: see [LIST](../note/#list-values-in-an-order) and [NOISE](../note/#noise-a-contour-over-the-steps). On a
  lane, the list position counts every trig, locked or not, so raising % adds locks without changing the values
  already there. Each lane draws its own list and contour.
- **PARM never lands on the other lane's parameter.** The Rytm keeps one lock per parameter and step, so two lanes
  on one parameter could not both be stored; turning PARM skips the parameter the other lane is on.

## The parameters

| Page | Parameters |
|---|---|
| SMPL | TUN, FIN, BR, SMP (the sample slot), STA, END, SLEV |
| FLTR | FATK, FDEC, FSUS, FREL, FENV, FRQ, RES |
| AMP | AATK, AHLD, ADEC, OVR, DEL, REV, PAN, VOL |
| LFO | LSPD, LFAD, LSPH, LDEP |

Not offered yet: the SYNTH page (a different parameter per machine) and the stepped settings (the filter type, the
LFO's multiplier, waveform, mode and destination). With samples loaded into the project, a lane on SMP gives trigs
sample locks; an emulator card has no samples until you load some ([Trying it in the emulator](../../getting-started/emulator/)).

## Lock rows

The Rytm stores up to 72 lock rows per pattern; a parameter locked on a track takes one row. A take that would need
a row the pattern does not have is refused before anything is written: the strip says **NO LOCK ROWS LEFT**. Free
rows by clearing locks on another track, or use a parameter the track already has locks on.
