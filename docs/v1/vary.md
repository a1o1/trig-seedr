# PHRS and CVAR: variation over pages and over loops

A take is a seed and a recipe. VARY decides how much take n + 1 keeps of take n. PHRS and CVAR use those related
takes: PHRS puts them side by side on the track's pages, and CVAR plays them in turn, loop by loop, through trig
conditions.

## TAKE: PHRS, CVAR

| Knob | Values | Meaning |
|---|---|---|
| PHRS | OFF, AAAA, AAAB, ABAB, ABAC, ABCD | a 16-step phrase per page; each letter is a take: A = SEED, B = SEED + 1, C = SEED + 2, D = SEED + 3 |
| CVAR | OFF, 1:2, AAAB | takes SEED and SEED + 1 written together, as conditions |

At OFF both change nothing. VARY sets how alike the letters are: at 0 % B is unrelated to A, at 100 % every
letter is the same take.

## PHRS

Each page of 16 steps plays its letter's take, made as a 16-step phrase from that page's own steps. Pages with the
same letter are the same: with ABAB, page 3 repeats page 1 and page 4 repeats page 2. AAAA repeats one phrase on
every page.

Everything that is random in the take (which steps get trigs, velocities, conditions, locks, random micro timing),
DENS's count, ROT, SYNC and the meter's ranks belong to the phrase, so they repeat with the letter. Three things
belong to the track's positions and stay there:

- **SKIP** clears the track's first steps, not each page's.
- **The groove** (TIME) runs along the whole track, so a 32-step groove keeps its second half on pages 2 and 4.
- **MASK and COPY** read the other track at the same steps as without PHRS.

On SOURCE = TRACK each page keeps its own trigs and their parameter locks; the letter decides how they are dressed
(velocity, conditions, locks, timing), and ROT and SYNC move steps within the page.

**Shorter tracks.** PHRS applies page by page as far as the track goes. A track of 24 steps has page 1 and the
first 8 steps of page 2: with ABAB those are the first 8 steps of phrase B. A track of 16 steps or fewer is a single
phrase, so PHRS changes nothing there.

## CVAR

CVAR renders takes SEED and SEED + 1 and writes both into the track:

- a trig in both takes plays plain (it keeps any condition the TRIG page gave it in take SEED);
- a trig only in take SEED, and a trig only in take SEED + 1, get conditions that take turns.

| CVAR | Take SEED only | Take SEED + 1 only | Plays |
|---|---|---|---|
| 1:2 | 1:2 | 2:2 | SEED, SEED + 1, SEED, SEED + 1, … |
| AAAB | !PRE | 4:4 | SEED three times, then SEED + 1 |

The Rytm has no "not 4:4" condition, so AAAB uses !PRE, which is true when the trig condition evaluated last on
the track was false. It needs a 4:4 trig earlier in the loop. A trig of take SEED that
comes before the first 4:4 trig plays plain, in all four loops.

With VARY high the two takes share most trigs and CVAR changes a few; with VARY 0 they are unrelated. CVAR works
with PHRS: each page's letter is then a pair of takes. The preview shows every trig the track holds, both takes'.

CVAR writes conditions, so it needs a track that has them; on a track without conditions YES refuses it.
