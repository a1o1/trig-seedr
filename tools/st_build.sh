#!/bin/sh
# tools/st_build.sh: a Syntakt OS 1.42 build for analog-emu (docs/plan-syntakt.md, task S0.3).
#     sh tools/st_build.sh NAME [--core st|mini] [MODDIR...]
# builds the core and each mod with Synfact's elekloader (../external/synfact-dev, copied to build/emu-st/synfact so
# nothing is written into external/), patches the stock file, and writes build/emu-st/NAME-emu.syx and its
# .syx.map.json. Prints the .syx's path last.
#
#   --core st    Synfact's core-st (mods/core-st, every service). With gcc 16 it does not fit its 64 KiB area
#                (status/S0.3.md): the patch is refused
#   --core mini  (default) core-st's hook bus only: core.s and settings.s at its boot, tick, draw, key, encoder
#                and SETTINGS sites, generated here from the copy. A measurement core, not a product
#
# EMULATOR ONLY: elekloader's packer makes a container larger than the official file's, which the Syntakt profile
# refuses (its flash_limit: Synfact repacks with elektron-firmware-tool for a unit). This script lifts that limit
# in-process for the build. Never send a *-emu.syx to a unit.
#
# Variables: WORKSPACE (as tools/emu.sh), STOCK_ST (the stock 1.42 file), ELEKLOADER_CROSS (default m68k-elf-).
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) E=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) E=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
STOCK=${STOCK_ST:-"$E/Official Firmwares/Syntakt OS 1.42/Syntakt OS/Syntakt_OS1.42.syx"}
SRC=$E/external/synfact-dev
W=$R/build/emu-st
export ELEKLOADER_CROSS=${ELEKLOADER_CROSS:-m68k-elf-} PYTHONDONTWRITEBYTECODE=1

[ $# -ge 1 ] || { echo "usage: sh tools/st_build.sh NAME [--core st|mini] [MODDIR...]" >&2; exit 2; }
NAME=$1; shift
CORE=mini
if [ "$1" = --core ]; then CORE=$2; shift 2; fi
[ -f "$STOCK" ] || { echo "the stock Syntakt OS 1.42 file is missing: $STOCK" >&2; exit 1; }
[ -d "$SRC/elekloader" ] || { echo "Synfact is missing: $SRC" >&2; exit 1; }

# Synfact, copied when its commit changes (its builders write next to the sources)
mkdir -p "$W"
SF=$W/synfact
COMMIT=$(git -C "$SRC" rev-parse --short HEAD)
if [ "$COMMIT" != "$(cat "$W/synfact.commit" 2>/dev/null)" ]; then
    rm -rf "$SF"
    rsync -a --exclude .git --exclude out "$SRC/" "$SF/"
    echo "$COMMIT" > "$W/synfact.commit"
fi

# build MODDIR OUTDIR: one .elemod
build() {
    rm -rf "$2"
    (cd "$SF" && python3 -m elekloader.sdk.build "$1" --stock "$STOCK" --out "$2") > "$2.log" 2>&1 \
        || { cat "$2.log" >&2; exit 1; }
    grep -h '^  ' "$2.log" | sed 's/;.*//'
}

# the core
case $CORE in
st)
    build "$SF/mods/core-st" "$W/core-st" ;;
mini)
    M=$W/core-mini/src
    rm -rf "$W/core-mini"; mkdir -p "$M/core" "$M/core-st"
    cp "$SF/mods/core/core.s" "$SF/mods/core/settings.s" "$M/core/"
    cp "$SF/mods/core-st/settings.s" "$M/core-st/"
    python3 - "$SF/mods/core-st/mod.json" "$M/core-st/mod.json" <<'PY'
import json, sys
m = json.load(open(sys.argv[1]))
sites = ('0x40000538', '0x4000a220', '0x4000a286', '0x4000b420', '0x4000b468', '0x4005f176')
syms = ('DRAWALL', 'KEYDISP', 'ENCDISP', 'NO_DTIM0', 'VBR_FN', 'OP_NEW', 'ITEM_CTOR', 'MENU_ADD', 'SETTINGS_RET')
d = {k: m[k] for k in ('license', 'author', 'device', 'os')}
d.update(id='core', version='0.0.1-mini', title='core (hook bus only)', category='Framework',
         description='core-st %s: boot, tick, draw, key, encoder and SETTINGS only (trig-seedr S0.3)' % m['version'],
         sources=['../core/core.s', 'settings.s'], defsym={k: v for k, v in m['defsym'].items() if k in syms},
         sites=[s for s in m['sites'] if s['addr'] in sites],
         collections={k: 4 for k in ('ev_tick', 'ev_draw', 'ev_key', 'ev_enc', 'ev_settings')})
assert len(d['sites']) == len(sites), 'core-st moved a site'
json.dump(d, open(sys.argv[2], 'w'), indent=1)
PY
    build "$M/core-st" "$W/core-mini/out" ;;
*) echo "--core st or mini" >&2; exit 2 ;;
esac
set -- "$(ls "$W"/core-"$CORE"*/core-*.elemod "$W"/core-"$CORE"/out/core-*.elemod 2>/dev/null | head -1)" "$@"

# each mod, then the patch; the list of .elemod files is built in the positional parameters
n=$#; i=1
while [ $i -le $n ]; do
    a=$1; shift
    case $a in
    *.elemod) set -- "$@" "$a" ;;
    *) d=$(cd "$a" && pwd); o=$W/mod-$(basename "$d"); build "$d" "$o"; set -- "$@" "$(ls "$o"/*.elemod)" ;;
    esac
    i=$((i + 1))
done
OUT=$W/$NAME-emu.syx
rm -f "$OUT" "$OUT.map.json"
(cd "$SF" && python3 - "$STOCK" "$OUT" "$W/$NAME.lock.json" "$@" <<'PY'
import sys
from elekloader.devices import syntakt
from elekloader import patch
# emulator only: the official container's size protects a transfer to a unit, not analog-emu
object.__setattr__(syntakt.DEVICE, 'flash_limit', 0x400000)
stock, out, lock, mods = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
sys.argv = ['patch', '--stock', stock, '--out', out, '--lockfile', lock] + sum((['--mod', m] for m in mods), [])
patch.main()
PY
) > "$W/$NAME.patch.log" 2>&1 || { cat "$W/$NAME.patch.log" >&2; exit 1; }
grep -E '^linked|^verified' "$W/$NAME.patch.log"
[ -f "$OUT" ] || { cat "$W/$NAME.patch.log" >&2; exit 1; }
echo "$OUT"
