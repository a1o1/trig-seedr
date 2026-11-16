# trig-seedr: design for version 1

Status: agreed direction, 2026-11-15. Decisions V1–V11 are settled. Section 13 lists assumptions that still need Alan's answer; the plan (`plan-v1.md`) says which work each one gates. Background: `requirements.md` (what 0.3 does, D1–D21) and the research notes (project doc `research-generation-ideas.md`).

## 1. Goal

One coherent product that holds all nine researched ideas: preview, Barlow ranking and meters, accent flag from rank, syncopation, variation with memory, cross-track mask, value lists, noise contours, and the smaller items (roles, candidate scoring, drifting micro timing), plus a map source after Grids. Written for Elektron's sequencers in general; the Analog Rytm MKII is the first device.

## 2. Decisions

| # | Decision |
|---|---|
| V1 | All researched ideas are in scope for version 1, delivered in stages. Refactoring the 0.3 pages and engine is accepted; the base mechanics (write, audition, locks, keys) are proven. |
| V2 | **One recipe per take.** Every YES renders the whole chain from the original track. NO returns to the original. YES with no knob turned takes the next seed. YES means the same on every page. |
| V3 | **The generator is a layer that can be off: SOURCE = TRACK** uses the trigs that are there, and every later layer shapes or dresses them. |
| V4 | **A layer at 0 does not touch its fields.** |
| V5 | **A recipe per track.** |
| V6 | **Protective default.** A track that has trigs and no recipe yet starts on SOURCE = TRACK; an empty track on RANDOM. The title bar always shows the source (TRK or the seed). |
| V7 | Pattern-wide operations (ROTATE ALL) are a separate tool with its own YES. |
| V8 | **Elektron-sequencer generic.** Terms and keys common to Elektron's sequencers; device-only items become instances of generic ones (sample slot = a lock on a chosen parameter); a device states its capabilities and layers it lacks are not shown. |
| V9 | **No one-way doors for the A4 / Syntakt.** Every per-trig field takes the same value source (range, list with an order, contour), so note pools and pattern lists can be added to NOTE later. |
| V10 | **Navigation: LEFT / RIGHT move between layer pages.** PAGE returns to its stock job (which 16 steps the TRIG keys show), so pages 2–4 of a take can be seen while auditioning. The VIEW cursor moves to a knob. To verify in the emulator: stock PAGE and the TRIG keys behave normally while trig-seedr is open. |
| V11 | **Seeds change once, at 0.4.** The engine principles (section 3) change which take a seed number gives; kept patterns and every feature are unaffected. From 0.4 on, golden vectors hold seeds fixed. (Confirmed 2026-11-15.) |

## 3. Engine principles

