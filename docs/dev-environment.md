# Development environment

Development is on the Mac. Since version 0.4 the Mac is the reference: everything is built, tested and released there
(decided 2026-11-15). Read this before building or testing anything.

## 1. The workspace folder

`~/dev/github/elektron` on the Mac.

| Folder | What | Tracked in git? |
|---|---|---|
| `trig-seedr/` | this repository | yes |
| `elekloader/` | mod loader and SDK (Python), `mods/core-ar2` (the Rytm core, 0.13) | its own repository |
| `analog-emu/` | whole-device emulator (Rust): `rytm` (window), `dtboot`, `dtpeek`, `dtcard` (headless) | its own repository |
| `Official Firmwares/` | stock `Analog-Rytm_MKII_OS1.74.syx` | no; never commit or share |
| `mi/eurorack/` | Mutable Instruments' sources (Grids' map) | its own repository |
| `Manuals/` | reference manuals | no |
| `.toolchains/` | the Linux toolchain 0.3 was built with (section 8) | no |

Files that exist only locally and must stay untracked: `trig-seedr/build/`, `mods/*/out/`, `mods/seedr-ar2/grooves-live.h`, every `.syx`, `.snap` and `.img`.

## 2. The Mac (macOS on Apple silicon): the reference

| Tool | Version | Where | Notes |
|---|---|---|---|
| ColdFire cross compiler | m68k-elf-gcc 16.2.0 and binutils, from Homebrew | `/opt/homebrew/bin` | `brew install m68k-elf-binutils m68k-elf-gcc` (elekloader's documented Mac route); prefix `m68k-elf-` |
| The emulator | analog-emu, built natively | `analog-emu/target/release/` | `rytm`, `dtboot`, `dtpeek`, `dtcard`; rebuild with `cargo build --release` in `analog-emu/` |
| elekloader | | `elekloader/` | pure Python, runs as is |
| Host C compiler | Apple clang | `cc` | the host tests, with ASan / UBSan |

- **gcc 16 defaults to C23.** core-ar2 0.13 does not build as C23 (`mods/core-ar2/scenes.c` calls methods through `int (*)()`); `tools/emu.sh` builds it with `-std=gnu17`, so `tools/run_tests.sh` and `tools/build_release.sh` work. A fix for elekloader is on the local branch `core-ar2-c23-method-prototype` in `elekloader/`, to be sent upstream.
- **Releases** are built on the Mac with `tools/build_release.sh`.

## 3. Variables the scripts use

| Variable | Meaning | Value |
|---|---|---|
| `WORKSPACE` | the folder holding `elekloader/` and `analog-emu/` | `~/dev/github/elektron` |
| `ELEKLOADER_CROSS` | prefix of the cross tools | `m68k-elf-` |
| `B` | folder of `dtboot`, `dtpeek`, `dtcard` | `~/dev/github/elektron/analog-emu/target/release` (the default on the Mac) |
| `STOCK` | the stock OS file | default under `Official Firmwares/` |

`WORKSPACE` defaults to the folder above the checkout, and the test scripts also find it from a git worktree's path
(`.claude/worktrees/<name>/`). Set it explicitly for a checkout elsewhere. Use two `export` lines: the shell expands
`$WORKSPACE` in a line before that line sets it, so on one line `B` would come out empty.

```sh
export WORKSPACE=~/dev/github/elektron
export B=$WORKSPACE/analog-emu/target/release ELEKLOADER_CROSS=m68k-elf-
```

## 4. Rules

1. **Snapshots are not portable between builds.** Use a `.snap` only with the emulator build and the `.syx` that made it. After rebuilding the mod, delete old snapshots (the window's `play.snap` included).
2. **Emulator suites run side by side.** Each checkout's `tools/run_tests.sh` writes only under its own `build/emu/` (task 0.5), so suites in two worktrees do not interfere.
3. **Leave `.toolchains/target-linux/` alone.** It holds the Linux emulator build (section 8); `analog-emu/target/` is the Mac's.

## 5. Commands

```sh
sh tools/doctor.sh            # the environment
make -C lib test              # host tests with sanitizers
make -C lib coldfire          # cross build, no undefined symbols
sh tools/run_tests.sh         # build the mod, patch the stock OS, run every check (host and emulator); NAME... for some
sh tools/build_release.sh     # the firmware to flash: build/trig-seedr-<version>-os1.74.syx
```

The window, for trying a build by hand:

```sh
cd ~/dev/github/elektron/analog-emu
target/release/dtcard play.img format
target/release/rytm ../trig-seedr/build/emu/seedr.syx --card play.img
```

A fresh card shows FILE SYSTEM OK dialogs: close them with NO before anything else responds. In the window a click is
one press: **Shift + click** latches a key down (TRIG held + PAGE: Shift + click TRIG, click PAGE, Shift + click TRIG).

## 6. First run in a new session

Check, and report the result before starting a task:

```sh
uname -sm                                  # Darwin arm64
which m68k-elf-gcc && m68k-elf-gcc --version | head -1
python3 --version && cc --version | head -1
ls "$B"/dtboot "$B"/dtpeek "$B"/dtcard
ls "$WORKSPACE/Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx"
sh tools/run_tests.sh 00 01                # every line should start with PASS
```

If the cross compiler or the emulator binaries are missing, say so and ask before installing anything.

## 7. Claude sessions

- **Claude Code in a Mac terminal** runs on macOS: everything above applies.
- **The Claude desktop app** runs its commands in a Linux virtual machine (Ubuntu 22.04, ARM64), where this folder is
  `~/mnt/elektron`. That path does not exist on the Mac, and the Mac cannot run the VM's programs. Development no
  longer uses it; a desktop-app session working on this repository should say it is in Linux before building.

## 8. History: the Linux toolchain (0.3)

Versions up to 0.3 were built and tested in the desktop app's Linux VM, with tools in `.toolchains/` that only run
there: an m68k-elf gcc 14.2.0 built from source, Rust 1.97.0, and a Linux build of the emulator
(`.toolchains/target-linux/release`), set up by `. ~/mnt/elektron/.toolchains/env.sh`. These are Linux aarch64
programs and do not run on macOS. Release 0.3's `.syx` came from there; 0.4 and later are built on the Mac.
