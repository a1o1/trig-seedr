# T0a: rewriting steps while the Rytm plays

State: done, for review
Branch: worktree-t0a-rytm-live (from worktree-turing a25c8fa)

## Done

- **`rt_seed_write_one(track, parm, take, i)`** (device/rytm_ar2.h): step i of a take written as `rt_seed_write`
  writes it (the room check for that step's locks first, nothing written when they don't fit; its locks from the
  snapshot's origin; one change notice when it changed), every other step left alone. `rt_seed_room_src` is now
  `rt_seed_room_range` over steps 0..n, so the full write is unchanged.
- **`rt_playhead(track, &length)`** (device/rytm_ar2.h): the step track t plays now and the length it wraps at,
  from core-ar2 0.11's getters (`core_transport`, `core_track_step`, `core_track_length`), -1 while stopped.
- **Host test** (device/test_rytm_ar2.c, `test_write_one`, 2,000 random takes): write_one of step i gives the same
  records, lock stores and row table as the full write of the original with only step i taken from the take; a
  refusal writes nothing. 31,884 checks in all (3,979 new); a planted fault (step i written from step i − 1's data)
  fails it.
- **mods/livetest-ar2 and tools/tests/45-live.sh** (in the Rytm suite): 21 checks pass, about 75 s.
  - BD gets trigs on 0, 4, 8, 12 and a lane-1 lock on 4; PLAY; then, from `ev_tick`, every step the playhead has
    passed is rewritten (its trig flipped, its velocity 20 + its rewrite count), loop after loop.
  - In the clock's interrupt (`ev_seq_step`, reads and counters only) every step the clock made is compared with
    the trig the record held: **no miss** in 10 loops at 120 BPM and 27 at 300 BPM, rewritten steps among them;
    the record ends as the start pattern flipped once per rewrite; the other twelve tracks are unchanged; step 4's
    lock survives its rewrites.
  - **NO while playing** (the snapshot restored): BD's record and lock store byte for byte as before the start,
    and no write after it. **Keep while playing**: the track stays as rewritten, no more writes.
- Rytm files 00 (golden vectors 129 the same), 01 and 13 pass.

## Findings

- **The step position: poll `core_track_step` in the tick** (`rt_playhead`). It agreed with the interrupt's view
  on every tick (the disagreement counter stayed 0). `ev_seq_step` reports the step the clock *made*, one ahead of
  the sound; it is the right place only for checks that must see each step as it fires. The live page needs no
  interrupt code.
- **What may be rewritten:** the steps from the last tick's playhead up to, not including, the current one. The
  clock has already made the step after the current one (`ev_seq_step`: "one step ahead of the sound"), so the
  current and next steps are never touched; the passed ones are made again only on the next loop, so a rewrite is
  heard there. The sequencer reads the pattern object rt_current gives (A01's 0x407dd8e3 here), so no copy is
  involved.
- **The tick keeps up at 300 BPM** (20 sixteenths a second against a 20 Hz tick): 439 rewrites in 440 ticks, the
  largest batch of passed steps in one tick 1. The batch loop handles more (a slower tick, a faster track speed:
  each passed step is rewritten in order).
- **Cost of one step's rewrite, measured with DMA timer 0 (DTCN0)**: average about 419,000 emulator instructions
  (about 2.1 ms at 200M instructions a second), largest about 661,000. Split by a probe: the read about 1 %, the
  room check under 1 %, the change notice about 4 %, **stock's step routines about 94 %** (setting and clearing
  the trig resets the step and re-sets its locks). At 300 BPM that is about 4 % of the UI task's time. The full
  `rt_seed_write` cost about 557,000 for the same change. The emulator cannot break in the mods' area
  ("hook outside the hook window"), so the mod times itself.
- `core_tempo` (0x40287ffc, BPM x 120) can be poked to set the tempo; the clock follows it at its next tick.

## The interface for the live page (T3)

In the frame's device part (mods/seedr-ar2/seedr.c), three calls:

| Call | Rytm |
|---|---|
| `sd_playhead(track, &length)`: the step playing now, -1 when stopped | `rt_playhead` |
| `sd_write_one(track, parm, take, i)`: step i only, as write writes it | `rt_seed_write_one` |
| the snapshot, restore, drop | the device's existing `snapshot`, `restore`, `drop` |

Each tick while live: `p = sd_playhead(t, &len)`; for each step from the last p up to p (wrapping), compute the
step's new value and `sd_write_one` it; remember p. Never write p or p + 1.

## Open points

- **Lag on a unit:** about 2 ms of stock routines a rewritten step. If hardware shows the UI lagging with several
  tracks live at high tempo, write only the fields that changed (the step routines are already called only for
  those) or rewrite at most N steps a tick and carry the rest (the batch loop allows it).
- **Track speeds and scale modes:** checked at 1x on one track; `core_track_step` follows each track's own speed
  and length, so other speeds should work the same way, but they are not in 45-live.
- **Pattern changes while live:** not exercised; the frame's existing rule (another pattern: NOT APPLIED) covers it
  in T3.
- **Interrupt safety of the writes:** the UI task's stock step routines run while the clock interrupt reads other
  steps; 45-live saw no miss and no fault over 37 loops with a rewrite every step, but the emulator's timing is a
  model. On-device testing decides.
