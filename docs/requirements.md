# trig-seedr: requirements (v1.0, 2026-11-14)

Scope: offline, destructive pattern algorithms (applied to a track on a key press, with an audition undo) for the Analog Rytm MKII OS 1.74, mono only, as an elekloader mod. Later devices: Analog Four MKII, Syntakt.

## Decisions

| # | Decision |
|---|---|
| D1 | v1 algos: **GENERATOR** and **ROTATE**. CURVES is out of v1. |
| D2 | **Generate into the track's existing length** (the pages already configured). Changing the length or page count is a later step. |
| D3 | **Trig conditions and retrigs are part of GENERATOR's output**, not a separate algo. |
| D4 | **No chords.** Mono only. On the A4 the target is 4 mono voices, so a mono generator carries over; chords to be revisited. |
| D5 | **SYMMETRY dropped.** |
| D6 | **SHUFFLE deferred.** Micro timing matters more. |
| D7 | **ROTATE added**, mainly to move a groove against the rhythm (rotate some fields while the trigs stay put), plus whole-pattern rotation, which stock lacks. |
| D8 | **Groove-driven micro timing** (an Ableton groove driving µtime and velocity): see D12–D14. |
| D9 | **EVERY N dropped**, to keep v1 simple. GENERATOR's ACC and VEL cover accents. |
| D10 | **Device order: Rytm MKII (OS 1.74, elekloader core-ar2) first.** A4 MKII second. Syntakt (SyntHack / MAX, OS 1.41) after, in coordination with those authors. |
| D11 | **Undo is an audition.** The first APPLY snapshots the original; each further APPLY regenerates from that original; NO reverts to it; KEEP (FUNC + YES) accepts the result and drops the snapshot. |
| D12 | **Grooves come from Ableton .agr files (and MIDI files exported from Live) via a host tool** (`tools/groove2h.py`), built into the firmware as tables. Adding grooves means a rebuild. |
| D13 | **GROOVE is its own page**, on any track (hand-programmed or generated), with the D11 audition. YES on GROOVE while a GEN take is auditioned keeps the take first, so NO undoes only the groove. |
| D14 | **Timing and velocity, separate amounts**, after Live's Groove Pool (plus QUANT and RANDOM). |
| D15 | **YES regenerates.** A YES with no knob turned since that page's last YES moves to the next seed (a new take, also right after FUNC + YES); after a knob turn it keeps the seed, so the change is heard on the same take. |
| D16 | **Sample-slot locks per trig** (GEN 3): a share of the generated trigs get a p-lock of the sample slot, from a slot-number range (low–high). |
| D17 | **Open / close with TRIG held + PAGE.** Stock 1.74 uses every FUNC + key combination (FUNC + PLAY MODE is its KIT menu); TRIG held + PAGE does nothing in stock (checked in analog-emu). |
| D18 | **Leaving mid-audition keeps the take.** Changing track or pattern, or closing the page, while a take is auditioned keeps it, as FUNC + YES would. |

## 1. Algorithms

| Algo | What | Here |
|---|---|---|
| GENERATOR | random trigs on a grid at a density, with velocity, length, note, micro timing, accents, conditions, retrigs and sample locks from ranges | v1 |
| ROTATE | moves whole steps or one field, on a track or the whole pattern | v1 |
| GROOVE | a groove's micro timing and velocity on the existing trigs | v1 |
| CURVES | waveforms onto a field or a parameter | later |
| SHUFFLE | random reordering of steps or fields | deferred |
| SYMMETRY, EVERY N | mirrored patterns; accent / ghost every N steps | dropped |

## 2. What a Rytm step can hold

Field offsets, the stock routines that write them, parameter locks and the lock budget: `firmware-notes-os174.md`.

## 3. GENERATOR

Region: the track's current length as the sequencer plays it. All of it is replaced (each step cleared, then written); other tracks are untouched. `lib/gen.c`.

