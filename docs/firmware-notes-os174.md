# Analog Rytm MKII OS 1.74: step data, locks and the write API

Firmware study notes behind trig-seedr's device layer (`device/rytm_ar2.h`). Everything here was checked in analog-emu running the stock OS 1.74, unless marked otherwise. Not yet run on a unit.

Sources: libanalogrytm `pattern.h` (the pattern SysEx format), rytm-rs, sidecar-mods `re/symbols.toml` (OS 1.73 addresses), elekloader `mods/core-ar2`, the stock OS (unpacked with elekloader, disassembled with capstone, `tools/unpack.py`, `tools/cfdis.py`). No firmware or firmware-derived data is included in this repository.

## 1. Getting a track (the handles core-ar2 uses)

```c
app   = APP();                                   /* 0x4017fa0a */
pat   = CURRENT_PATTERN(app);                    /* 0x400af0ac */
n     = CURRENT_TRACK(APP_TRACKS(app));          /* 0x400b693e(0x400ae492(app)), 0..12 */
track = TRACK_REC(pat, n);                       /* 0x400ab29e: pat + 0x70 + 96*n */
```

`track` is a 96-byte object with a vtable: +0x28 returns the data record, +0x10 sends a change notice. Fields: +0x2c the pattern object (its +0x28 returns the pattern's data), +0x40 the track number. `TRACK_LENGTH(track)` is 0x400c0524.

In the emulator the record of track t in live pattern p is at `0x407dd8e3 + 0x14F00·p + 0x395·t` (A01 BD = 0x407dd8e3); the pattern's lock store follows the 13 records, at +0x2e91 + 0x162a·t.

## 2. The data record (0x395 bytes per track)

| Offset | Size | Field |
|---|---|---|
| +0x000 | 64 × u16 | flag word per step (note trig 0x0381; default swing trigs 0x0010 on every 2nd step) |
| +0x080 | 64 × u8 | NOTE (0xFF = track default) |
| +0x0C0 | 64 × u8 | VELOCITY (0xFF = track default) |
| +0x100 | 64 × u8 | LENGTH (0xFF = track default) |
| +0x140 | 64 × s8 | MICRO TIMING −23..+23 |
| +0x180 | 64 × u8 | RETRIG LENGTH 0..127 (new trig 46) |
| +0x1C0 | 64 × u8 | RETRIG RATE 0..16 (new trig 9 = 1/16) |
| +0x200 | 64 × s8 | RETRIG VELOCITY CURVE −128..127 |
| +0x240 | 64 × s8 | TRIG CONDITION −1 (none)..64 |
| +0x280 | 64 × u8 | SOUND LOCK (0xFF = none) |
| +0x2C0 / +0x2C1 / +0x2C2 | u8 | track default NOTE (60) / VELOCITY (100) / LENGTH (10) |
| +0x2C5 | u8 | track length |
| +0x2C7 | u8 | track speed (2 = 1x) |
| +0x2CA | u8 | Euclid mode on |
| +0x2CB / +0x2CC | u8 | Euclid PL1 / PL2 (pulses) |
| +0x2D1 | 64 × u8 | the pattern Euclid mode plays: per step the pulse's number, 0xFF none (PL2 and OP combined); entries past the length keep old values. Euclid mode leaves the flag words as they were (F2, `tools/tests/22-euclid.sh`) |

**Flag bits:** 0x0001 trig · 0x0002 retrig · 0x0004 mute · 0x0008 accent · 0x0010 swing · 0x0020 slide · 0x0080 / 0x0100 / 0x0200 / 0x0400 SYN / SMP / ENV / LFO state · 0x0800 / 0x1000 / 0x2000 SYN / SMP / ENV lock on.

**Encodings.**
- LENGTH: stepped scale, in 1/16 of a step: codes 0..14 → 2..16 (by 1), 14..30 → 16..32 (by 1), 30..46 → 32..64 (by 2), 46..62 → 64..128 (by 4), 62..78 (by 8), 78..94 (by 16), 94..110 (by 32), 110..126 (by 64); 127 = INF. So 14 = 1/16, 30 = 1/8, 46 = 1/4, 62 = 1/2, 78 = 1/1, 126 = 8/1.
- MICRO TIMING: 1/24 of a step; the engine runs at 600 ticks per step (25 per unit).
- RETRIG RATE: 0 = 1/1 … 9 = 1/16 … 16 = 1/80.
- TRIG CONDITION: 0..21 = 1%..100%, 22 FILL, 23 !FILL, 24 PRE, 25 !PRE, 26 NEI, 27 !NEI, 28 1ST, 29 !1ST, 30..64 = A:B 1:2..8:8 (code 29 + B(B−1)/2 + A − 1).

## 3. Stock routines used (track object first; steps past the length are refused)

| Routine | Address |
|---|---|
| place / remove note trig | 0x400c2620 (track, step, on) |
| set / clear flag bits | 0x400c2482 (track, step, mask, on) |
| NOTE, VEL, LEN | 0x400c0776, 0x400c07f0, 0x400c086a (track, step, value) |
| MICRO TIMING | 0x400c08e4 (track, step, value + 64) |
| CONDITION | 0x400c03d6 (track, step, −1..64) |
| RETRIG length, rate, velocity curve | 0x400c0a02, 0x400c0978, 0x400c0a8c |
| parameter lock set | 0x400c058c (track, step, sound index, value 8.8, fine flag) |
| parameter lock clear | 0x400c05e2 (track, step, sound index) |
| trig shift | 0x400c1d7a (track, ±1, −1): one step a call, wraps at the length, carries every field and the step's 88-byte lock record, swing bits stay |
| sound lock set / clear | 0x400c0670 (track, step, value ≤ 128; voice tracks only, otherwise it stores 0xFF) / 0x400c070a (track, step); both send the step's change notice |
| memcpy | 0x40098bc0 |

Removing a trig through the stock routine leaves the SYN/SMP/ENV state bits on the step, as removing it by hand does.

**The flag routine resets a step left without a trig.** 0x400c2482 ORs or clears any mask; then, whenever the step
is left without the note trig (bit 0), it calls 0x400c0b18 (note, velocity, length to the default, micro timing 0,
retrig 46 / 9 / 0), 0x400c0624 (every parameter lock of the step, through the pool's 0x400ac86e) and 0x400c070a (the
sound lock to 0xFF). So a write sets the flags first, then the fields, then the locks (device/rytm_ar2.h,
rt_seed_write_step). The lock set / clear routines do not touch the flag word; their length check lets step ==
length through (`blt`), so the device layer never writes there.

## 4. Parameter locks

- **Live store:** per track 5,674 bytes: per step 88 bytes = 44 int16 values by sound index (−1 = none), stored as value × 256; then a fine-flag byte per sound index at +5632.
- **Stored rows:** the pattern's saved copy keeps locks in 72 rows of (type, track), 66 bytes each, behind the lock pool (track object +0x2c) at +0x20. Sound index → row type for voice tracks: table 0x401c5be4 (its inverse 0x401c5b4c). A new row is taken while used + cost ≤ 71 (cost 2 for a fine parameter); past that a new lock lives only until the next save, without a warning.
- **Sample slot:** sound index 19, value slot × 256, 0 = OFF.
- **Sound index → parameter, voice tracks** (the parameter records at 0x401bfb2c, 0x34 bytes each, +0x04 the
  index): 1..8 LFO (MUL, WAV, MOD, DST, SPD, FAD, SPH, DEP); 9..16 the SYNTH page (per machine; 9 is LEV); 17..24
  SAMPLE (TUN, FIN, SMP, BR, STA, END, LOP, LEV); 25..32 FILTER (ENV, ATK, DEC, SUS, REL, FRQ, RES, TYP); 33..41 AMP
  (ATK, HLD, DEC, OVR, VOL, PAN, ACC, DEL, REV). Words 42 and 43 of a step's 88 bytes are not locks. The lanes'
  parameter list is device/rytm_params.h (task 0.4).
- Checked (task 0.4, tools/tests/11-device.sh): a step with three FLTR locks made by hand, moved one step through
  the lock set / clear routines, keeps all three after SAVE PATTERN + RELOAD PATTERN and after a power cycle.
- Checked: sample locks written through 0x400c058c survive SAVE PATTERN + RELOAD PATTERN (so their row is stored), play the locked slot's sample, and move with whole steps under the trig shift.

## 5. Other findings

- **Keys:** stock 1.74 uses every FUNC + key combination (FUNC + PLAY MODE opens the KIT menu). TRIG held + PAGE does nothing in stock.
- **The TRIG page:** its view is a ParametersSeqNoteView (vtable 0x401e3148); +0x8c is the sub-page, 0 TRIG, 1 EUCLIDEAN, cycled by the TRIG key (0x4003ab90). While the page shows, the view is on the view controller's list (ev_tick's ctrl: +28 the first node, +20 the end; a node's +0 the next, +8 its view). Coming back to TRIG from another page within about half a second keeps the sub-page; later it starts at TRIG. FUNC + YES / FUNC + NO do nothing on the Rytm main screen (pattern save / reload are YES / NO + SCENE).
- **Knobs:** `ev_enc` gives the knob as 0..7 (A..H) and raw encoder counts. Stock ignores the first two turn events of a knob idle for ~2 s and takes 4 counts per value step.
- **Modified mark:** byte 0x405c8e4e; 1 on a fresh, never-saved project, cleared by a project save, set again after an edit.
- **Sidecar's OS 1.73 addresses** near 0x400c0000 sit 0x6c later in 1.74; its data tables near 0x401c5000, 0x218 later. Each address used here was checked in the 1.74 disassembly.
