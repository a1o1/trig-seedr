# T0b: the Syntakt's playhead (TURING's live page)

State: ready for review
Branch: worktree-t0b-st-playhead (from worktree-turing at a25c8fa)

## Done

- **The playhead on OS 1.42** (firmware notes section 9): the sequencer's running byte `0x43bd83bf` and each track's
  last step `0x43bd8416 + t`, next step `0x43bd83fc + t`, bar count `0x43bd853c + t`, and the UI's copies
  `0x444e3370 / 0x444e337d + t`; measured in analog-emu-st (snapshots during PLAY, STOP). Synfact's
  `docs/MACHINE-REFERENCE.md` section 9 gives OS 1.41's, each 0x1000 lower; credited.
- **`device/syntakt_st.h`:**
  - `st_playhead(t)`: the step track t played last, or -1 while stopped (or for a track out of range);
  - `st_steps_passed(from, to, len, mask)`: the steps between two reads, as a mask; from == to: none;
  - `st_seed_write_steps(track, parm, take, mask)`: the device write for only the steps a mask names;
    `st_seed_write` is it with no mask, unchanged.
  - Host tests (`device/test_syntakt_st.c`): the playhead; every from / to / length; 500 cases of masked writes in
    playhead order equal to the whole write, other tracks untouched, restore byte-identical. 18,378 checks, 0 failed.
- **`mods/livetest-st`**, a test build: each tick reads the playhead and writes the passed steps from a target.
- **`tools/tests-st/45-live.sh`** (runs in `st_run_tests.sh`), 16 checks, 0 failed, about 2 minutes: track 4 (5 steps)
  emptied with stock's keys; live towards trigs on steps 1 and 4 while PLAY runs 12 s:
  - the track ends as the target, every step written, at most 2 steps a tick at 135 BPM;
  - every onset of track 4 (dtboot --dsp-wav) on a target step, both heard, 33 onsets;
  - the first loop plays the original: the first onset is the second loop's step 1 (1,058 ms; PLAY at 500 ms, a loop
    555 ms);
  - no other track changes (records and lock stores);
  - still playing, a restore gives track 4 back byte for byte.

## Findings

- **The UI tick is 30 Hz** (90 ticks in 600M instructions). At 300 BPM with a track at speed 2x a step is 25 ms, so
  up to two steps pass between reads: the mask covers them.
- **A step is heard on the next loop** when written at or behind the playhead: the sequencer prepares a step's trig a
  step ahead. So the live page's rule "write the steps the playhead has passed" (design L6) never changes what is
  about to play.
- **Cost:** about 57,000 instructions of emulated time a step written (every field and the trig changing; interrupts
  included), about 0.3 ms; two steps a tick under 2 % of a tick. More than about 20,000 step writes in one tick stall
  the UI task (seen at 400 rewrites of 64 steps; 300 finished): far from a live page's few a tick.
- **Missed ticks are made up**: a tick busy for several periods is followed by quick ticks, so counting ticks does not
  measure work; the cost above is timed by when the tick clears its command, sampled every 1M instructions.
- `dtboot --break` refuses addresses outside its hook window (stock code), so a mod's function cannot be timed by
  breakpoints; `--watch` logs writes without their clock.

## Decisions (by the project's precedent, for the lead to confirm)

- **Polling from the tick, not a sequencer hook**: the minimal core has UI events only (SD1), and polling needs no
  stock-code patch. Design L8 allows either.
- **Masked write in the device layer** rather than in the mod, so the live page and any later tool share it, with host
  tests as for the rest of the layer.
- **from == to means no move.** A whole loop between two reads cannot be told from no move; it cannot happen at 30 Hz
  except on a 1-step track.

## Open points

- A **1-step track** never changes its playhead byte: the live page cannot see its loops by polling. The bar count
  (`0x43bd853c + t`) might give them; not checked.
- **Per-track speed** (the track scale) is read from the playhead as it is; only the default speeds were measured.
- **Pattern changes and resyncs** set the step from the pattern's position (Synfact lists the sites); the live page's
  NOT APPLIED on a pattern change (L6) covers them, not checked here.
- On a unit: the 30 Hz tick and the step timing are the emulator's; the next loop rule follows from the code.