**GEN 1: rhythm and notes**
| Knob | Name | Range | Meaning |
|---|---|---|---|
| A | GRID | 1/16, 1/8, 1/4, 1/2 | trigs land only on multiples of the grid |
| B | DENS | 0..100% | share of grid positions that get a note trig (exact count) |
| C | VEL | 1..127, FUNC: min | velocity range |
| D | LEN | stock's stepped lengths, FUNC: min | note length range, shown as note values (1/16, 1/8 …) or in steps; DEF leaves the track default |
| E | NOTE | -24..+24, FUNC: min | offset from the track's default note (0..0: leave the default) |
| F | UTIM | 0..100% | random micro timing amount; 0 = on the grid |
| G | SEED | FREE, 0..9999 (FUNC: ±100) | the random source |
| H | ROT | -N..+N steps | rotate the result before writing |

**GEN 2: Elektron extras (D3)**
| Knob | Name | Range | Meaning |
|---|---|---|---|
| A | ACC | 0..100% | share of generated trigs that get an accent |
| B | COND | 0..100% | share of generated trigs that get a condition |
| C | CTYP | X%, A:B, 1ST, FILL, MIX | which conditions COND may use |
| D | CVAL | depends on CTYP | X%: probability range; A:B: the cycle (2..8) |
| E | RTRG | 0..100% | share of trigs that get a retrig |
| F | RATE | 0..16, FUNC: min | retrig rate range |
| G | RCUR | -128..127 | retrig velocity curve (stock's retrig VEL: negative fades the repeats out, positive in) |

**GEN 3: locks (D16)**
| Knob | Name | Range | Meaning |
|---|---|---|---|
| A | SMP% | 0..100% | share of generated trigs that get a sample-slot lock. 0: the track's sample locks are left as they are; above 0: GEN replaces them |
| B | SMPL | 0..127, FUNC: min | the slot range a locked trig draws from (0 = OFF) |

GEN refuses ("NO LOCK ROWS LEFT") when SMP% > 0, the track has no sample-lock row yet and the pattern's 72 lock rows are full (a new lock would be lost at the next save).

## 4. ROTATE

Moves step data by STEPS with wrap-around at each track's length. `lib/gen.c` (fields) and stock's trig shift (whole steps).

| Knob | Name | Range | Meaning |
|---|---|---|---|
| A | STEPS | -63..+63 | how far; negative = earlier |
| B | SCOPE | TRACK, ALL | the current track, or every track of the pattern (tracks in Euclid mode are skipped) |
| C | WHAT | ALL, VEL, NOTE, LEN, UTIM, COND, RTRG, ACC | ALL = whole steps through stock's trig shift: fields and parameter locks move, swing bits stay. A single field moves while the trigs stay put |
| D | MODE | TRIG, GRID | a single field moves between trigs (nothing lost) or by steps (values landing on empty steps are dropped). ALL and ACC always move by step |

Not in v1: PAGE scope, moving the locks alone, aligning tracks of different lengths to the master length.

## 5. GROOVE (D12–D14)

Applies a groove's micro timing and velocity to the trigs of the current track. `lib/groove.c`; tables from `tools/groove2h.py`.

**Groove tables.** One entry per Rytm step (a 1/16 at speed 1×), repeating every `len` steps (up to 64): µtime at 100 % in 1/24-step units (1/96 beat), and the groove's velocity at that step (0 = no groove note there).

**From an .agr.** Live 11's plain `.agr` is XML: a MIDI clip (note times in beats, velocities, loop length) and the Groove's Grid (Base: 1 = 1/8, 3 = 1/16, checked against the files' names).
- Each base-grid line takes the displacement of the groove note nearest to it (within half a grid line); steps between lines move in proportion (a piecewise-linear time warp, as Live describes Base); a step on a grid line with a note takes that note's velocity.
- Check: Live's "Swing MPC 3000 16ths 54" puts every second 16th 0.0209 beats late = 2 units, the MPC formula's value for 54 %.
- Some `.agr` files are in a protected format (header `ab 1e 56 78`); the tool skips them. Route for those: Live itself (drag onto a MIDI track, export the clip as MIDI).

**From a MIDI file.** Each note-on is a groove note. Base from `--base` or the file name (`8ths` → 1/8, `16T` → 1/16T, otherwise 1/16); length from `--beats` or the file's end rounded up to whole bars.

| Knob | Name | Range | Meaning |
|---|---|---|---|
| A | GRV | 1..N | the groove; its name is on the bottom line |
| B | TIME | -100..+100 % | share of the groove's µtime added to each trig |
| C | VEL | -100..+100 % | at +100 % a trig's velocity is scaled by the groove velocity ÷ the groove's loudest; negative inverts |
| D | QUANT | 0..100 % | existing µtime pulled toward 0 first |
| E | RAND | 0..100 % | random µtime, up to ±12 units (half a step) at 100 % |
| F | SHIFT | -63..+63 | the groove moved later / earlier against the pattern |
| G | SEED | FREE, 0..9999 | for RAND |

Only steps with a trig change; µtime is clamped to ±23. Limits: a step is taken as a 1/16 whatever the track's speed; triplet grooves and odd meters are warped onto 16ths as they come; grooves longer than 64 steps are cut to 64.

## 6. Behaviour requirements

- **R1 Region.** The track's existing length, all pages (D2). ROTATE also has the ALL scope.
- **R2 Undo (audition, D11).**
  - The first APPLY copies the affected tracks: each track's step record (917 bytes) and lock store (5,674 bytes); all 13 tracks for ROTATE ALL (86 KB, in the mod's .bss).
  - While auditioning, every APPLY starts from that copy, so results never build on each other.
  - APPLY on another page, or over other tracks, keeps the current audition first; the new one starts from that result.
  - NO restores the copy; FUNC + YES keeps the result; leaving the page, the track or the pattern keeps it (D18).
  - Restore: locks that differ go back through stock's lock set / clear (so the stored lock rows follow), then the bytes through stock's memcpy, then the change notice (the grid redraws).
  - RAM only. Stock RELOAD PATTERN (NO + SCENE) stays as a fallback to the saved state.
- **R3 Seed.** Same seed + settings + length = same result.
- **R4 Live.** Works stopped or playing; the result plays from the next step.
- **R5 Lock budget.** The trig-page fields are per-track arrays, not lock rows. GEN 3's sample locks use one row per track; GEN checks for room first. GROOVE adds no rows; ROTATE moves existing locks with whole steps.
- **R6 Stock-compatible output.** Everything written is ordinary Rytm data: it plays and saves on a stock OS.
- **R7 UI.** One page per algo, knobs A..H + YES; opened with TRIG held + PAGE (D17).
- **R8 Safety.** Touch only the tracks in scope; mark the project modified after APPLY.
- **R9 Euclid mode.** GENERATOR, GROOVE and ROTATE (TRACK) refuse on a track in Euclid mode; ROTATE ALL skips such tracks.

## 7. Where the pages live

| Option | How | Status |
|---|---|---|
| A. Behind the TRIG key, after EUCLID (target) | TRIG page 1 → EUCLID → GEN 1 … | needs a core-ar2 contribution: `core_view_pages` does not cover the TRIG view |
| B. Mod-drawn page opened by a key combination (now) | `ev_key` opens it, `ev_draw` draws it, `ev_enc` takes the knobs | TRIG held + PAGE (D17) |

## 8. Open

1. The TRIG view's page list (option A).
2. A first run on a unit.
3. A4 MKII: FUNC + YES / NO are pattern save / reload there, so KEEP needs another key.

## 9. Later

- GEN 3: more lock targets (any sound parameter, a range per trig).
- Range knobs without FUNC (push to switch low / high, or separate MIN / MAX), if FUNC + turn proves awkward.
- Grooves: track speed taken into account; a groove from a Rytm pattern; grooves loaded at run time instead of built in.
- ROTATE: PAGE scope, locks alone.
- SHUFFLE (D6), CURVES including curves onto parameter locks.
- Generating the length (NORMAL scale shares one length across tracks; per-track length needs ADVANCED scale).
- A4 MKII, then Syntakt (D10).
