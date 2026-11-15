#!/bin/sh
# tools/doctor.sh: checks this environment for building and testing (docs/dev-environment.md
# section 6), on the Mac. Runs no emulator suite.
#     sh tools/doctor.sh          the tools, the folders, the stock OS
#     sh tools/doctor.sh --host   also make -C lib test and make -C lib coldfire
# Each line starts OK, WARN or MISSING; the exit status is 1 when something is MISSING.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu.sh"
missing=0
ok() { echo "OK      $*"; }
warn() { echo "WARN    $*"; }
miss() { echo "MISSING $*"; missing=1; }

os=$(uname -sm)
case $os in
"Darwin arm64") ok "system: $os (the Mac)" ;;
"Linux aarch64") warn "system: $os: the Claude desktop app's Linux VM; development, tests and releases are on the Mac" \
    "(docs/dev-environment.md sections 7, 8)" ;;
*) warn "system: $os (the scripts are run on Darwin arm64, the Mac)" ;;
esac
ok "workspace: $E${WORKSPACE:+ (WORKSPACE)}"
for d in elekloader analog-emu; do [ -d "$E/$d" ] && ok "$d: $E/$d" || miss "$d: not in $E"; done

if command -v "${ELEKLOADER_CROSS}gcc" > /dev/null 2>&1; then
    v=$(${ELEKLOADER_CROSS}gcc -dumpversion)
    ok "cross compiler: $(command -v "${ELEKLOADER_CROSS}gcc") $v"
    [ "${v%%.*}" -ge 15 ] && warn "gcc $v defaults to C23: core-ar2 0.13 needs -std=gnu17 (tools/emu.sh adds it, so" \
        "tools/run_tests.sh and tools/build_release.sh build; a plain elekloader build of core-ar2 fails)"
else
    miss "cross compiler: ${ELEKLOADER_CROSS}gcc (docs/dev-environment.md sections 2, 3)"
fi
command -v python3 > /dev/null && ok "python3: $(python3 --version 2>&1)" || miss "python3"
command -v cc > /dev/null && ok "cc: $(cc --version 2>&1 | head -1)" || miss "cc"
if PYTHONPATH=$E/elekloader python3 -c 'import elekloader.sdk.build, elekloader.patch' 2> /dev/null; then
    ok "elekloader imports (sdk.build, patch)"
else
    miss "elekloader does not import from $E/elekloader"
fi

for t in dtboot dtpeek dtcard; do
    if [ ! -x "$B/$t" ]; then miss "$t: not in $B"
    elif "$B/$t" --help > /dev/null 2>&1 || "$B/$t" 2>&1 | grep -q usage; then ok "$t: $B/$t"
    else miss "$t: $B/$t does not run here (a binary for the other system?)"
    fi
done
[ -f "$STOCK" ] && ok "stock OS: $STOCK" || miss "stock OS: $STOCK"
[ -f "$R/mods/seedr-ar2/grooves-live.h" ] && ok "grooves-live.h: present (local only, untracked)" \
    || warn "grooves-live.h: absent (builds use grooves-swing.h; 02-groove skips FUNK MODERN)"

# leftovers: locks whose owner has gone, the old script's state in the shared folders
for l in "$W"/*.lock "$W"/fix/*.lock; do
    [ -d "$l" ] || continue
    p=$(cat "$l/pid" 2>/dev/null)
    if [ -n "$p" ] && kill -0 "$p" 2>/dev/null; then ok "lock held by process $p: $l"; else warn "stale lock (taken over on the next run): $l"; fi
done
n=$(find "$E/analog-emu" -maxdepth 1 \( -name '*.snap' -o -name '*.img' \) 2>/dev/null | wc -l | tr -d ' ')
[ "$n" -gt 0 ] && warn "analog-emu/ holds $n snapshots and card images from release 0.3's test script or by hand (tools/run_tests.sh writes none there)"

if [ "$1" = --host ]; then
    if make -C "$R/lib" test > "$W/doctor-test.log" 2>&1; then ok "make -C lib test: $(grep -c 'failed' "$W/doctor-test.log") test programs, none failed"
    else miss "make -C lib test failed ($W/doctor-test.log)"; fi
    if make -C "$R/lib" coldfire > "$W/doctor-cf.log" 2>&1; then ok "make -C lib coldfire: $(tail -1 "$W/doctor-cf.log")"
    else miss "make -C lib coldfire failed ($W/doctor-cf.log)"; fi
fi
exit $missing
