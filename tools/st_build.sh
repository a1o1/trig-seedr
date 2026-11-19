#!/bin/sh
# tools/st_build.sh: a Syntakt OS 1.42 build for analog-emu (docs/plan-syntakt.md, task S0.3).
#     sh tools/st_build.sh NAME [--core st|mini] [--unit] [MODDIR...]
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
#   --unit       also build/emu-st/NAME-unit.syx (task U3): the stock file rebuilt by elektron-firmware-tool
#                (tools/eft.sh) with the patched MAIN (section 3) only, then checked by elekloader's syx.verify with
#                the profile's own limit (the official container's size) and an in-place margin of at least 4096
#                bytes (Synfact's rule). The tool's packing level is the tightest, of 3, 2, 1, 0, that passes both:
#                a tighter stream lets the bootloader's in-place writer overtake unread input (status/U3.md). Its
#                facts in NAME-unit.json. Unit-shaped; built in ST_AREA (below: U1's area by
#                default, status/U1.md). Not flashed or checked on a unit. Never send it anywhere
#
# The mods' area (the core's boot copier puts the mods' code and data there): ST_AREA, hex LO-HI, default
# 47a00000-47b00000, 1 MiB inside the range status/U1.md shows the OS never uses (above the DSP boot buffer, below
# the DMA ring). Synfact's profile has 46000000-46010000, which stock's sample blob reaches past about 84 s of
# samples; ST_AREA=46000000-46010000 builds there as before.
#
# Variables: WORKSPACE (as tools/emu.sh), STOCK_ST (the stock 1.42 file), ELEKLOADER_CROSS (default m68k-elf-),
# ST_AREA.
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
# device/st_params.h is made from the stock file, never committed (tools/st_params.sh)
[ -n "$(sh "$R/tools/st_params.sh")" ] || { echo "st_build.sh: no device/st_params.h (sh tools/st_params.sh)" >&2; exit 1; }
case $R in
*/.claude/worktrees/*) E=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) E=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
STOCK=${STOCK_ST:-"$E/Official Firmwares/Syntakt OS 1.42/Syntakt OS/Syntakt_OS1.42.syx"}
SRC=$E/external/synfact-dev
W=$R/build/emu-st
export ELEKLOADER_CROSS=${ELEKLOADER_CROSS:-m68k-elf-} PYTHONDONTWRITEBYTECODE=1
ST_AREA=${ST_AREA:-47a00000-47b00000}

[ $# -ge 1 ] || { echo "usage: sh tools/st_build.sh NAME [--core st|mini] [MODDIR...]" >&2; exit 2; }
NAME=$1; shift
CORE=mini UNIT=
while :; do
    case $1 in
    --core) CORE=$2; shift 2 ;;
    --unit) UNIT=1; shift ;;
    *) break ;;
    esac
done
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
(cd "$SF" && python3 - "$ST_AREA" "$STOCK" "$OUT" "$W/$NAME.lock.json" "$@" <<'PY'
import sys
from elekloader.devices import syntakt
from elekloader import patch
# emulator only: the official container's size protects a transfer to a unit, not analog-emu
object.__setattr__(syntakt.DEVICE, 'flash_limit', 0x400000)
# the mods' area (ST_AREA)
lo, hi = (int(x, 16) for x in sys.argv[1].split('-'))
object.__setattr__(syntakt.DEVICE, 'areas', {'ddr': (lo, hi)})
object.__setattr__(syntakt.DEVICE, 'ddr', (lo, hi))
stock, out, lock, mods = sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:]
sys.argv = ['patch', '--stock', stock, '--out', out, '--lockfile', lock] + sum((['--mod', m] for m in mods), [])
patch.main()
PY
) > "$W/$NAME.patch.log" 2>&1 || { cat "$W/$NAME.patch.log" >&2; exit 1; }
grep -E '^linked|^verified' "$W/$NAME.patch.log"
[ -f "$OUT" ] || { cat "$W/$NAME.patch.log" >&2; exit 1; }
echo "$OUT"

# ---- --unit (task U3): repack with elektron-firmware-tool, verify with the profile's own limits ----
if [ -n "$UNIT" ]; then
    EFT=$(sh "$R/tools/eft.sh")
    UOUT=$W/$NAME-unit.syx
    rm -f "$UOUT" "$W/$NAME-unit.json" "$W/$NAME-main.bin"
    (cd "$SF" && python3 -c 'import sys; from elekloader import syx
open(sys.argv[2], "wb").write(syx.Syx.load(sys.argv[1]).section(3))' "$OUT" "$W/$NAME-main.bin")
    ok=
    for LEVEL in 3 2 1 0; do
        rm -f "$UOUT"
        "$EFT" -i "$STOCK" -c 3 "$W/$NAME-main.bin" -l $LEVEL -o "$UOUT" > "$W/$NAME-unit.log" 2>&1 \
            || { cat "$W/$NAME-unit.log" >&2; exit 1; }
        if (cd "$SF" && python3 - "$STOCK" "$UOUT" "$W/$NAME-main.bin" "$W/$NAME-unit.json" $LEVEL <<'PY'
import json, sys
from elekloader import syx
from elekloader.devices import syntakt
stock, raw, main = syx.Syx.load(sys.argv[1]), open(sys.argv[2], 'rb').read(), open(sys.argv[3], 'rb').read()
try:
    facts = syx.verify(raw, stock, main, syntakt.DEVICE)   # the profile as shipped: flash_limit = the official container
except syx.SyxError as e:
    print('level %s: %s' % (sys.argv[5], e)); raise SystemExit(1)
if facts['main']['inplace_min_gap'] < 4096:
    print('level %s: in-place margin %d < 4096' % (sys.argv[5], facts['main']['inplace_min_gap'])); raise SystemExit(1)
built = syx.Syx(raw)
facts.update(level=int(sys.argv[5]), changed_sections=[s for s in stock.stored if built.stored[s] != stock.stored[s]],
             stock_container=stock.total, flash_limit='0x%06x' % syntakt.DEVICE.flash_limit)
json.dump(facts, open(sys.argv[4], 'w'), indent=1)
print('verified %s (level %s): container %d of %d bytes, in-place margin %d, sections changed %s'
      % (sys.argv[2].rsplit('/', 1)[-1], sys.argv[5], facts['container_len'], stock.total,
         facts['main']['inplace_min_gap'], facts['changed_sections']))
PY
        ); then ok=1; break; fi
    done
    [ -n "$ok" ] || { echo "no packing level verifies: $UOUT" >&2; rm -f "$UOUT"; exit 1; }
    echo "$UOUT"
fi
