# trig-seedr 0.7

## 0.7.0 (in progress)

0.7 is the next step of version 1's milestone M2 towards 1.0: the changes from hardware testing (M2.5) and the
lead's small items.

## Tests

- **The device layer is fuzzed on the host** (`device/test_rytm_ar2.c`): random takes written into a model of
  stock's memory, checked against I6–I8 and for every field and lock; 3,000 cases in every `make -C device test`.

## Changed

- **Range knobs** (VEL, LEN, NOTE, RNGE, CVAL, RATE): the knob sets only the high end and FUNC + knob only the low
  end. An end stops where it meets the other; before, turning the high end down past the low end pushed the low end
  down with it (Alan, hardware test).

- **More knobs hidden while they cannot change the take**, as 0.6 began (they keep their value): CTYP and CVAL
  while COND is 0, RATE and RCUR while RTRG is 0, and TIME, GVEL and SHFT while no groove is chosen (GRV OFF).
