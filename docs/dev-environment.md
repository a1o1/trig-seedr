# Development environment

One workspace folder is used from two operating systems. Read this before building or testing anything; most early confusion comes from running one system's tools on the other.

## 1. The workspace folder

`~/dev/github/elektron` on the Mac. The same folder appears as `~/mnt/elektron` inside the Linux shell (section 2).

| Folder | What | Tracked in git? |
|---|---|---|
| `trig-seedr/` | this repository | yes |
| `elekloader/` | mod loader and SDK (Python), `mods/core-ar2` (the Rytm core, 0.13) | its own repository |
| `analog-emu/` | whole-device emulator (Rust): `rytm` (window), `dtboot`, `dtpeek`, `dtcard` (headless) | its own repository |
| `Official Firmwares/` | stock `Analog-Rytm_MKII_OS1.74.syx` | no; never commit or share |
| `mi/eurorack/` | Mutable Instruments' sources (Grids' map) | its own repository |
| `Manuals/` | reference manuals | no |
| `.toolchains/` | the Linux toolchain (section 2) | no |

Files that exist only locally and must stay untracked: `trig-seedr/build/`, `mods/*/out/`, `mods/seedr-ar2/grooves-live.h`, every `.syx`, `.snap` and `.img`.

## 2. The Linux shell (the reference environment)

- **What it is:** an Ubuntu 22.04 virtual machine on ARM64 (aarch64) that the Claude desktop app attaches to the workspace folder. It is reachable only from sessions of the Claude app. A terminal on the Mac, and Claude Code started in one, cannot run commands in it.
- **Everything built and tested so far came from Linux**: the mod, the patched OS, all emulator checks, the release `.syx`.
- **Setup:** `. ~/mnt/elektron/.toolchains/env.sh` in every new shell. It provides:

| Tool | Version | Where | Notes |
|---|---|---|---|
| ColdFire cross compiler | binutils 2.44, gcc 14.2.0, C only | `.toolchains/m68k-elf/bin` | built from source; prefix `m68k-elf-`, so `ELEKLOADER_CROSS=m68k-elf-` |
| Rust | 1.97.0 | `.toolchains/rust-1.97.0` | with a small ALSA shim so the emulator's MIDI crate builds without system packages |
| Emulator binaries | | `.toolchains/target-linux/release` | `CARGO_TARGET_DIR` points here; rebuild with `cargo build --release --bin dtboot --bin dtpeek --bin dtcard` in `analog-emu/` |

- These are Linux aarch64 programs. **They do not run on macOS**, although the Mac can see the files.

## 3. The Mac (macOS on Apple silicon)

| Have | State |
|---|---|
| The emulator, built natively | `analog-emu/target/release/` holds macOS builds of `rytm`, `dtboot`, `dtpeek`, `dtcard` (built with the Mac's own Rust: `cargo build --release`) |
| elekloader | pure Python, runs as is |
| A C compiler for the host tests | Apple clang (`cc`) |
| The ColdFire cross compiler | installed 2026-11-15 from Homebrew: `m68k-elf-gcc` 16.2.0 in `/opt/homebrew/bin` (`brew install m68k-elf-binutils m68k-elf-gcc`, elekloader's documented Mac route). `make -C lib coldfire` passes with it (task 0.1). Set `export ELEKLOADER_CROSS=m68k-elf-` for elekloader |

Differences to keep in mind on the Mac:

- Homebrew's cross compiler is a newer gcc (16.x) than the reference (14.2). The mod's bytes can differ. The emulator checks decide whether behaviour is the same; if a check passes on one system and fails on the other, report it instead of working around it. **Release `.syx` files are built in Linux** until that changes.
- The scripts were written and run on Linux. Look for GNU-only commands before trusting a failure (one helper in `tools/test_seedr.sh` uses `md5sum`).
- Point the scripts at the Mac's emulator: `export B=~/dev/github/elektron/analog-emu/target/release`.

## 4. Variables the scripts use

| Variable | Meaning | Linux shell | Mac |
|---|---|---|---|
| `WORKSPACE` | the folder holding `elekloader/` and `analog-emu/` | `~/mnt/elektron` | `~/dev/github/elektron` |
| `ELEKLOADER_CROSS` | prefix of the cross tools | `m68k-elf-` (from `env.sh`) | `m68k-elf-` |
| `B` | folder of `dtboot`, `dtpeek`, `dtcard` | `$CARGO_TARGET_DIR/release` (default) | `…/analog-emu/target/release` |
| `STOCK` | the stock OS file | default under `Official Firmwares/` | same |

`WORKSPACE` defaults to the folder above the checkout. Set it explicitly in a git worktree (`.claude/worktrees/<name>/`), where that default is wrong.

## 5. Rules for sharing the folder

1. **Keep the two emulator builds apart.** `analog-emu/target/` is the Mac's; `.toolchains/target-linux/` is Linux's. In Linux, never run `cargo` without `env.sh` (it would write Linux binaries into the Mac's folder).
2. **Mod output is portable.** `.elemod` files and patched `.syx` files are ColdFire code and data: a `.syx` built in Linux runs in the Mac's emulator window and on the unit.
3. **Snapshots are not assumed portable.** Use a `.snap` only with the emulator build and the `.syx` that made it. After rebuilding the mod, delete old snapshots (the window's `gen-samples.snap` included) or run part 1 of the tests again.
4. **One emulator suite at a time** until the test harness keeps its snapshots per worktree (plan, task 0.5): today the suite writes them into `analog-emu/`.
5. **git:** commits and pushes are made on the Mac. From the Linux shell git needs permission to delete files in the folder (its lock files).

## 6. Commands

```sh
make -C lib test              # host tests with sanitizers (either system)
make -C lib coldfire          # cross build, no undefined symbols (needs the cross compiler)
sh tools/test_seedr.sh        # build the mod, patch the stock OS, run the emulator checks; PART=n for one part
sh tools/build_release.sh     # the firmware to flash (Linux)
```

The window on the Mac, for trying a build by hand:

```sh
cd ~/dev/github/elektron/analog-emu
target/release/rytm ../trig-seedr/build/seedr.syx --card gen-samples.img
```

A fresh card shows FILE SYSTEM OK dialogs: close them with NO before anything else responds.

## 7. First run in a new environment

Check, and report the result before starting a task:

```sh
uname -sm                                  # Darwin arm64, or Linux aarch64
which m68k-elf-gcc && m68k-elf-gcc --version | head -1
python3 --version && cc --version | head -1
ls "$B"/dtboot "$B"/dtpeek "$B"/dtcard     # after setting B (Mac) or sourcing env.sh (Linux)
ls "$WORKSPACE/Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx"
make -C lib test && make -C lib coldfire
PART=1 sh tools/test_seedr.sh              # every line should start with PASS
```

If the cross compiler or the emulator binaries are missing, say so and ask before installing anything.

## 8. Untested alternative: the Linux toolchain from a Mac terminal

A Linux ARM64 container with the folder mounted where `env.sh` expects it should be able to run the reference toolchain:

```sh
docker run --rm -it --platform linux/arm64 \
  -v ~/dev/github/elektron:/root/mnt/elektron -w /root/mnt/elektron/trig-seedr \
  ubuntu:22.04 bash
# inside: apt-get update && apt-get install -y python3 make gcc git; . ~/mnt/elektron/.toolchains/env.sh
```

Nobody has tried this; use `ldd` on a failing tool to see which library it wants.
