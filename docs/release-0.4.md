# trig-seedr 0.4

## 0.4.1

Two fixes for the hardware, nothing else changed. Build 0.4.1 to flash, not 0.4.0.

- **The compiler.** Homebrew's m68k-elf-gcc 16.2 miscompiles, at `-O2`, a struct copy followed by stores to its
  fields: one 4-byte store lands 6 bytes early ("store merging"). Found by task F5 (`docs/status/F5.md`), where a
  moved step lost its note and velocity and corrupted the step before it. Every ColdFire build (the mod, core-ar2,
  the `lib/` and `device/` checks) now uses `-fno-store-merging`. 0.4.0 was built without it; no emulator check
  failed, but the same optimisation may have touched code the checks do not reach.
- **New trigs keep stock's SYN / SMP / ENV bits.** A trig the mod placed on an empty step had the flag word
  0x0001 / 0x0011: the device layer cleared the bits (0x0380) that stock's place routine sets from the track's
  defaults. In the emulator the synth still sounded; whether the sample layer plays without them on a unit was not
  known. New trigs are now made as the TRIG keys make them.

## 0.4.0

0.4 has 0.3's features on version 1's structure (`design-v1.md`, `plan-v1.md` milestone M0). It is the base the
version 1 features (EUCLID, MAP, SYNC, value lists, noise, phrases, …) are added to in 0.5 to 0.9.

Status: every check of `tools/run_tests.sh` passes in analog-emu on the Mac (Homebrew m68k-elf-gcc 16.2). Not yet
built with the reference compiler in the Linux shell, and not yet run on a unit.

## Changed on purpose

- **What a seed gives** (design V11). A take is now made by version 1's engine: a random number per layer and step,
  keys for every share knob. Seed 5 in 0.4 is not seed 5 in 0.3. Kept patterns are ordinary pattern data and are not
  affected. From 0.4 on, the golden vectors (`lib/golden/`) hold what each seed gives.
- **Pages and navigation** (V10). LEFT / RIGHT step through TAKE, RHYTHM, PLACE, DYNAMICS, NOTE, LOCK, TRIG, TIME,
  VIEW and TOOLS. PAGE and the TRIG keys are stock's while trig-seedr is open (PAGE picks which 16 steps the TRIG keys
  show). Every knob and UP / DOWN are trig-seedr's while it is open.
- **One recipe per track, and YES renders all of it** (V2, V5). In 0.3 each page had its own YES (generate, groove,
  rotate). Now YES renders the whole recipe from the original track. The 0.3 pages map onto layers:

  | 0.3 | 0.4 |
  |---|---|
  | GEN 1 GRID, DENS | RHYTHM, SRC = RAND |
  | GEN 1 VEL, GEN 2 ACC, METER VSHP / VAMT | DYNAMICS VEL, ACC, SHAPE / AMT |
  | GEN 1 LEN, NOTE | NOTE |
  | GEN 1 UTIM, GROOVE RAND | TIME RAND (one knob, see below) |
  | GEN 1 SEED | TAKE SEED |
  | GEN 1 ROT, ROTATE (this track) | PLACE ROT; ROTATE's one-field mode is PLACE FLD / FROT / FMOD |
  | GEN 2 COND, RTRG | TRIG |
  | GEN 3 SMP | LOCK, lane 1 (parameter SMP) |
  | METER BEAT | TAKE BEAT |
  | METER SKIP, BIAS | PLACE SKIP, RHYTHM BIAS |
  | GROOVE | TIME; with SRC = TRACK it grooves the trigs on the track, as before |
  | ROTATE, scope ALL | TOOLS, ROTATE ALL |

- **SOURCE = TRACK** (V3, V6). A track with trigs starts on SRC = TRACK: the pages shape the trigs that are there
  (0.3's GROOVE and ROTATE did this; now every page can). An empty track starts on RAND. A page at 0 (or DEF) leaves
  its fields alone, so SRC = TRACK with every page at 0 writes nothing.
- **ROT comes after the source** (design section 4). It rotates the take's whole steps. In 0.3 the weights for BIAS
  and the velocity shapes were taken after ROT; now SKIP and the shapes still refer to where steps end up, and BIAS
  places trigs before ROT moves them.

## Knobs that changed

- **VEL starts at DEF** (the trig's velocity, or the track's default for a new trig), not 80–120.
- **RAND** (TIME) replaces GEN 1's UTIM and GROOVE's RANDOM: up to ±RAND % of the Rytm's ±23/24 step. 0.3's GROOVE
  RANDOM 100 (±12) is about RAND 50. It draws from the take's seed; GROOVE's own SEED knob is gone.
- **SEED** runs 0–9999; 0.3's FREE is gone (YES with no knob turned already takes the next seed).
- **BIAS** is steeper: at DENS 25 %, BIAS +60 puts about 80 % of the trigs on beats (0.3 reached about 57 % only at
  +100).
- **A velocity shape with VEL = DEF** runs from half the trig's velocity up to it.

## New

- **VARY** (TAKE): how much of take n the next seed keeps (0: unrelated, as 0.3; 100: the same take for every seed).
- **Two lock lanes** (LOCK), each on any of 26 parameters (SMPL, FLTR and AMP pages, LFO speed, fade, phase, depth).
  UP / DOWN picks the lane. Lane 1 starts on the sample slot, lane 2 on filter frequency.
- **SHAPE EVRY** (every N-th trig loud) and **HUM** (random velocity spread) on DYNAMICS.

## Recipes and patterns

Recipes live in RAM for the pattern in play: changing pattern starts new ones; power off forgets them (assumption
A3). A recipe is made when trig-seedr first shows the track: a track that was empty then stays on RAND after trigs
are recorded into it; turn SRC to TRACK to shape them.

## Not yet in 0.4

Hidden until their workstream lands: METR, ROLE, SHAPE RANK and AMOD TOP (F1), EUCLID (F2), MAP (F3), COPY and MASK
(F4), SYNC (F5), value lists and LEGATO (F6), NOISE (F7), DRFT (F8), PHRS and CVAR (F9), the preview of the take in the
bottom strip (F10; it shows the track as it is now), QUAL (F11).
