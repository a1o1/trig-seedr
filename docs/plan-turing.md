# trig-seedr: plan for TURING (0.9.0)

Draft, 2026-11-20. Builds `docs/design-turing.md` to a candidate for on-device testing. plan-v1's hard rules and gates
apply. Alan asked for the work to run without his input until a candidate is ready on units; the lead decides and
documents (design L1–L12). Integration branch: `worktree-turing`; nothing goes to `main` or is released before a
pass on a unit.

| Task | What | Depends | Done when |
|---|---|---|---|
| T0a | Rytm: the step position live (core-ar2's `ev_seq_step` or a polled playhead) and a rewrite behind the playhead in analog-emu, from a test mod | — | a script shows steps rewritten behind the playhead while PLAY runs, heard on the next loop, nothing else changed. **Done 2026-11-20** ([`status/T0a.md`](status/T0a.md)): `rt_playhead` (polled), `rt_seed_write_one`, `tools/tests/45-live.sh` 21 checks; about 2 ms a rewritten step |
| T0b | Syntakt: the playhead (step position per track) on OS 1.42, polled from the tick | — | firmware notes; a script reads it in analog-emu-st while PLAY runs |
| T1 | The engine: `lib/src_turing.c`, `lib/scale.c`, the recipe fields (L10), `seedref`, golden vectors for the new recipes, props and fuzz | — | host tests on both profiles, golden: every old vector the same, new ones frozen. **Done 2026-11-20** ([`status/T1.md`](status/T1.md)): golden 145 the same (129 old, 16 new) |
| T2 | Offline UI on both devices: RHYTHM SRC TURG, NOTE SCALE / ROOT, VSRC TURG; emulator tests against seedref (Rytm `tools/tests/`, Syntakt `tools/tests-st/`) | T1 | the takes equal seedref's, field by field |
| T3 | The live TURING page (frame), Rytm first, then the Syntakt with T0b | T0, T1, T2 | the page's rules (L6, L7) checked in the emulator on each device |
| T4 | Docs (the manual's TURING page, device notes), version 0.9.0 (TS09), the full suites, candidate files and a hardware checklist for TURING | T2, T3 | Alan has the files and the checks |
