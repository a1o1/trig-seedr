---
title: TURING
description: "A Turing machine for a track: a loop that changes a little each time round, written down the track or rewritten while the pattern plays."
sidebar: { order: 12 }
---

New in 0.9, a candidate for testing on a unit. A Turing machine here is a loop of LOOP steps. Each step of the loop
holds a trig (on or off) and a value. Each time the loop comes round it changes a little, by an amount AMT sets and
in a way FLAV sets; the values can drive notes, velocities and parameter locks, so pitch and sound change with the
rhythm. Trig Seedr has it in two forms:

| | Where | What it writes |
|---|---|---|
| **TURG, the source** | RHYTHM: SRC = TURG ([RHYTHM](../rhythm/#turg)) | a whole take at once: the loop's passes one after another down the track. On a 64-step track with LOOP 8 you hear eight passes, each drawn from the one before. It follows every [generate and apply](../concepts/#generate-and-apply) rule: YES, NO, FUNC + YES, the preview, SEED and VARY |
| **TURING, the live page** | the last page, after TOOLS | the track while the pattern plays: each step the playhead has just passed takes the loop's next change, so you hear the loop drift as it plays |

The same machine runs in both: the same FLAV, LOOP, AMT and BIAS give the same steps.

## The knobs

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | FLAV | DJVU, CRPT, SHFT | DJVU | always |
| B | LOOP | 1–16 | 8 | always |
| C | AMT | 0–100 | 25 | always |
| D | BIAS | 0–100 % | 50 % | always |

These are the track's own TURING settings, the same as RHYTHM's (FLAV, LOOP, AMT, BIAS there); turning one on
either page changes both. While the page runs they can be turned, and the next steps follow them.

- **LOOP**: the loop's length in steps (SHFT: at least 2).
- **BIAS**: the chance a new step of the loop gets a trig.
- **STRT** (on RHYTHM only): the first pass from the seed, or the track's own first LOOP steps.

## The flavours

| FLAV | AMT 0 | AMT 50 | AMT 100 |
|---|---|---|---|
| DJVU | every pass new | the loop repeats exactly | the loop's steps play in a shuffled order |
| CRPT | the loop repeats exactly | values change (up to 25 % a pass), trigs stay | values change more and trigs flip, up to half a pass |
| SHFT | the loop inverts each pass, so it repeats every two | a coin flip for each step | the loop repeats exactly |

- **DJVU** keeps a loop of steps and, below the middle, brings in new ones; above it, it replays the same steps out
  of order.
- **CRPT** changes the values first, so the rhythm stays while the melody drifts; past the middle the trigs flip
  too.
- **SHFT** is a shift register: each step comes round again, kept or inverted. Its value is the newest eight
  steps read as a number.

## Notes: SCALE and ROOT

A value becomes a note through NOTE's range and its **SCALE** (NOTE, knob H), on **ROOT** (FUNC + the SCALE knob,
C … B, while a scale is on): the nearest note of the scale, the lower one on a tie. SCALE works for every note
source, not only TURG; OFF leaves notes as they are. See [NOTE](../note/#turg-and-scale).

## The live page

![TURING, the live page](../../../assets/screens/turing.png)

Below the knobs, the loop's steps: a box when a step's trig is on, a bar for its value, the playhead's step
outlined. The strip says **YES: START**, or **LIVE** and the step while it runs.

| Key | Does |
|---|---|
| **YES** | starts: writes the take of the run (as TURG would, with PHRS and CVAR off), then rewrites each step as the playhead passes it. Needs RECORD on (grid recording), as every generate. YES while it runs rewrites every step from the loop as it stands (RESTARTED) |
| **FUNC + YES** | keeps the track as it stands and stops (APPLIED) |
| **NO** | puts the original back and stops |
| **FUNC + UP** | remembers the loop (REMEMBERED) |
| **FUNC + DOWN** | brings the remembered loop back (RECALLED; NOTHING REMEMBERED if none). Both say YES TO START with no run |

A live run is a take that keeps changing, so it follows the same rules as any take not yet applied: **closing Trig
Seedr, changing track or pattern, or turning RECORD off puts the original back (NOT APPLIED)**. YES on another
page while it runs does the same first. Only the track you are on runs, only below its length; the steps the
playhead is on or about to play are never written.

- **The FX track** is refused (NOT ON THE FX TRACK), and so is a track in stock's Euclidean mode.
- **A one-step track** writes nothing live: its playhead never moves on.
- The remembered loop lasts until the unit is switched off.

## On each device

The same on the Analog Rytm MKII and the Syntakt. Both rewrite only steps the playhead has passed, read from the
running sequencer each screen refresh. A step takes about 2 ms to write on the Rytm and about 0.3 ms on the Syntakt.
Track speeds other than 1x and pattern changes while the page runs have not been tried on a unit yet:
[Feedback](../../beta/feedback/) is welcome.
