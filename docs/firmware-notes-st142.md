# Syntakt OS 1.42: step data, locks and the write API

Firmware study notes for a Syntakt device layer (docs/plan-syntakt.md, task S0.4), in the shape of
[`firmware-notes-os174.md`](firmware-notes-os174.md) for the Rytm. Stock file: `Syntakt_OS1.42.syx`, sha256
`ded3c59f…37c1`. Each finding is marked **[measured]** (in analog-emu running the stock OS: a snapshot read with
`tools/st_peek.py`, an edit made with stock keys, `tools/st_s04.sh`) or **[inferred]** (from the disassembly, not
yet confirmed by a measurement). Not run on a unit.

Sources: the stock OS (unpacked with elekloader `syx`, disassembled with capstone: `tools/unpack.py`,
`tools/cfdis.py`, in `build/`, never committed). Capstone does not decode ColdFire's `mvs`/`mvz` (it prints
`.byte 0x71, …`); they were decoded by hand. Synfact (`../external/synfact-dev`, GPL-2.0-or-later, by GSM1980)
named many of these routines first; each such finding says **(Synfact)** and where. analog-emu's
`docs/SYNTAKT-PORT.md`. No firmware or firmware-derived data is included in this repository.

## 1. Getting a track

```c
app     = APP();                                /* 0x4016e9c8: the singleton at *0x444e23f4 */
pattern = ACTIVE_PATTERN(app);                  /* 0x40019000: app + 0x3394 + 0x680 * index (0..127) */
track   = ACTIVE_TRACK(app);                    /* 0x40019036: pattern + 0x70 * (n + 1), n 0..12 */
data    = TRACK_DATA(track);                    /* vtable +0x28 (0x4017a22c): *(track + 0x10) */
length  = TRACK_LENGTH(track);                  /* 0x40027f3a */
locks   = LOCK_MODEL(pattern);                  /* 0x40016062: pattern + 0x620; its data *(+0x10) */
```

- **APP** (Synfact, `mods/arp-st/view.c`: 0x4016e9c8 and 0x40019036) [measured: `*0x444e23f4` = 0x445648c0 after a
  cold boot]. The UI controller is a separate singleton, `0x4016e044` → `*0x444e2334` (Synfact,
  `elekloader/sdk/include/syntakt/trig-pages.h`).
- **Patterns** are 128 objects of 0x680 bytes in the application, from +0x3394 [inferred from 0x40019000]; the
  active index comes from 0x400c2154 (0..127). The default project's A01 is index 0 [measured].
- **Tracks:** 13 (12 and the FX track, n = 12), clamped 0..12 by 0x40015fae; the active one is 0x4001ff74(app +
  0x30) [inferred]. A track object is 0x70 bytes: vtable 0x401c6e94 [measured], +0x10 the data record, +0x2c the
  pattern's lock model, +0x38 the track number, +0x3c the pattern object [measured for track 1].
- **The data:** a pattern's 13 track records are contiguous, 0x499 bytes apart, then its lock store, then the
  pattern's settings record (its name first: `ADRIANA`) [measured: A01's records at 0x40a39b34 + 0x499·n, the lock
  store at 0x40a3d6f9, the settings at 0x40a5ae94].
- **Change notice:** a step setter calls the track's vtable +0x10 with `{0x401c6b9c, step}` [inferred]; a lock
  routine calls the lock model's vtable +0x10 with `{0x401c48cc, track, step, id}` [inferred].

## 2. The data record (0x499 bytes per track)

Per step, 64 entries; the Rytm's order with NOTE moved after the conditions.

