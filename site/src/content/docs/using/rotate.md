---
title: Rotate
sidebar: { order: 4 }
---

ROTATE moves step data with wrap-around at each track's length — mainly to move a groove against the rhythm, and to shift a whole pattern, which the stock OS can't do.

![ROTATE](../../../assets/screens/rotate.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | STEPS | −63…+63 | how far; negative is earlier |
| B | SCOPE | TRACK, ALL | this track, or every track of the pattern (tracks in Euclid mode are skipped) |
| C | WHAT | ALL, VEL, NOTE, LEN, UTIM, COND, RTRG, ACC | ALL moves whole steps through the OS's own trig shift: fields and parameter locks move, swing stays. Anything else moves that field while the trigs stay put |
| D | MODE | TRIG, GRID | a field moves from trig to trig (nothing lost), or by steps (values landing on empty steps are dropped) |

With SCOPE ALL the audition covers every track: NO puts them all back.