| # | Principle | Why |
|---|---|---|
| P1 | **A random stream per layer.** Each layer draws from its own stream (seed mixed with the layer's id). | Turning COND % must not change the rhythm. |
| P2 | **Stable order.** Every share knob (DENS, COND %, RTRG %, lock %, ACC %, SYNC) gives each step a key and takes the top share. Raising the knob adds to what was there; lowering removes in reverse. | Knobs behave like Grids' fill; the preview is predictable. |
| P3 | **Stateless variation.** A take is (seed n, recipe). With VARY = k %, take n + 1 keeps k % of take n's keys and re-rolls the rest, computed from n alone (no stored history). | Related takes, phrases over pages and condition-carried variation all come from one mechanism. |
| P4 | Layer at 0 = untouched (V4). With a generating source the steps are new, so untouched means the default. | SOURCE = TRACK is safe. |
| P5 | **Guarantees are on the final result**: SKIP, MASK and the velocity shapes refer to where steps end up. | No knob can undo another's promise. |
| P6 | `lib/` is device-neutral C99, freestanding, integer only, no libc. | Host tests; later devices. |

**Keys (P2).** `key(step) = |BIAS| × rank(step) + (100 − |BIAS|) × random(step)`, rank reversed for negative BIAS. BIAS 0 is a random order, 100 the pure rank order.

**Variation (P3).** Each key belongs to a slot (layer, step, purpose). Slot s re-rolls at take k when `hash(k, s) mod 100 ≥ VARY`. Its random number at take n comes from the latest k ≤ n at which it re-rolled (walk back at most 64 takes). VARY 0: every take is unrelated (as 0.3). VARY 100: every take is equal.

## 4. The chain

| Order | Layer | Owns | Notes |
|---|---|---|---|
| 1 | RHYTHM | which steps have trigs | the only layer that creates trigs |
| 2 | PLACE | where steps end up | ROT → SYNC → MASK → SKIP, in that order |
| 3 | DYNAMICS | velocity, accent flag | |
| 4 | NOTE | length, note | |
| 5 | LOCK | parameter locks on chosen parameters | lanes; lock-row budget checked before writing |
| 6 | TRIG | trig condition, retrig | |
| 7 | TIME | micro timing | a groove's velocity amount scales velocity after DYNAMICS (the one shared field) |

A step carries its **origin**: the original step it came from, or none for a generated step. PLACE moves origins; the device layer then moves the whole step, all parameter locks included.

Change from 0.3: ROT is a plain rotation after the source (it moves a weighted rhythm off the beat). SKIP, MASK and the velocity shapes stay final-position.

## 5. Pages and knobs (provisional)

LEFT / RIGHT order. Knob letters may move; the page tables make that cheap. FUNC + knob sets the low end of a range.

| Page | A | B | C | D | E | F | G | H |
|---|---|---|---|---|---|---|---|---|
| TAKE | SEED | VARY | PHRS | CVAR | METR | BEAT | QUAL | |
| RHYTHM | SRC | by source, below | | | | | | |
| PLACE | ROT | SYNC | MASK | MTRK | SKIP | FLD | FROT | FMOD |
| DYNAMICS | VEL | SHAPE | AMT | N | ACC | AMOD | HUM | |
| NOTE | LEN | LMOD | NOTE | NSRC | ORDR | NLEN | ZOOM | |
| LOCK | PARM | % | RNGE | SRC | ORDR | LLEN | ZOOM | (UP / DOWN: lane 1, 2) |
| TRIG | COND | CTYP | CVAL | RTRG | RATE | RCUR | | |
| TIME | GRV | TIME | GVEL | QUANT | RAND | DRFT | SHFT | |
| VIEW | step cursor | | | | | | | |
| TOOLS | TOOL | STEPS | | | | | | (own YES) |

RHYTHM by source:

| SRC | Knobs | Meaning |
|---|---|---|
| TRACK | none | the trigs that are there |
| RANDOM | GRID, DENS, BIAS, ROLE | keys from rank and chance (section 3). ROLE picks the rank table: METER (Barlow), KICK, BACK, HAT, PERC |
| EUCLID | STEPS, PULS | evenly spread pulses; combines with every later layer and the audition |
| MAP | PART, DENS, X, Y, CHAOS | section 6 |
| COPY | TRK | the trig positions of another track |

Other knobs:

- **TAKE.** VARY: share of the previous take kept. PHRS: OFF (one take over the whole length), or a 16-step phrase over the pages (AAAA, AAAB, ABAB, ABAC, ABCD; a letter is a take offset). CVAR: variation written as trig conditions (OFF, 1:2, AAAB). METR: 4/4, 3/4, 6/8, 12/16, 5/8, 7/8. BEAT: steps per beat. QUAL: candidates scored per take.
- **PLACE.** SYNC: share of eligible trigs moved one grid step earlier, onto a weaker step (after Sioros and Guedes); the step's data moves with it. MASK: OFF, AVOID, ONLY against track MTRK. SKIP: the first N steps are cleared. FLD, FROT, FMOD: rotate one field against the trigs (0.3's ROTATE fields).
- **DYNAMICS.** VEL: DEF or a range. SHAPE: FLAT, STRONG, WEAK, OFF, NOT 1, NOTBAR, RANK, NOISE, EVRY, MAP. AMT: share of the shape. N: every N for EVRY, zoom for NOISE. ACC: share of trigs with the accent flag. AMOD: RAND, or TOP (the highest-ranked trigs). HUM: random spread.
- **NOTE.** LMOD: RANGE or LEGATO (to the next trig). NSRC: RANGE, LIST, NOISE. ORDR: UP, DOWN, PING, PNG2, ODD, RAND. NLEN: list length.
- **LOCK.** PARM: the parameter (the Rytm's default: sample slot). Two lanes in version 1.
- **TIME.** 0.3's GROOVE knobs plus DRFT (correlated drift).

## 6. MAP (after Grids)

- Data: 25 nodes on a 5×5 map, each 3 parts × 32 steps of levels 0–255 (`mi/eurorack/grids/resources.cc`, 2,400 bytes). X / Y blend the four nearest nodes.
- A step's key is its level plus a chaos offset (one random value per part per pass, a P3 slot). It fires when the key exceeds 255 − DENS. Steps beyond 32 repeat.
- DYNAMICS SHAPE = MAP uses the level as velocity, so the steps Grids accents are the loudest. ACC with AMOD = TOP reproduces Grids' accents: with SRC = MAP it ranks trigs by their key (the level plus the chaos offset, at most 255; Grids accents a trig whose key is above 192), so ACC at the share that counts those trigs sets the accent flag on exactly them (F12). With any other source TOP ranks by the meter (F1).
- PART and DENS are per track; X, Y and CHAOS are shared by the pattern's tracks (assumption A1).
- Licence: Grids' sources are GPL version 3 or later; trig-seedr and elekloader are GPL-2.0-or-later (assumption A6). Names: no "Mutable Instruments" and no "Grids" in the product; the mode is MAP, credited in the acknowledgements.

## 7. Variation

| Feature | From P3 |
|---|---|
| VARY | take n + 1 keeps a share of take n |
| PHRS | page p renders take n + offset(p) |
| CVAR | takes n and n + 1 rendered together: trigs in both are plain, the others get complementary conditions |
| CHAOS (MAP) | a slot per part per pass |

## 8. Screen and keys

- **Title bar:** the sprout, page name, track, the page's place among those shown (3/10); at the right the source: TRK, EUC, CPY, MAP (with CHAOS below 4, when it draws no random numbers), or S + seed for a source that draws from it (Alan, 2026-11-15).
- **Body:** two rows of four cells (label, value), as 0.3.
- **Bottom strip:** the preview: 64 steps × 2 px, bar height = velocity, the take as it would be written. It is rendered from the recipe on every knob change, before YES. Messages replace it for 2 seconds.

| Key | Does |
|---|---|
| LEFT / RIGHT | previous / next page |
| UP / DOWN | lane on LOCK; otherwise unused |
| knobs A–H | the page's settings; FUNC + knob: the low end of a range |
| YES | generate: render the recipe and write it to be heard (needs grid recording) |
| NO | back to the original; with no take pending, close |
| FUNC + YES | apply: the only way a take stays (assumption A4) |
| PAGE, TRIG keys | stock |

Opening and closing are the device's business (Rytm: the TRIG cycle after EUCLIDEAN, TRIG held + PAGE as a shortcut).

## 9. Generate and apply

**Generate and apply** (Alan, 2026-11-16, after the first hardware test). YES generates only with grid recording on (RECORD lit; live recording does not count); otherwise it writes nothing and says RECORD TO GENERATE, so browsing and the preview are always safe. The first YES snapshots the tracks in scope; each YES renders from the snapshot; NO restores it; FUNC + YES applies the take and drops the snapshot, the only way a take stays. Changing track or pattern, closing trig-seedr, or turning grid recording off with a take pending restores the snapshot (NOT APPLIED). With SOURCE = TRACK the snapshot is also the source. A pattern save during a take stores the take as it stands.

Unlike stock's Euclidean mode, which plays from its own table and never changes the stored trigs, trig-seedr must write trigs to be heard; hence the apply step. (Until 0.5, leaving kept a take, as 0.3's D11 and D18.)

