---
title: Keys and pages
sidebar: { order: 1 }
---

Trig Seedr is a third page behind the **TRIG** key, after the Euclidean page:

**TRIG** (stock's TRIG page) → **TRIG** (EUCLIDEAN) → **TRIG** (Trig Seedr) → **TRIG** (back to stock's TRIG page)

**TRIG held + PAGE** opens it from any page. A parameter page key (SRC, SMPL, FLTR, AMP, LFO, FX) or a menu key closes it. **LEFT / RIGHT** step through its pages:

**TAKE → RHYTHM → PLACE → DYNAMICS → NOTE → LOCK → TRIG → TIME → VIEW → TOOLS → TAKE**

A page, knob or value whose feature is not built yet is not shown. The title bar shows the page, the track and the page's place among the pages shown (`3/10`), and at the right the source: `TRK` (the track's own trigs) or `S` and the seed, after `AUD` while a take is being auditioned. The place is left out when a long title would not fit.

| Key | What it does |
|---|---|
| TRIG on the EUCLIDEAN page | open |
| TRIG on Trig Seedr | close (stock's TRIG page) |
| TRIG held + PAGE | open, from anywhere |
| LEFT / RIGHT | the previous / next page |
| UP / DOWN | LOCK: lane 1 or 2 |
| knobs A–H | the page's settings |
| FUNC + knob | the low end of a range (VEL, LEN, NOTE, RNGE, CVAL, RATE); SEED in steps of 100; other wide knobs in steps of 10 |
| YES | render the track's recipe (an audition); on TOOLS, run the tool |
| NO | while auditioning: back to the original; otherwise: close |
| FUNC + YES | keep the take |
| PAGE, the TRIG keys, transport | stock's: PAGE still picks which 16 steps the TRIG keys show |

## One recipe per track

Every track has a recipe: the settings of all the pages. **YES renders the whole recipe from the original track**, every time, so YES means the same on every page.

- A track that has trigs starts with **SRC = TRACK**: its own trigs, which the other pages then shape. An empty track starts with **SRC = RAND**. A recipe is made when Trig Seedr first shows the track, and is kept: a track that was empty then stays on RAND after you record trigs into it.
- A page at 0 (or DEF) leaves its fields as they are. With SRC = TRACK and every page at 0, YES changes nothing.
- Recipes are kept in memory for the pattern in play: changing pattern starts new ones. Power off forgets them; what was written is ordinary pattern data.

## The audition

- The **first YES** copies the track (TOOLS: every track) and writes the take.
- **Every YES after it** starts again from that copy: you hear takes, not changes on top of changes.
- **YES with no knob turned** since the track's last YES moves to the **next seed**: a new take. After a knob turn, YES keeps the seed, so you hear just that change.
- **NO** restores the original. **FUNC + YES** keeps the take; so does leaving the track or the pattern.
- A take that cannot be written (a lock with no room in the pattern, a feature not built) writes nothing and says why for two seconds.

## TAKE

![TAKE](../../../assets/screens/take.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | SEED | 0–9999 | the take number |
| B | VARY | 0–100 % | how much of take n the next take keeps: 0 every seed is unrelated, 100 every seed gives the same take |
| F | BEAT | 1–16 | steps per beat, for BIAS and the velocity shapes |

## VIEW

![VIEW](../../../assets/screens/view.png)

Knob A moves the cursor. The 16 steps around it (filled: a note trig; small: a lock trig; a bar under a step: accent; a dot above: retrig) and, for the step under the caret:

- `N V L U`: note, velocity, length code, micro timing (`-` = the track default).
- `C R`: trig condition, retrig (rate and velocity curve); the lane 1 lock, if any.
- `F0381`: the step's flag word in hex (0x0001 trig, 0x0002 retrig, 0x0004 mute, 0x0008 accent, 0x0010 swing, 0x0020 slide, 0x0080 / 0x0100 / 0x0200 SYN / SMP / ENV on); `DN` / `DV`: the track's default note and velocity.

## The other pages

- [The layers](../generator/): RHYTHM, PLACE, DYNAMICS, NOTE, LOCK, TRIG.
- [TIME](../groove/): grooves, random micro timing.
- [Rotating](../rotate/): PLACE's rotations and TOOLS' ROTATE ALL.
