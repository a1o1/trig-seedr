#!/bin/sh
# tools/st_run_tests.sh: the Syntakt's emulator test files (tools/tests-st/NN-*.sh, in order), then a summary, as
# tools/run_tests.sh does for the Rytm (docs/plan-syntakt.md S4).
#     sh tools/st_run_tests.sh              all of them
#     sh tools/st_run_tests.sh 03 play      those whose name contains 03 or play
# Needs ../analog-emu-st (status/S0.3.md; ST_B overrides). A file's output is also in build/emu/st<name>.out.
# Exit status: 0 when every file passed.
R=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$R/build/emu"
pass= fail=
for f in "$R"/tools/tests-st/[0-9][0-9]-*.sh; do
    n=$(basename "$f" .sh)
    if [ $# -gt 0 ]; then
        hit=
        for a in "$@"; do case $n in *"$a"*) hit=1 ;; esac; done
        [ -n "$hit" ] || continue
    fi
    start=$(date +%s)
    if sh "$f" 2>&1 | tee "$R/build/emu/st$n.out"; [ "$(tail -1 "$R/build/emu/st$n.out" | grep -c ' 0 failed$')" = 1 ]; then
        pass="$pass $n"
    else
        fail="$fail $n"
    fi
    echo "  ($n: $(($(date +%s) - start)) s)"
done
echo "passed:${pass:- none}"
echo "failed:${fail:- none}"
[ -z "$fail" ]