## 10. Device layer contract

A device provides, for `lib/`:

- capabilities: tracks, steps, which fields exist (retrig, accent flag, micro-timing range), the lockable parameters with names;
- read any track (also for MASK and COPY) and write the track in scope, applying origins (whole-step moves with all locks), fields and lock lanes;
- snapshot / restore, the lock-row room check, the change notice;
- keys and drawing.

## 11. Invariants (each has a test)

| # | Invariant |
|---|---|
| I1 | Identity: SOURCE = TRACK with every layer at 0 leaves the track byte-identical, lock store included. |
| I2 | Determinism: same recipe, original and context give the same result. |
| I3 | Independence: changing one layer's settings changes only the fields that layer owns (P1). |
| I4 | Monotonic shares: raising a share knob keeps every step it had already chosen (P2). |
| I5 | Counts: DENS gives its exact count at the source; MASK and SKIP only remove. |
| I6 | Scope: only the tracks in scope change; the track's length is respected. |
| I7 | Generate and apply: NO, and leaving with a take pending, restore byte-identical state; FUNC + YES (apply) keeps the take. |
| I8 | Lock rows: a lock that cannot be stored is refused before anything is written. |
| I9 | Stock data: the result plays, saves, reloads and survives a power cycle on the stock OS. |
| I10 | VARY: at 0 consecutive takes are independent; at 100 identical; in between the share of kept keys matches. |

## 12. Not in version 1

- A4 MKII and Syntakt ports; note pools (chord, scale), chord sequencing.
- More than two lock lanes; LFO-shaped lock curves (the NOISE source covers contours).
- Recipes saved with the project (assumption A3).

## 13. Assumptions to confirm

| # | Assumption | Gates |
|---|---|---|
| A1 | MAP's X, Y and CHAOS are shared across the pattern's tracks; PART and DENS are per track. **Confirmed 2026-11-15.** | F3 |
| A2 | VARY, PHRS and CVAR as in sections 3, 5 and 7. **Confirmed 2026-11-15.** On a track shorter than four pages PHRS applies page by page as far as the track goes (F9 may propose otherwise). | F9 |
| A3 | Recipes live in RAM only (lost at power-off; the written steps are ordinary pattern data). | nothing |
| A4 | Keep stays FUNC + YES on the Rytm; the key is a device choice. | nothing |
| A6 | The map data lives in its own file under GPL-3.0-or-later and the README says a build with it is GPL-3.0-or-later. Alternative: move the repository to GPL-3.0-or-later. **Confirmed 2026-11-15: the data in its own file** (`lib/map_data.c`, its GPL-3.0-or-later header); the repository stays GPL-2.0-or-later. | F3 |
| A7 | Meter (METR, BEAT) is per track; a new recipe starts from the last one used. **Confirmed 2026-11-15** (as built in 0.4: `seed_recipe_init`). | F1 |
