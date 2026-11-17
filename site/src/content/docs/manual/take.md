---
title: TAKE
description: "SEED, VARY, phrases over the pages (PHRS), variation as trig conditions (CVAR), the meter (METR, BEAT) and candidate scoring (QUAL)."
sidebar: { order: 2 }
---

TAKE holds the settings that apply to the take as a whole: which take (SEED), how takes relate (VARY, PHRS, CVAR),
the meter the other pages use (METR, BEAT), and how many candidates are tried (QUAL).

![TAKE](../../../assets/screens/take.png)

| Knob | Name | Values | Default | Shown |
|---|---|---|---|---|
| A | SEED | 0–9999 (FUNC: steps of 100) | 1 | always |
| B | VARY | 0–100 % | 0 % | always |
| C | PHRS | OFF, AAAA, AAAB, ABAB, ABAC, ABCD | OFF | on a track longer than 16 steps |
| D | CVAR | OFF, 1:2, AAAB | OFF | while the trig positions come from the seed |
| E | METR | 4/4, 3/4, 6/8, 12/16, 5/8, 7/8 | 4/4, or the last recipe's | always |
| F | BEAT | 1–16 | 4, or the last recipe's | always |
| G | QUAL | 1–8 | 1 | as CVAR |

"The trig positions come from the seed" means SRC = RAND, MAP with CHAOS 4 or more, or SYNC above 0. With
SRC = TRACK, EUCLID or COPY (and SYNC at 0) every seed has the same trigs, so CVAR and QUAL would change nothing.

## SEED

The take's number. The same SEED, recipe and original give the same take. YES with no knob turned moves to the
next seed while a take is playing or just after FUNC + YES; after NO or NOT APPLIED it brings the same seed back
([Generate and apply](../concepts/#generate-and-apply)). Turn SEED to go back to a take you heard.

## VARY

How much of take n the next take, n + 1, keeps. At 0 % every seed is unrelated to the one before; at 100 % every
seed gives the same take. In between, each of the take's random choices (which steps get a trig, each velocity,
each condition, each lock, each noise level) is kept with VARY's chance and drawn again otherwise. So with VARY at
70 %, pressing YES walks through takes that each keep about 70 % of the one before.

VARY is computed from the seed alone, not from what you heard: take 12 at VARY 70 % is always the same take, and is
related to take 11 whether or not you heard 11. PHRS and CVAR below use these related takes.

## PHRS: a phrase over the pages

| PHRS | Pages 1–4 play |
|---|---|
| OFF | one take over the whole track |
| AAAA | one 16-step phrase on every page |
| AAAB | take A three times, then B |
| ABAB | A, B, A, B |
| ABAC | A, B, A, C |
| ABCD | four phrases |

Each letter is a take: A = SEED, B = SEED + 1, C = SEED + 2, D = SEED + 3. VARY sets how alike they are: at 0 % B is
unrelated to A, at 100 % every letter is the same. Pages with the same letter are the same.

Everything random in the take (which steps get trigs, velocities, conditions, locks, random micro timing), DENS's
count, ROT, SYNC and the meter's ranks belong to the phrase, so they repeat with the letter. Four things belong to
the track's positions and stay there:

- **SKIP** clears the track's first steps, not each page's.
- **The groove** (TIME) runs along the whole track: a 32-step groove keeps its second half on pages 2 and 4.
- **MASK and COPY** read the other track at the same steps as without PHRS.
- **EUCLID** runs its cycle along the whole track, as without PHRS; the letters change only how its trigs are
  dressed.

**MAP under PHRS** is a phrase like RAND: each page plays the map's first 16 steps (with that page's own CHAOS
lift). On a track longer than 16 steps, the map's steps 17 to 32 are not heard while PHRS is on.

**SRC = TRACK** keeps each page's own trigs and their locks; the letter decides how they are dressed (velocity,
conditions, locks, timing), and ROT and SYNC move steps within the page.

**A track that is not a whole number of pages.** PHRS applies page by page as far as the track goes. On 24 steps,
page 1 plays phrase A and the 8 steps of page 2 the first 8 steps of B (with ABAB). With SRC = TRACK, COPY or EUCLID
that short last page is a phrase of its own length, so ROT and SYNC turn its trigs round inside the track and none
is lost. On a track of 16 steps or fewer PHRS changes nothing, and TAKE does not show it.

## CVAR: variation as trig conditions

CVAR renders takes SEED and SEED + 1 and writes both into the track, so the track plays them in turn, loop by loop:

