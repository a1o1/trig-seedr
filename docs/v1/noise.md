# NOISE: contours over the steps

NOISE gives a value that changes slowly from step to step instead of jumping at random. Every ZOOM steps there is
a random level, and the steps between move from one level to the next along an S-curve (value noise with a
smoothstep). The result is a contour: a note line that drifts up and down, a filter or sample lock that sweeps, a
velocity that swells and falls.

NOISE is a value source, like RANGE: NOTE and both LOCK lanes take it, and DYNAMICS has it as a shape.

## Where it is

| Page | Knob | Values | Meaning |
|---|---|---|---|
| NOTE | NSRC | RANGE, LIST, NOISE | NOISE: the note follows a contour inside the NOTE range |
| NOTE | ZOOM | 1..64 | steps from one random level to the next (shown with NSRC = NOISE) |
| LOCK | SRC | RANGE, LIST, NOISE | NOISE: the lane's locks follow a contour inside RNGE |
| LOCK | ZOOM | 1..64 | as NOTE's, per lane (shown with SRC = NOISE) |
| DYNAMICS | SHAPE | ... NOISE ... | velocity follows a contour between VEL's low and high end; AMT is its share |
| DYNAMICS | N | 1..64 | with SHAPE = NOISE: the zoom |

The range knobs (NOTE, RNGE, VEL) set where the contour lies. NOISE does nothing while its range is DEF (NOTE, VEL)
or its lane's % is 0 (LOCK), as with RANGE.

## ZOOM

| ZOOM | Result |
|---|---|
| 1 | a new random level on every step: the same spread as RANGE |
| 2..8 | a contour that turns every few steps |
| 16..64 | a slow sweep over a page or the whole track |

The track's first step is always one of the random levels. The contour wraps at the track's length: the last
steps lead back to the first step's level, so the loop has no jump. When the length is not a multiple of ZOOM, the
last cell is shorter than the others.

## The range

The contour's lowest level is the range's low end and its highest the high end; each value of the range is equally
likely at the random levels. A range whose low end (FUNC + knob) is above its high end turns the contour over.

## What decides the values

- **The step, not the trig.** A trig's value comes from the contour at its step. Raising DENS or a lane's % adds
  trigs or locks without changing the values of those already there, and SKIP or MASK removing a trig changes no
  other trig's value.
- **SEED and VARY.** Each random level of the contour is one of the take's random numbers. With VARY at k %, the
  next SEED keeps about k % of the levels and draws the rest again, so the contour of one take is a variation of
  the previous one's. VARY 100 gives every SEED the same contour; VARY 0 a new one each time.
- **Each place its own contour.** NOTE, each LOCK lane and the velocity shape draw different contours from the same
  take, so turning one does not change another.
