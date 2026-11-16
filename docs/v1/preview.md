# The preview strip and VIEW

## The preview strip

The bottom of every page except VIEW shows the take that YES would write on the track in view: one bar per step,
2 pixels apart, its height the step's velocity (a step at the track's default velocity uses that default). An
empty step on a beat (every 4th step) has a dot. A track of 64 steps fills the screen's width.

The strip is rendered from the track's recipe and its original:

- **A knob turn** shows its result at once, before YES. Nothing is written to the track until YES.
- **While a take is auditioned**, the strip renders from the original track, as the next YES will, not from the
  take that is playing. Turn a knob and the strip shows the next take while the current one keeps playing.
- **After YES** the strip and the track are the same take. A second YES with no knob turned takes the next seed;
  the strip shows it once the seed changes in the title bar. After NO or NOT APPLIED, YES brings back the take the
  strip shows, the one you heard.
- **Trigs recorded with the TRIG keys** while seedr is open change the original: with SOURCE = TRACK the strip
  follows them.

Messages (APPLIED, NOT APPLIED, BACK TO THE ORIGINAL, RECORD TO GENERATE, a groove's name) replace the strip for 2
seconds. When YES would write nothing, the strip says why instead: NOT ON THE FX TRACK, EUCLID IS ON: TURN IT OFF,
or the recipe's error. With RECORD off the strip still previews the take: YES needs RECORD to write it. A lock that does not fit in the pattern's lock rows is reported only
at YES (NO LOCK ROWS LEFT).

## VIEW

VIEW shows the track's steps as they are (during an audition: the take), read-only. Knob A moves the cursor.

The top row shows the 16 steps of the cursor's page:

| Mark | Meaning |
|---|---|
| tall box | a trig |
| small box | a trig with the SYN, SMP and ENV switches off (as seedr writes trigs today) |
| hollow box | a muted trig |
| line under the box | the accent flag |
| short line over the box, left | a retrig |
| short line over the box, right | a lock on lane 1 or lane 2 |
| arrow under the step | the cursor |

Below it, for the step under the cursor:

- `S` step, `N` note, `V` velocity, `L` length (`-`: the track's default), `U` micro timing;
- `C` trig condition, `R` retrig rate and velocity (`-`: no retrig), and each lane's lock as its parameter's name
  and value (only when the step has one);
- at the right, the step's flag word (`F`, hex) and the track's default note and velocity (`DN`, `DV`). These
  are left out when the lock values fill the line: first the defaults, then the flag word.

VIEW has the "Trig Seedr" footer in place of the strip.
