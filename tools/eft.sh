#!/bin/sh
# tools/eft.sh: elektron-firmware-tool (Marcel Bierling, MIT; ../external/elektron-firmware-tool), compiled into this
# checkout's build/eft/ when missing or when the clone's commit changed; prints the executable's path. The clone is
# never written to. It is a plain C11 program: standard library only, no network, no USB or MIDI (status/U3.md).
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) E=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) E=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
SRC=$E/external/elektron-firmware-tool
D=$R/build/eft
[ -f "$SRC/main.c" ] || { echo "elektron-firmware-tool is missing: $SRC" >&2; exit 1; }
COMMIT=$(git -C "$SRC" rev-parse --short HEAD)
if [ ! -x "$D/elektron-firmware-tool" ] || [ "$COMMIT" != "$(cat "$D/commit" 2>/dev/null)" ]; then
    mkdir -p "$D"
    ${CC:-cc} -std=c11 -O2 -Wall -Wextra -o "$D/elektron-firmware-tool" "$SRC"/*.c >&2
    echo "$COMMIT" > "$D/commit"
fi
echo "$D/elektron-firmware-tool"