- a trig in both takes plays every time (it keeps any condition TRIG gave it in take SEED);
- a trig in only one of them gets a condition that takes turns with the other's.

| CVAR | Trigs only in SEED | Trigs only in SEED + 1 | Plays |
|---|---|---|---|
| 1:2 | 1:2 | 2:2 | SEED, SEED + 1, SEED, SEED + 1, … |
| AAAB | !PRE | 4:4 | SEED three times, then SEED + 1 |

The Rytm has no "not 4:4" condition, so AAAB uses **!PRE**, which is true when the condition evaluated last on the
track was false. It needs a 4:4 trig earlier in the loop: a trig of take SEED that comes before the first 4:4 trig
plays every time, in all four loops. This is how AAAB works in 0.7; beta feedback will decide whether it stays.

With VARY high the two takes share most trigs and CVAR changes a few; with VARY 0 they are unrelated. CVAR works
with PHRS: each page's letter is then a pair of takes. The preview shows every trig the track holds, both takes'.

## METR and BEAT: the meter

Trig Seedr ranks every step of a bar by how strongly it belongs to the meter, using Clarence Barlow's
*indispensability*. The rank decides where RHYTHM's BIAS puts trigs, how DYNAMICS' shapes weigh steps, which trigs
AMOD = TOP accents, which steps SYNC may move onto, and QUAL's fit to the meter.

| METR | Usual BEAT | Bar | Grouping |
|---|---|---|---|
| 4/4 | 4 | 16 steps | 2 × 2 quarters |
| 3/4 | 4 | 12 steps | 3 quarters |
| 6/8 | 2 | 12 steps | 2 dotted quarters of 3 eighths |
| 12/16 | 1 | 12 steps | 4 dotted eighths of 3 sixteenths |
| 5/8 | 2 | 10 steps | 5 eighths (2 + 3) |
| 7/8 | 2 | 14 steps | 7 eighths (2 + 2 + 3) |

BEAT is the number of steps per note value of the meter: a quarter in 4/4 and 3/4, an eighth in 6/8, 5/8 and 7/8, a
sixteenth in 12/16. A bar is the meter's count times BEAT steps.

In 4/4 with BEAT 4 the sixteen steps rank, strongest first:

```
step   1  9 13  5 15  7 11  3 16  8 12  4 14  6 10  2
```

The bar's first step, the half bar, beat 4, beat 2, then the "&"s, then the sixteenths, a pickup before a beat
ahead of the step after it. A track of several bars uses the same order in each bar; between equal steps of
different bars, bar 1 comes first, then 3, 4 and 2.

METR and BEAT belong to the track. A new recipe takes them from the recipe you generated last.

## QUAL: the best of several candidates

QUAL renders several candidates of the same take and keeps the best-spread one. At 1 the take is the only candidate.

Each candidate is a different set of trig positions drawn from the same seed: only RHYTHM and PLACE differ between
candidates. The other pages draw the same random numbers for every candidate, so the winner is dressed as the first
candidate would have been on the same steps. The score looks only at where the trigs are; each item decides only
when the ones before it are equal:

1. **Count kept** (RAND only): the candidate closest to the count DENS asks for. This differs only when MASK or SKIP
   removes trigs.
2. **No long holes:** the smallest largest gap between consecutive trigs (the gap from the last trig round to the
   first counts too).
3. **No clumps:** the fewest trigs one grid step apart.
4. **Fit to the meter** (RAND with BIAS not 0): the highest mean rank, or the lowest with a negative BIAS.

Items 2 and 3 do not change when a take is rotated, so at BIAS 0 QUAL picks a better-spread take with the same feel
and does not pull trigs towards the beats. Over 1,000 takes of RAND on 16 steps at DENS 25 %, the largest gap went
from 7.75 steps on average at QUAL 1 to 5.63 at QUAL 8.

- **SEED** still names one take: the same SEED and settings give the same take.
- **VARY with QUAL above 1** keeps less than VARY says: when take n + 1 is won by another candidate than take n,
  their trig positions are unrelated. At VARY 90 % on 16 steps, consecutive takes shared 89 % of their trigs at
  QUAL 1 and 51 % at QUAL 8. The dressing (velocity and the rest) still keeps VARY's share.
- **DENS with QUAL above 1** can change more than one trig at a time, because another candidate can win. Set QUAL to
  1 to sweep DENS like a fill.
- **Cost:** on a 64-step track with most pages on, a render took 1.1 ms at QUAL 1 and 3.7 ms at QUAL 8 in the
  emulator.
