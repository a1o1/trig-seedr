# trig-seedr: notes for agents

Version 1 is being built to a written design and plan. Before changing anything:

1. Read `docs/dev-environment.md`. The workspace is shared between macOS and a Linux (Ubuntu, ARM64) shell; the cross compiler and the emulator binaries differ per system. Run its first-run checks (section 7) and report the result before building or testing.
2. Read `docs/design-v1.md` (the specification) and `docs/plan-v1.md` (tasks, file owners, gates).
3. Work only on the task you were given, in its own worktree and branch, in the files that task owns.
4. Follow the hard rules in `docs/plan-v1.md` section 2. In short: never commit Elektron firmware, anything built from it, or Ableton-derived groove data; never write the owner's email address anywhere; Conventional Commits; nothing goes to `main` except through the lead.

Environment and test commands: `docs/dev-environment.md`. What exists today (0.3): `README.md`, `docs/requirements.md`, `docs/firmware-notes-os174.md`, `tools/README.md`.
