---
title: Rotating (PLACE, TOOLS)
sidebar: { order: 4 }
---

Three rotations, all with wrap-around at the track's length.

## PLACE ROT: whole steps

PLACE's **ROT** (knob A, −63…+63) moves the take's whole steps, parameter locks included; the swing bits stay with the positions. With SRC = TRACK it rotates the track's own trigs, as 0.3's ROTATE page did.

## PLACE FLD: one field against the trigs

![PLACE](../../../assets/screens/place.png)

| Knob | Name | Range | What |
|---|---|---|---|
| F | FLD | OFF, VEL, NOTE, LEN, UTIM, COND, RTRG, ACC, LANE1, LANE2 | the field; the trigs stay put |
| G | FROT | −63…+63 | how far; negative is earlier |
| H | FMOD | TRIG, GRID | TRIG: the field moves from trig to trig (nothing is lost); GRID: by steps (a value landing on a step without a trig is dropped) |

The field rotation runs last, after every page has written its fields, so it moves the values of the take.

## TOOLS: ROTATE ALL

![TOOLS](../../../assets/screens/tools.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | TOOL | ROT ALL, YOLO | ROT ALL here; YOLO rolls a recipe for one track ([YOLO](../yolo/)) |
| B | STEPS | −63…+63 | how far |

**YES** on TOOLS rotates every track of the pattern through the OS's own trig shift (tracks in Euclid mode are skipped). It is separate from the recipes, and follows the same generate-and-apply rules: it needs RECORD, **FUNC + YES** applies, **NO** or leaving puts every track back.
