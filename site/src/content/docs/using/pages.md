---
title: Keys and pages
sidebar: { order: 1 }
---

Trig Seedr is a third page behind the **TRIG** key, after the Euclidean page:

**TRIG** (stock's TRIG page) → **TRIG** (EUCLIDEAN) → **TRIG** (Trig Seedr) → **TRIG** (back to stock's TRIG page)

**TRIG held + PAGE** opens it from any page. A parameter page key (SRC, SMPL, FLTR, AMP, LFO, FX) or a menu key closes it. **PAGE** steps through its pages:

**VIEW → GEN 1 → GEN 2 → GEN 3 → METER → GROOVE → ROTATE → VIEW**

| Key | What it does |
|---|---|
| TRIG on the EUCLIDEAN page | open |
| TRIG on Trig Seedr | close (stock's TRIG page) |
| TRIG held + PAGE | open, from anywhere |
| PAGE | the next page |
| knobs A–H | the page's settings |
| FUNC + knob | the low end of a range (VEL, LEN, NOTE, RATE, SMPL); SEED in steps of 100 |
| YES | apply: generate, groove or rotate |
| NO | while auditioning: back to the original; otherwise: close |
| FUNC + YES | keep the take |
| LEFT / RIGHT, UP / DOWN | VIEW: move the cursor a step / 16 steps |

## The audition

- The **first YES** copies the track (ROTATE ALL: every track) and applies.
- **Every YES after it** starts again from that copy: you hear takes, not changes on top of changes.
- **YES with no knob turned** since the page's last YES moves to the **next seed**: a new take. After a knob turn, YES keeps the seed, so you hear just that change.
- **NO** restores the original. **FUNC + YES** keeps the take; so does leaving the page, the track or the pattern.
- YES on another page keeps the current take first. Generate, then groove, then NO: only the groove is undone.

## VIEW

![VIEW](../../../assets/screens/view.png)

The track's steps (filled: a note trig; small: a lock trig; a bar under a step: accent; a dot above: retrig) and, for the step under the caret:

- `N V L U`: note, velocity, length code, micro timing (`-` = the track default).
- `C R`: trig condition, retrig (rate and velocity curve).
- `F0381`: the step's flag word in hex (0x0001 trig, 0x0002 retrig, 0x0004 mute, 0x0008 accent, 0x0010 swing, 0x0020 slide, 0x0080 / 0x0100 / 0x0200 SYN / SMP / ENV on); `DN` / `DV`: the track's default note and velocity.
