# QUAL: the best of several takes

QUAL renders several candidates of the same take and keeps the best-distributed one. At QUAL 1 (the default) the
take is the only candidate, as before.

## TAKE: QUAL

| Knob | Values | Meaning |
|---|---|---|
| QUAL | 1..8 | candidates rendered for each take; the one with the best score is shown and written |

Each candidate is a different set of trig positions drawn from the same seed. Only RHYTHM and PLACE differ between
candidates. Velocity, notes, lengths, locks, conditions, retrigs and micro timing draw the same random numbers for
every candidate, so the winner is dressed as the first candidate would have been on the same steps.

## The score

The score looks only at where the trigs are. Each item decides only when the ones before it are equal:

1. **Count kept** (RANDOM only). The take that keeps closest to the count DENS asks for. Candidates differ here only
   when MASK or SKIP removes trigs, so QUAL then prefers a take whose trigs lie outside the steps they clear.
2. **No long holes.** The take whose largest gap between consecutive trigs is the smallest. The gap from the last
   trig round to the first counts too, as the pattern loops.
3. **No clumps.** The take with the fewest adjacent trigs: trigs one grid step apart (RHYTHM's GRID for RANDOM,
   one step for every other source).
4. **Fit to the meter** (RANDOM with BIAS other than 0). The take whose trigs have the highest mean rank in the
   meter (Barlow's rank for METR, BEAT and ROLE, see `meter.md`). With a negative BIAS, the lowest.

Items 2 and 3 do not change when the take is rotated, so at BIAS 0 QUAL does not move trigs towards strong or weak
steps. It picks a better-spread take with the same feel. With a BIAS, item 4 settles what the others leave equal,
in BIAS's direction.

Two candidates with the same score: the earlier one wins. So QUAL has no effect when every candidate has the same
trigs: SOURCE = TRACK, EUCLID, COPY and MAP with CHAOS below 4, without SYNC, and RANDOM with BIAS at +100 or −100.
The TAKE page shows QUAL only while the trig positions come from the seed (SRC = RAND, MAP with CHAOS 4 or more,
or SYNC above 0); its value is kept while it is hidden. Raising QUAL keeps the
take unless one of the added candidates scores higher.

Over 1,000 takes of RANDOM on 16 steps (`lib/test_score.c`):

| DENS, BIAS | | QUAL 1 | 2 | 4 | 8 |
|---|---|---|---|---|---|
| 25 %, 0 | largest gap (steps) | 7.75 | 6.74 | 6.11 | 5.63 |
| | trigs on the four beats (of 4,000) | 1,015 | 982 | 986 | 970 |
| 50 %, 0 | largest gap | 4.42 | 3.86 | 3.48 | 3.20 |
| | adjacent trigs per take | 3.74 | 3.22 | 2.84 | 2.44 |
| 25 %, 60 | trigs on the four beats (of 4,000) | 2,869 | 3,113 | 3,392 | 3,721 |

## QUAL and the other TAKE knobs

- **SEED.** A take is still a function of its seed number: the same SEED and settings give the same take.
- **VARY** keeps a share of each candidate's random numbers from one take to the next. When take n + 1 is won by
  another candidate than take n, their trig positions are unrelated, so with QUAL above 1 consecutive takes share
  less than VARY says. The dressing (velocity and the rest) still keeps VARY's share. Measured on 16 steps, DENS 50:

  | VARY | QUAL 1 | 2 | 4 | 8 |
  |---|---|---|---|---|
  | 50 % | 49 % | 26 % | 17 % | 10 % |
  | 90 % | 89 % | 71 % | 60 % | 51 % |

  VARY 0 and VARY 100 behave as at QUAL 1: unrelated takes and one take for every seed.
- **PHRS and CVAR** choose the best candidate for each take they combine.

## DENS with QUAL above 1

At QUAL 1, raising DENS only adds trigs and lowering it removes them in reverse (design P2). With QUAL above 1 a
DENS change can make another candidate win, so the take can change more than one trig at a time. Set QUAL back to 1
to sweep DENS like a fill.

## Cost

Every candidate runs RHYTHM and PLACE; the layers after them run once, on the winner. On a 64-step track with most
layers on, a render took 1.1 ms at QUAL 1, 2.3 ms at QUAL 4 and 3.7 ms at QUAL 8 in the emulator.
