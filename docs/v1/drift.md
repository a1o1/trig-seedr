# DRFT: drifting micro timing

DRFT moves the trigs' micro timing along a slow curve: neighbouring steps are early or late together, and the
curve wanders from ahead of the beat to behind it and back over the track, as a player who rushes for a while and
then drags. RAND gives every trig its own random offset; DRFT gives offsets that belong together.

## TIME: DRFT

| Knob | Values | Meaning |
|---|---|---|
| DRFT | 0..100 % | the largest drift, as a share of the device's micro-timing range (Rytm: ±23 at 100 %, as RAND) |

At 0 % DRFT changes nothing. DRFT runs last in TIME: QUANT, the groove (GRV, TIME, SHFT) and RAND first, then the
drift is added. The result stays inside the device's range.

## The curve

The curve depends on the step's position, not on which steps have trigs: changing DENS, SYNC or MASK does not
change the drift a step gets, only which steps have a trig to carry it.

It is a sum of layers of smooth noise: one with a new value every 2 steps, one every 4, 8, 16 and 32, each
interpolated in a straight line between its values. A layer is used when its spacing is shorter than the track,
so a 16-step track has three layers (2, 4, 8) and a 64-step track five. The longer layers give the slow wander,
the shorter ones the small movements on top. The curve wraps at the track's length, so the last step leads into
the first without a jump when the pattern loops.

The drift on a step is at most DRFT % of the range. Most steps sit well inside it: on the Rytm at DRFT 100 % the
average drift is about ±8 of 23, and the largest values come where the layers agree.

A track of one or two steps has no room to drift; DRFT leaves it as it is.

## The seed and VARY

The layers' values come from the take, as every layer's random numbers do. Another SEED gives another curve. With
VARY, a take keeps a share of the previous take's values and draws the rest again, so consecutive takes drift in
related ways; at VARY 100 % every take has the same curve. Raising DRFT scales the same curve: no step changes
direction.

## On SOURCE = TRACK

DRFT adds to the micro timing the trigs already have. Steps without a trig are not changed.
