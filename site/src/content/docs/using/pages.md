---
title: Keys and pages
sidebar: { order: 1 }
---

Trig Seedr is a third page behind the **TRIG** key, after the Euclidean page:

**TRIG** (stock's TRIG page) → **TRIG** (EUCLIDEAN) → **TRIG** (Trig Seedr) → **TRIG** (back to stock's TRIG page)

**TRIG held + PAGE** opens it from any page. A parameter page key (SRC, SMPL, FLTR, AMP, LFO, FX) or a menu key closes it. **LEFT / RIGHT** step through its pages:

**TAKE → RHYTHM → PLACE → DYNAMICS → NOTE → LOCK → TRIG → TIME → VIEW → TOOLS → TAKE**

A page, knob or value whose feature is not built yet is not shown. The title bar shows the page, the track and the page's place among the pages shown (`3/10`), and at the right the source: `TRK` (the track's own trigs), `EUC` (EUCLID), `CPY` (COPY), `MAP` (MAP while CHAOS is below 4), or `S` and the seed for a source that draws from it, after `AUD` while a take is being auditioned. The place is left out when a long title would not fit.

| Key | What it does |
|---|---|
| TRIG on the EUCLIDEAN page | open |
| TRIG on Trig Seedr | close (stock's TRIG page) |
| TRIG held + PAGE | open, from anywhere |
| LEFT / RIGHT | the previous / next page |
| UP / DOWN | LOCK: lane 1 or 2 |
| knobs A–H | the page's settings |
| FUNC + knob | the low end of a range (VEL, LEN, NOTE, RNGE, CVAL, RATE); SEED in steps of 100; other wide knobs in steps of 10 |
| YES | generate: write a take to hear it (needs RECORD on); on TOOLS, run the tool |
| NO | with a take pending: back to the original; otherwise: close |
| FUNC + YES | apply: keep the take (the only way a take stays) |
| PAGE, the TRIG keys, transport | stock's: PAGE still picks which 16 steps the TRIG keys show |

## One recipe per track

Every track has a recipe: the settings of all the pages. **YES renders the whole recipe from the original track**, every time, so YES means the same on every page.

- A track that has trigs starts with **SRC = TRACK**: its own trigs, which the other pages then shape. An empty track starts with **SRC = RAND**. A recipe is made when Trig Seedr first shows the track, and is kept: a track that was empty then stays on RAND after you record trigs into it.
- A page at 0 (or DEF) leaves its fields as they are. With SRC = TRACK and every page at 0, YES changes nothing.
- Recipes are kept in memory for the pattern in play: changing pattern starts new ones. Power off forgets them; what was written is ordinary pattern data.

## Generate and apply

Trig Seedr **generates** a take and you **apply** it. Generating writes real trigs into the track so you can hear
them; the track's original is kept until you apply the take, and comes back if you don't.

This is not how the Rytm's Euclidean mode works. Euclidean mode is built into the OS: the Rytm plays a separate
Euclidean pattern, so your stored trigs never change, and turning EUC off brings them back at any time. Trig Seedr
has no such separate pattern: the sequencer plays the stored trigs, so a take has to be written into them. That is
why generating needs RECORD (as any editing of trigs does) and why a take stays only when you apply it.

| You do | RECORD off | RECORD on (grid recording) |
|---|---|---|
| Open Trig Seedr, change page, turn knobs | Everything works. The strip at the bottom previews the take you would get. Nothing is written | The same |
| **YES** (generate) | Nothing is written. The screen says **RECORD TO GENERATE** | The take is written so you can hear it; your original is kept. The title shows **AUD** |
| **YES** again | | A new take, always from your original: the next seed if you turned no knob, otherwise your change |
| **NO** | Closes Trig Seedr | Your original comes back. With no take pending, NO closes Trig Seedr |
| **FUNC + YES** (apply) | Nothing to apply | **The take stays and your original is dropped.** This is the only way to keep a take. The screen says **APPLIED** |
| Change track or pattern, close Trig Seedr, or turn RECORD off, with a take not applied | | Your original comes back. The screen says **NOT APPLIED** |
| TOOLS: ROTATE ALL | Nothing happens (**RECORD TO GENERATE**) | YES rotates every track, FUNC + YES applies, NO or leaving puts every track back |
| Save the pattern with a take not applied | | The save stores the take as it is at that moment |

Good to know:

- **Nothing is lost by not applying.** The track's recipe and seed stay, so YES generates the same take again.
- RECORD means grid recording: RECORD lit, the sequencer stopped or playing. Live recording (RECORD + PLAY) does
  not count.
- A take that cannot be written (a lock with no room left in the pattern) writes nothing and says why for two
  seconds.

## TAKE

![TAKE](../../../assets/screens/take.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | SEED | 0–9999 | the take number |
| B | VARY | 0–100 % | how much of take n the next take keeps: 0 every seed is unrelated, 100 every seed gives the same take |
| C | PHRS | OFF, AAAA, AAAB, ABAB, ABAC, ABCD | a phrase of takes over the 16-step pages; shown on a track longer than 16 steps ([Phrases and CVAR](../../features/vary/)) |
| D | CVAR | OFF, 1:2, AAAB | takes n and n + 1 written as alternating trig conditions; shown while the trig positions come from the seed: SRC = RAND, MAP with CHAOS 4 or more, or SYNC above 0 ([Phrases and CVAR](../../features/vary/)) |
| E | METR | 4/4, 3/4, 6/8, 12/16, 5/8, 7/8 | the meter, for BIAS, ROLE and the velocity shapes ([Meters and roles](../../features/meter/)) |
| F | BEAT | 1–16 | steps per note value of the meter (4/4: a quarter; 6/8: an eighth) |
| G | QUAL | 1–8 | the best spread of this many candidate takes; shown as CVAR is ([QUAL](../../features/score/)) |

## VIEW

![VIEW](../../../assets/screens/view.png)

Knob A moves the cursor. The 16 steps around it (filled: a note trig; small: a lock trig; a bar under a step: accent; a dot above: retrig) and, for the step under the caret:

- `N V L U`: note, velocity, length code, micro timing (`-` = the track default).
- `C R`: trig condition, retrig (rate and velocity curve); the lane 1 lock, if any.
- `F0381`: the step's flag word in hex (0x0001 trig, 0x0002 retrig, 0x0004 mute, 0x0008 accent, 0x0010 swing, 0x0020 slide, 0x0080 / 0x0100 / 0x0200 SYN / SMP / ENV on); `DN` / `DV`: the track's default note and velocity.

## The other pages

- [The layers](../generator/): RHYTHM, PLACE, DYNAMICS, NOTE, LOCK, TRIG.
- The Features section: a page for each of version 1's features.
- [TIME](../groove/): grooves, random micro timing.
- [Rotating](../rotate/): PLACE's rotations and TOOLS' ROTATE ALL.
