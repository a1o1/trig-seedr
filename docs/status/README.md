# Task status

One file per task, `docs/status/<id>.md`, kept current by the task's owner (`plan-v1.md` section 4). The lead reads
these before merging. A request to change something the task does not own (the contract `lib/seed.h`, the
framework, another workstream's files) goes in the status file's "Requests" section, not into the code.

## Template

```markdown
# <id>: <title>

State: not started | in progress | ready for review | merged (<commit>)
Branch: worktree-<name>

## Done
## Decisions taken
## How to test
## Open points
## Requests to the lead
```

## Branches and worktrees

Each task runs in its own worktree, `.claude/worktrees/<name>/`, on the branch `worktree-<name>`, started from
`origin/main` after the task's prerequisites are merged and pushed (`plan-v1.md` section 3). Only the lead merges to
`main`.

| Task | Worktree name | Branch | Starts after |
|---|---|---|---|
| 0.1 Contract | `m0-contract` | `worktree-m0-contract` | |
| 0.2 Engine port | `m0-engine` | `worktree-m0-engine` | 0.1 |
| 0.3 Page framework | `m0-pages` | `worktree-m0-pages` | 0.1 |
| 0.4 Device layer | `m0-device` | `worktree-m0-device` | 0.1 |
| 0.5 Test harness | `m0-harness` | `worktree-m0-harness` | 0.1 |
| 0.6 Integration | `m0-integration` | `worktree-m0-integration` | 0.2–0.5 |
| F1 Rank and meter | `f1-rank` | `worktree-f1-rank` | G0 |
| F2 EUCLID | `f2-euclid` | `worktree-f2-euclid` | G0 |
| F3 MAP | `f3-map` | `worktree-f3-map` | G0, A1, A6 |
| F4 COPY and MASK | `f4-copy-mask` | `worktree-f4-copy-mask` | G0 |
| F5 SYNC | `f5-sync` | `worktree-f5-sync` | G0 |
| F6 Value lists | `f6-lists` | `worktree-f6-lists` | G0 |
| F7 Noise | `f7-noise` | `worktree-f7-noise` | G0 |
| F8 Drift | `f8-drift` | `worktree-f8-drift` | G0 |
| F9 Variation | `f9-vary` | `worktree-f9-vary` | G0, A2 |
| F10 Preview and VIEW | `f10-preview` | `worktree-f10-preview` | G0 |
| F11 Candidate scoring | `f11-score` | `worktree-f11-score` | F1, design note |
| F12 MAP accents | `f12-map-accents` | `worktree-f12-map-accents` | F1, F3 |
| D1 Lock rows | `d1-lock-rows` | `worktree-d1-lock-rows` | 0.4 (delegated by the lead) |

## Files a task fills

Task 0.1 created every `lib/` module named in `plan-v1.md` section 4 as a stub: each function returns `SEED_E_NA`
and the file's `seed_have_<name>` flag is 0. An owner replaces the bodies in its own files and sets the flag to 1.
`lib/Makefile` builds every `*.c` in `lib/` and runs every `test_*.c`, so a new file (`map_data.c`,
`test_<module>.c`) needs no Makefile change.
