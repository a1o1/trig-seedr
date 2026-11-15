# Contributing to trig-seedr

## Setup

Build and test as the README describes: elekloader, analog-emu and the stock OS 1.74 next to this repository, and an
m68k-elf cross compiler. Then, once per clone:

```sh
git config core.hooksPath .githooks      # checks each commit message (below)
pipx install commitizen                  # optional: `cz commit`, `cz bump`, `cz changelog`
```

## Before committing

```sh
make -C lib test coldfire                # host tests (sanitizers) and the ColdFire build check
sh tools/run_tests.sh                    # every check, host and emulator (docs/dev-environment.md for the variables)
```

Never commit Elektron's firmware or anything built from it (`build/`, `*.syx`, emulator snapshots and card images),
the unpacked sections the firmware study tools write, or groove tables made from Ableton's files
(`grooves-live.h`). `.gitignore` covers these; check `git status` anyway.

## Commit messages: Conventional Commits 1.0.0

Every commit message follows [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/):

```
<type>(<scope>): <subject>

<body>

<footer>
```

- **subject:** imperative, lower case, no full stop, about 70 characters at most.
- **body:** what changed and why, wrapped at 72; mention what was checked (host tests, emulator parts).
- **footer:** `BREAKING CHANGE: …` for anything that changes the keys, a page's knobs or the data written; or `!`
  after the type / scope.

**Types:** `feat` (a new page, knob or behaviour), `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`,
`chore`, `revert`.

**Scopes** (optional, one per commit):

| Scope | What |
|---|---|
| `lib` | the version 1 engine: `lib/seed.h` and the modules in `lib/` (a workstream may use its layer instead: `sync`, `map`, …) |
| `seedr` | the mod in general: keys, pages (`pages.c`), audition, drawing |
| `device` | `device/rytm_ar2.h`, the OS 1.74 layer |
| `groove2h` | the groove table tool |
| `tools` | other scripts in `tools/` |
| `test` | `tools/run_tests.sh`, `tools/tests/`, `emu.sh`, `seedref`, host tests |
| `docs` | README, `docs/` |
| `build` | Makefiles, `mod.json`, `.gitignore` |

Examples:

```
feat(seedr): add the TOOLS page
fix(device): clear a sample lock through stock's clear routine
docs(readme): add screenshots of the pages
test(seedr): check the lock-row refusal with a test build
```

The `commit-msg` hook in `.githooks/` rejects other messages (merges, reverts and fixups pass).

## Versions

The version is in `.cz.toml` and the README's Version badge (`cz bump` updates both from the commit history and tags
`vX.Y.Z`); keep `mods/seedr-ar2/mod.json`'s `version` (MAJOR.MINOR) in step by hand.

## Pull requests

One topic per pull request. Say what was checked: host tests, which emulator parts, and anything tried by hand in the
emulator's window or on a unit (with the unit's OS version).
