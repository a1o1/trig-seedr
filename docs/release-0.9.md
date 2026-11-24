# trig-seedr 0.9

## 0.9.0 (2026-11-24)

0.9.0 adds TURING, a Turing machine for a track, on the Analog Rytm MKII and the Syntakt. It passed the full
emulator suites on both devices (Rytm 34 files, 558 checks; Syntakt 11 files, 165) and a hardware pass on a unit of
each: `flashing.md`'s and `flashing-st.md`'s checks 1 to 25, on 2026-11-24. The boot screen shows **TS09** on both
devices.

### TURING

A loop of LOOP steps (1–16), each step a trig and a value, that changes a little each time round: by an amount AMT
sets (0–100) and in a way FLAV sets. The values can drive notes, velocities and the LOCK lanes, so pitch and sound
change with the rhythm. The manual's page: TURING.

- **Three flavours:** **DJVU** (new steps below the middle, a locked loop at 50, the same steps shuffled above it),
  **CRPT** (values drift up to 50, trigs flip above it), **SHFT** (a shift register: locked at 100, inverted each
  pass at 0, a coin flip at 50). Credits in the acknowledgements.
- **TURG, the source** (RHYTHM SRC = TURG; FLAV, LOOP, AMT, BIAS, STRT): a whole take of the loop's passes down the
  track, deterministic as every source: SEED, VARY, the preview, YES, NO and FUNC + YES as before. STRT TRACK starts
  from the track's own first LOOP steps.
- **TURG as a value source** for NOTE, VEL and the LOCK lanes.
- **SCALE and ROOT** on NOTE (knob H; FUNC + the knob turns ROOT): 19 scales, for every note source. OFF changes
  nothing.
- **The live page TURING**, the last page after TOOLS: while the pattern plays, each step the playhead has just
  passed takes the loop's next change. YES starts; FUNC + YES keeps the track and stops; NO puts the original back;
  FUNC + UP remembers the loop, FUNC + DOWN brings it back. Closing Trig Seedr, changing track or pattern, or RECORD
  off puts the original back (NOT APPLIED), as for any take not yet applied.

### What to tell us

- How each flavour and AMT feel, offline and live.
- The live page at slow and fast tempos (the emulators ran 300 BPM: a step takes about 2 ms to write on the Rytm,
  0.3 ms on the Syntakt), on drums and melodic tracks, and whether the screen and keys stay responsive.
- Several tracks: only the track in view runs live (one machine); whether a machine per track is wanted.
- Leaving Trig Seedr stops the run and puts the original back (the fast path, design TD2): whether a run that keeps
  going while you play elsewhere is wanted.
- Track speeds other than 1x, and pattern changes while the page runs (not tried in the emulators).
- What YES while running should do (now: every step rewritten from the loop as it stands).
- On a Syntakt: FUNC + the SCALE knob for ROOT (the emulator cannot hold FUNC there).

### Also

- The Rytm and the Syntakt write one step at a time behind the playhead (`rt_seed_write_one`,
  `st_seed_write_steps`), and read their playheads (`rt_playhead`, `st_playhead`).
- YOLO does not roll TURING yet.
- **The Syntakt builds with the swing grooves only** (TIME's GRV): with TURING's code and the Live groove tables
  (`grooves-live.h`, local to your checkout) its unit file no longer fits, so `mods/seedr-st` leaves them out
  (`ST_LIVE_GROOVES` brings them back once the mod is smaller). The Rytm keeps them when they are there.
- 0.9.0 is the first Rytm build with a hardware pass since 0.7.1; it covers 0.8.0's frame shared with the Syntakt.

### Upstream used to build

| Project | Commit |
|---|---|
| elekloader | `9281c99` |
| analog-emu (the Rytm's emulator) | `2ac0f1f` |
| analog-emu, local branch `syntakt-modified` (the Syntakt's emulator; not upstream yet) | `19571f6` |
| synfact-dev (the Syntakt core's hook bus, elekloader's Syntakt profile) | `9db6680` |
| elektron-firmware-tool | `a5bce9a` |
