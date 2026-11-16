# trig-seedr 0.7

## 0.7.0

0.7 is the next step of version 1's milestone M2 towards 1.0: the changes from playing 0.6.0 on a Rytm MKII (M2.5), the
TOOLS page's YOLO, and the lead's small items. Generate and apply work as in 0.5.1, with YES after a revert below.

## New

- **TOOLS YOLO**: YES rolls a whole recipe for the track you are on and plays it; YES again rolls the next; FUNC + YES
  keeps it; NO puts the track and every page's settings back as they were before the first roll. WILD (0–100) sets
  how far a roll goes; ROLL (0–9999) finds a roll again. Rolls pick from musical ranges that suit the track (BD kick
  rhythms, SD backbeats, hats busier lines), never touch other tracks or the pattern's shared MAP settings, and keep
  the meter and the lock lanes' parameters. The site's Using, YOLO (Alan, hardware test).
- **A dot in the title bar** while a take is playing but not applied (was the letters AUD), before the source:
  `● S42`. FUNC + YES, NO or NOT APPLIED clear it (Alan, hardware test).

## Changed

- **YES after a revert brings the take you heard back.** After NO, or NOT APPLIED (leaving the track, pattern or
  Trig Seedr, or turning RECORD off), YES with no knob turned writes the same seed again instead of the next one,
  so FUNC + YES keeps the take you were hearing. YES while a take plays, or after FUNC + YES, still moves to the next
  seed (Alan, hardware test).

- **Range knobs** (VEL, LEN, NOTE, RNGE, CVAL, RATE): the knob sets only the high end and FUNC + knob only the low
  end. An end stops where it meets the other; before, turning the high end down past the low end pushed the low end
  down with it (Alan, hardware test).

- **More knobs hidden while they cannot change the take**, as 0.6 began (they keep their value): CTYP and CVAL
  while COND is 0, RATE and RCUR while RTRG is 0, and TIME, GVEL and SHFT while no groove is chosen (GRV OFF).

## Tests

- **YOLO**: `lib/test_yolo.c` (every roll at WILD 0–100 on five roles and lengths renders, repeats, keeps the meter and
  lanes, stays in its musical ranges; 31,000 checks) and `tools/tests/43-yolo.sh` (the takes and recipes against
  `seedref yolo=ROLL:WILD`, NO, FUNC + YES and NOT APPLIED with the recipe).
- **The device layer is fuzzed on the host** (`device/test_rytm_ar2.c`): random takes written into a model of
  stock's memory, checked against I6–I8 and for every field and lock; 3,000 cases in every `make -C device test`.
