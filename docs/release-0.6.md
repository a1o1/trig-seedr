# trig-seedr 0.6

## 0.6.0

0.6 is version 1's milestone M2 (integration): every combination of sources and layers tested together, fuzzed
and measured. It has 0.5.1's generate-and-apply rules unchanged. How it is tested: `docs/testing.md` (the site's
About, How it is tested).

## Fixed

| What | Before | Now | Found by |
|---|---|---|---|
| Two LOCK lanes on one parameter | the Rytm keeps one lock per parameter and step, so lane 1's new locks were lost | PARM skips the parameter the other lane is on; a recipe with both lanes on one parameter is refused (SETTING OUT OF RANGE) and nothing is written | M2.2 (fuzzing) |
| EUCLID under PHRS | the cycle restarted on every 16-step page (13/5 on 64 steps played its first 16 steps four times) | the cycle runs along the whole track; the phrase letters only dress its trigs | M2.1 (interplay) |
| PHRS on a short last page (24 or 40 steps) | ROT and SYNC pushed real trigs past the track's end | with SOURCE = TRACK, COPY or EUCLID a short last page is a phrase of its own length; RAND and MAP are unchanged | M2.1 |
| A:B's CVAL | shown as `/10` | `/8`, as the engine plays it | M2.1 |

## Changed

- **Knobs that cannot change the take are hidden**, and keep their value: PHRS on a track of 16 steps or fewer;
  CVAR and QUAL while the trig positions do not come from the seed (they do with SRC = RAND, MAP with CHAOS 4 or
  more, or SYNC above 0); ROLE at BIAS 0; AMOD at ACC 0.

## Tests

- **Fuzzing** (`lib/test_fuzz.c`, `tools/fuzz.sh`): random recipes over every source, layer and value source on
  three device profiles; 4,000 cases in every `make test`, and a long run of 18.6 million cases with no failures.
- **Interplay** (`mods/seedr-ar2/test_interplay.c`): every hidden knob, turned, leaves the take byte-identical;
  about 533,000 checks in every `make test`.
- **Emulator**: `40-interplay` and `41-fuzz-sample` added.

## Known

- MAP under PHRS plays the map's first 16 steps on every page, so steps 17–32 of the map are not heard
  (`docs/v1/vary.md`). Left for hardware testing.
