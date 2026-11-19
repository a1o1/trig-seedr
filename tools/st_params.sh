#!/bin/sh
# tools/st_params.sh: device/st_params.h, made from your own stock Syntakt OS 1.42 file (tools/st_params.py).
# The header is never committed (Alan, 2026-11-19: generated from the stock file's parameter table, so it
# stays local, as every build starts from your own stock file). Prints the header's path when it is there
# (made now, or already up to date); prints nothing when the stock file or synfact-dev's elekloader is
# missing, so the Makefiles and scripts skip their Syntakt parts. Exit status 0 either way.
#
# Variables: WORKSPACE (as tools/emu.sh), STOCK_ST (the stock 1.42 file), SYNFACT (synfact-dev, whose
# elekloader reads the container).
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) E=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) E=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
STOCK=${STOCK_ST:-"$E/Official Firmwares/Syntakt OS 1.42/Syntakt OS/Syntakt_OS1.42.syx"}
EL=${SYNFACT:-$E/external/synfact-dev}
H=$R/device/st_params.h

if [ -f "$H" ] && [ "$H" -nt "$R/tools/st_params.py" ]; then
    echo "$H"
    exit 0
fi
if [ ! -f "$STOCK" ] || [ ! -d "$EL" ]; then
    echo "st_params.sh: no stock Syntakt OS 1.42 file or synfact-dev: the Syntakt's parts are skipped" >&2
    exit 0
fi
PYTHONDONTWRITEBYTECODE=1 python3 -I "$R/tools/st_params.py" "$STOCK" "$EL" > "$H.tmp" && mv "$H.tmp" "$H" || {
    rm -f "$H.tmp"
    echo "st_params.sh: tools/st_params.py failed" >&2
    exit 0
}
echo "$H"
