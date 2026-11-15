#!/bin/sh
# tools/run_tests.sh: every test file in tools/tests (NN-<name>.sh, in order), then a summary.
#     sh tools/run_tests.sh              all of them
#     sh tools/run_tests.sh 03 rotate    those whose name contains 03 or rotate
# Each file writes only under this checkout's build/emu/<name>/ (tools/emu.sh), so this runs
# side by side with the same in another worktree. A file's output is also in build/emu/<name>.out.
# Exit status: 0 when every file passed.
R=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$R/build/emu"
pass= fail=
for f in "$R"/tools/tests/[0-9][0-9]-*.sh; do
    n=$(basename "$f" .sh)
    if [ $# -gt 0 ]; then
        hit=
        for a in "$@"; do case $n in *"$a"*) hit=1 ;; esac; done
        [ -n "$hit" ] || continue
    fi
    start=$(date +%s)
    if sh "$f" 2>&1 | tee "$R/build/emu/$n.out"; [ "$(tail -1 "$R/build/emu/$n.out" | grep -c ' 0 failed$')" = 1 ]; then
        pass="$pass $n"
    else
        fail="$fail $n"
    fi
    echo "  ($n: $(($(date +%s) - start)) s)"
done
echo "passed:${pass:- none}"
echo "failed:${fail:- none}"
[ -z "$fail" ]