| Offset | Size | Field | Evidence |
|---|---|---|---|
| +0x000 | 64 × u16 | flag word per step | [measured] |
| +0x080 | 64 × u8 | VELOCITY (0xFF = track default) | setter 0x40028362 [inferred]; VEL turned on a held trig wrote it [measured] |
| +0x0C0 | 64 × u8 | LENGTH (0xFF = track default) | setter 0x400283dc [inferred] |
| +0x100 | 64 × s8 | MICRO TIMING −23..+23; the setter takes value + 64 | setter 0x40028456 [inferred] |
| +0x140 | 64 × u8 | RETRIG LENGTH (by the Rytm's order) | setter 0x40028574 [inferred] |
| +0x180 | 64 × u8 | RETRIG RATE (by the Rytm's order) | setter 0x400284ea [inferred] |
| +0x1C0 | 64 × s8 | RETRIG VELOCITY CURVE (by the Rytm's order) | setter 0x400285fe [inferred] |
| +0x200 | 64 × s8 | TRIG CONDITION, −1 none, 0..99 (section 3) | setter 0x40027dec [inferred] |
| +0x240 | 64 × u8 | 0xFF on a new trig; two setters (0x40028076, 0x4002810e). Probably the sound lock, as the Rytm's +0x280 | [inferred] |
| +0x280 | 64 × u8 | not identified (0 in the default pattern) | — |
| +0x2C0 | 64 × u8 | NOTE, MIDI note (0xFF = track default) | (Synfact, `mods/arp-st/chord.h`); setter 0x4002817a; NOTE turned on a held trig wrote it [measured] |
| +0x300 / +0x340 / +0x380 | 64 × u8 each | NOT2 / NOT3 / NOT4: interval + 64; 64, 0 and >127 are OFF; 0xFF default | (Synfact, `chord.h`); setters 0x400281f4, 0x4002826e, 0x400282e8 |
| +0x3C0 / +0x3C1 | u8 | track default VELOCITY (100), LENGTH (14) | [measured values; inferred meaning] |
| +0x3C5 | u8 | track default NOTE (60) | (Synfact, `chord.h`) [measured value] |
| +0x3C6..+0x3C8 | u8 | NOT2..NOT4 defaults (64 = OFF) | (Synfact) [measured values] |
| +0x3C9 | s16 | the track's length, in per-track scale mode | `mvs.w 0x3c9(a0)` in 0x40027f3a; 64 on A01 track 1 [measured] |
| +0x3D1 | u8 | Euclidean mode on (EUC) | [measured: knob D on the Euclidean page] |
| +0x3D2 | u8 | Euclidean PL1 | [measured: knob A] |
| +0x3D8 | 64 × u8 | the Euclidean pattern: per step the pulse's number, 0xFF none | [measured: PL1 12 on LEN 64 wrote 0..11 at steps 0, 6, 11, …] |

**Track length:** 0x40027f3a returns 16 for a track with no data; else, if the pattern's settings byte +0x19 is
non-zero (per-track scale), the s16 at +0x3C9; otherwise the pattern's length, the settings' s16 at +0x14
(0x40015784) [inferred; the per-track case measured]. Step setters refuse a step at or past the length.

**Flag bits:**
- 0x0001 a trig (note or lock trig) [measured: a trig key in grid recording].
- 0x0010 on every second step of the default pattern [measured]; the Rytm's swing bit.
- 0x0004, 0x0008, 0x0010, 0x0020, 0x8000 each have a test routine (0x40027360, 0x40027376, 0x400273a2,
  0x4002738c, 0x40027348) [inferred]; on the Rytm 0x4 mute, 0x8 accent, 0x10 swing, 0x20 slide.
- 0x0080, 0x0100, 0x0200, 0x0400: set on a new trig from four track predicates (0x40027846, 0x400279aa,
  0x40027a52, 0x40027b00), the trig's default switches [inferred]. A note trig of the default pattern is 0x0281.
- 0x0800, 0x1000, 0x2000, 0x4000: cleared from a step without a trig; per-step overrides of those switches
  [inferred]. A note trig is a trig whose 0x80 switch is on (its own if 0x800 is set, else the track's); a trig
  without it is a lock trig: 0x4002787c / 0x400278d8 (Synfact, `sdk/include/syntakt/pattern.h`, step present =
  either) [inferred].

## 3. Trig conditions

The setter stores −1 (none) or 0..99. The menu's names are a pointer table at 0x401d4764; condition code c is
entry 169 + c [inferred]:

| Code | Condition |
|---|---|
| 0..21 | 1% 2% 4% 6% 9% 13% 19% 25% 33% 41% 50% 59% 67% 75% 81% 87% 91% 94% 96% 98% 99% 100% (as the Rytm's) |
| 22..31 | FILL, !FILL, PRE, !PRE, NEI, !NEI, 1ST, !1ST, LST, !LST |
| 32, 33 | 1:2, 2:2 |
| 34..99 | A:B for B = 3..8, each followed by its negation: 1:3, !1:3, 2:3, !2:3, 3:3, !3:3, 1:4, … 8:8, !8:8 |

The table repeats each name for the negated entry (the "!" is drawn apart); the negation is inferred from that
layout. **The Rytm's codes differ from 30 on** (Rytm: 30..64 = A:B, no LST, no negated A:B), so a Syntakt device
layer translates `lib/seed.h`'s codes.

## 4. Stock routines (track object first)

| Routine | Address | Evidence |
|---|---|---|
| set / clear flag bits | 0x40029db4 (track, step, mask, on). Setting bit 0 on a step without a trig first resets the step (below); a step left without a trig is cleared through 0x4002868a and 0x4002802a | [inferred] |
| place a note trig | 0x40029f32 (track, step, on): bit 0 on, 0x800 and 0x1000 off | [inferred] |
| reset a step | 0x40029c74 (track, step, keep): NOTE, NOT2..4, VEL, LEN to 0xFF; micro timing 0; condition and +0x240 to 0xFF; retrig 14 / 12 / 0; the switch bits from the track's defaults; the flag word's bits 10..15 cleared | [inferred]; a new trig's fields [measured: T5] |
| NOTE, NOT2, NOT3, NOT4 | 0x4002817a, 0x400281f4, 0x4002826e, 0x400282e8 (track, step, value) | (Synfact, `chord.h`) |
| VEL, LEN | 0x40028362, 0x400283dc (track, step, value) | [inferred] |
| MICRO TIMING | 0x40028456 (track, step, value + 64) | [inferred] |
| CONDITION | 0x40027dec (track, step, −1..99) | [inferred] |
| RETRIG length, rate, velocity curve | 0x40028574, 0x400284ea, 0x400285fe | [inferred] |
| +0x240 (sound lock?) | 0x40028076, 0x4002810e | [inferred] |
| track length | 0x40027f3a (track) | (Synfact, `pattern.h`) |
| lock set (model level) | 0x40017602 (model, track 0..12, step 0..63, id 0..70, value ≥ 0) | (Synfact: LFO3 hooks it, `mods/lfo3-st/main.c`) |
| lock set (track level) | 0x40027fa2 (track, step, id, value): refuses step ≥ length, calls 0x40017602 | [inferred] |
| lock clear (model level) | 0x4001741e (model, track, step, id): writes −1 | [inferred] |
| lock get | 0x4001722a (model, track, step, id), −1 none | (Synfact, `mods/arp-st/view.c`) |
| lock set / get / clear through a parameter view | 0x400387b4, 0x40037942, 0x400379a8 | (Synfact, `pattern.h`) |
| step has locks / a lock of id | 0x400273b8 → 0x40017164; 0x400273e0 → 0x4001705c | [inferred] |
| flag-masked rotate along the length | 0x40029b2c (track, …, mask, length) | [inferred]; the whole-step shift trig-seedr's ROTATE ALL needs is not found yet |

## 5. Parameter locks

- **A full matrix, no row pool.** Per track 0x2447 bytes: 64 steps × 72 int16 (ids 0..70 by sound parameter id,
  −1 = none; word 71 the step's lock count), then 71 bytes, one per id [inferred from 0x40017602; measured for one
  lock]. Address of (track, step, id): `locks + 0x2447·track + 2·(72·step + id)`.
- **No 72-row limit:** nothing in the set routine counts rows; every step can hold a lock of every id [inferred]. A
  Syntakt device layer has no room check to make (the Rytm's NO LOCK ROWS LEFT does not arise) — to be confirmed by
  setting many locks in the emulator.
- **Measured:** T1 held in grid recording on the SYN page, knob A turned: track 1, step 1, id 18 = 0x40DE; the
  step's word 71 = 1; the track's byte for id 18 (+0x2400 + 18) = 1. The value's scale per parameter, and which
  parameter each id is, is task S0.5's.
- Lock set updates the step's count (0x4001729e) and the track's id byte (0x400175c2); clear recomputes them
  (0x4001729e, 0x40017376) [inferred]. The id byte reads as "the track has a lock of this id"; whether it is that
  or a fine flag is not measured.
- Synfact's LFO3 and ARP use ids 0x58..0x67 for their own locks (`mods/lfo3-st/ui.c`, `mods/arp-st/locks.h`); stock
  ids end at 70 (0x46), so those are stored outside this matrix (Synfact's extension).

## 6. Grid recording, the TRIG page, the Euclidean mode

- **Grid recording** [measured]: the sequencer's selection object (UI controller `*0x444e2334` + 0x40 → node
  +0x1c → main screen +8 (vtable 0x401c849c) → +0xc0 → +0x6c) has +0x161 = 1 (active) and +0x168 = 1 while RECORD
  is lit, 0 after boot; +0x16a is live recording (RECORD + PLAY) (Synfact, `mods/arp-st/view.c`). The object is on
  the heap (0x445f94c0 after a cold boot), so a device layer follows the chain; no fixed byte as on the Rytm.
- **The TRIG page** [measured]: the view at main screen +0xc0 has vtable 0x401c9a9c and +0x88 = 0x13 while the
  TRIG page shows; +0x90 is the sub-page, 0 Trig Parameters, 1 Euclidean, stepped by the TRIG key. The page list is
  a vector at +0x7c..+0x80 (page ids {2, 3} on a digital track, {1, 3} analog, {26, 3} MIDI) (Synfact,
  `trig-pages.h`: its POLY and ARP add pages to that vector).
- **Euclidean mode** [measured]: +0x3D1 EUC, +0x3D2 PL1, the table at +0x3D8. Whether stock leaves the flag words
  as they are with EUC on (as the Rytm does) is not measured.

## 7. Saving

- **The working state** is written as a 'COKI' block to raw eMMC sectors 0x7a000 / 0x7c000, as on the Rytm: the
  constants and the writer are in the OS (0x400bd608..0x400bd74c) [inferred].
- **In analog-emu** `--power-off` runs the OS's handler, which parks the card after 3.0M instructions and writes 0
  sectors (`docs/status/S0.2.md`) [measured]. So the trigger differs from the Rytm's (another power-sense pin, or
  the supercap gate analog-emu models as charged); analog-emu's power-off is to be adapted. Whether a unit keeps
  an edit across a power cycle is Alan's check (S0.2's open point).
- Project save: the "Project Save" / `SAVING '%.16s'` strings exist (0x402500c5); the save path is not traced.

## 8. Open points

- Retrig length / rate / velocity: the field order is the Rytm's, but a new trig gets 14 / 12 where the default
  pattern holds 46 / 9; the encodings are not measured.
- +0x240 and +0x280 not identified.
- The whole-step shift (for TOOLS' ROTATE ALL) not found.
- The flag bits 0x4 / 0x8 / 0x20 / 0x8000 and the switch bits named by analogy, not measured.
- The lock value scale per id; the meaning of the per-track id bytes.
- The power-off trigger in analog-emu.
