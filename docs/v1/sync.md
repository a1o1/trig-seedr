# SYNC: syncopation by anticipation

SYNC moves trigs one grid step earlier, onto a weaker step, so they arrive before the beat they belonged to and
the beat itself is left silent. The method follows George Sioros and Carlos Guedes' syncopation transformations
(anticipation of an onset onto the preceding weaker pulse). A moved trig keeps everything it had: velocity, note,
length, micro timing, condition, retrig and every parameter lock.

## PLACE: SYNC

| Knob | Values | Meaning |
|---|---|---|
| SYNC | 0..100 % | the share of the possible anticipations that is made |

At 0 % SYNC changes nothing. SYNC runs after ROT and before MASK and SKIP.

## Which trigs move

A trig can be anticipated when:

- the step one grid step before it is empty, and no other trig lies in between;
- that step is weaker in the meter (Barlow's rank for METR and BEAT, see `meter.md`; ROLE does not change it);
- neither step is one that SKIP clears.

The grid step is RHYTHM's GRID when the source is RANDOM (1, 2, 4 or 8 steps), and one step for every other
source, SOURCE = TRACK included. The loop wraps: the bar's first step can be anticipated onto the track's last step.

**Runs resolve onto a strong step.** Two or more consecutive trigs that are all anticipated form a run. A run is
allowed only when the trig after it stays where it is and is stronger than every step the run left. So the rhythm
always lands again on a stronger step, and SYNC never moves every trig (that would only shift the track by one
step). A single anticipation, followed by a trig that stays, is always allowed.

Examples in 4/4 with BEAT 4 (steps numbered from 1):

| Trigs | SYNC 100 % gives | Why |
|---|---|---|
| 5, 10 | 4, 10 | 10 cannot move: step 9 is stronger |
| 1, 5, 9, 13 (four on the floor) | 1, 4, 8, 12; 1, 5, 8, 12; 1, 4, 9, 12; or 5, 8, 13, 16 | at most three of the four move; which ones depends on SEED |
| 1, 5, 6 | 1, 4, 6 or 5, 6, 16 | 1 and 5 together would be a run ending on 6, a weak step |

## The share and the seed

The trigs that can be anticipated are tried in an order that comes from the take (SEED, and VARY as for every
layer); each anticipation is kept when every run still resolves. SYNC takes the first share of the kept ones, so
raising SYNC only adds anticipations and lowering it removes them in reverse order. Another SEED tries them in
another order.

## What SYNC keeps

- **The count of trigs.** SYNC never adds or removes a trig.
- **Whole steps.** On SOURCE = TRACK the trig's step moves with all its parameter locks, as ROT moves them. The
  step it moves onto takes the trig's old place.
- **Swing** stays with the position, as with ROT.
- **SKIP:** no trig moves into the steps SKIP clears, and none comes out of them.

## Lock rows

Moving a step with parameter locks needs the pattern to have room for them. When the pattern has no lock rows
left, YES shows NO LOCK ROWS LEFT and nothing is written.
