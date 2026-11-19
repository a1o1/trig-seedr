# S3b: the Syntakt mod's open points

State: ready for review
Branch: worktree-s3b-machines-st (from 77d6d8c)

## Done

- **The track's machine** (priority 1). The mod reads the current track's machine each time it asks for the
  current track: the kit (the application's +0x130) gives the track's model (`0x4000f04c`), the model its Sound
  (vtable +0x28), and the Sound's byte +0xa2 is the machine's type, as in `device/st_params.h` (Synfact's
  `recorder-st`, `save.c` and `native.h`, OS 1.42) [measured: track 1 BD MODERN 0, track 2 SD VINTAGE 6, which
  stock's SYN page shows; SD BASIC 1 and SY BITS 37 after a change in stock's machine menu]. The FX track has none.
  - LOCK lists the machine's SYN controls, then the shared pages (filter, base-width on tracks 1..8, amp, LFO);
    lane 1 starts on the machine's knob A, lane 2 on FRQ (SD4).
  - After a machine change, each recipe lane follows its stock parameter id into the new list
    (`st_machine_sync`). A lane on a parameter the new machine lacks makes YES and the strip say SETTING OUT OF
    RANGE until PARM is turned. That needs a frame hook (`SD_LANE_CHECK`, `#ifdef`, which the Rytm does not
    define): lib/ refuses an index out of range as NOT AVAILABLE YET, and lib/ is not this task's to change.
- **VIEW** (2): the retrig mark and the `R` field only on a device whose caps have retrig. The Rytm's caps do, so
  its view is unchanged.
- **LOCK's RNGE** (3): a new recipe's lanes start on their parameter's whole range (TUNE 40..88, FRQ 0..127), by a
  second frame hook (`SD_LANE_DEFAULTS`). The rule: the lane's parameter's `lo..hi` when the recipe is made; lib/'s
  1..8 is the Rytm's sample slots.
- **`lib/props.h`'s Syntakt profile** (4): its lock parameters are `st_params.h`'s, track 1 with SP TWINSHOT
  (the longest list, 33 entries), built at start-up by `st_track_params` as on the device. The placeholders, some
  with negative ranges, are gone; no Syntakt parameter has a negative range.
- **ROTATE ALL** (5): OS 1.42 has no whole-step shift a mod can call, so the device layer rotates the track's
  first `len` steps in a scratch copy and writes it back as a restore does (every step through stock's
  routines, then the bytes, then the notices): step i goes to (i + k) mod len, as `seed_rotate` does. Every
  per-step array of the record moves (flags, VEL .. NOT4, the unidentified +0x240 and +0x280) and each step's 72
  lock slots. The swing bit (0x0010) stays with its position. A track in stock's Euclidean mode is skipped.
- **The modified mark** (6): stock's `0x40018c84(app, 1)`, the Rytm's `0x400aed98` found again by its code
  (unique match with call and address operands masked). It keeps the mark in the settings byte `0x40653442`
  (through `0x4006f300`). `sd_edited` asks; the next tick, in the UI task, sets it.

## Tests

| Test | Result |
|---|---|
| `tools/st_s3b.sh` (analog-emu-st) | 28 PASS: machine types, LOCK's defaults and RNGE, DEC and FRQ locks in their slots, the machine change (FRQ follows, DEC refused, PARM turned writes again), ROTATE ALL +3 on tracks 1 and 2 against each dump row rotated, NO, the modified mark (0 at boot, 1 after a stock trig, 1 after FUNC + YES) |
| `tools/st_s3.sh` | 18 PASS, as S3 left it |
| `make -C device test` | `test_syntakt_st` 15,244 checks (2,001 rotation checks: a planted wrong direction fails 344); `test_st_params` 256,668 (every machine change on every track: pages found in the same row, a SYN control only where the new machine has it); `test_rytm_ar2` 27,905; all 0 failed |
| `make -C lib test`, `make -C frame test` | every program on both profiles, 0 failed; `test_interplay` Rytm 557,062, Syntakt 560,288 |
| Rytm `sh tools/run_tests.sh 00 01 10` | 42 PASS; golden vectors 129 the same |

## Memory

Core 456 + `.run` 53,788 + `.bss` 10,420 in the 64 KiB area: **836 bytes spare** (S3 left 2,320). The native heap
gives about 163 KB at the first tick (S3's 152 KB and ROTATE ALL's 10.6 KB scratch).

## For the manual (draft, Syntakt only)

- LOCK: PARM lists the track's machine's own controls first, then the filter, amp and LFO pages. After you change
  the track's machine, a lane on a control the new machine lacks says SETTING OUT OF RANGE until you turn PARM.
- RNGE starts on the whole range of the lane's parameter.
- The Syntakt has no retrigs: TRIG's retrig knobs and VIEW's retrig field are not shown.

## Open points

- **Stock's popups over the screen** (`sd_stock_overlay`) are still not found: seedr draws over a popup.
- **836 bytes spare** in the mods' area. The next feature on the Syntakt needs room: more data to the heap,
  or a smaller build.
- The machine read calls two stock routines (`0x4016e9c8`, `0x4000f04c`) and the Sound's vtable from the key, encoder,
  tick and draw paths (UI task), as Synfact's recorder does; not measured for cost.
- A machine change while seedr is open cannot happen from the keys (FUNC + SYN closes seedr, NOT APPLIED); a
  kit or pattern load can. Both go through the same sync.
- Whether the sequencer plays the rotated steps (the notices are sent as for a restore) is S4's playback check.
