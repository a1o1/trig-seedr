#!/bin/sh
# tools/fuzz.sh: the long fuzz run of the chain (lib/test_fuzz.c, plan task M2.2) with ASan and UBSan.
#     sh tools/fuzz.sh [SECONDS [JOBS]]     default 3600 s, one job per CPU core (at most 8)
#     FUZZ_SEED=0x... sh tools/fuzz.sh 60 1 the same cases again (a job's seed is printed)
# Builds test_fuzz with lib/Makefile's host flags into build/fuzz/, runs JOBS copies side by side,
# each from its own seed, and reports cases, failures and each job's seed. A failure prints its case
# number: `FUZZ_CASE=0x... build/fuzz/test_fuzz` replays and prints it. Exit status 0 when every job
# found nothing.
R=$(cd "$(dirname "$0")/.." && pwd)
SECS=${1:-3600}
JOBS=${2:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 1)}
[ "$JOBS" -gt 8 ] && JOBS=8
O=$R/build/fuzz
mkdir -p "$O"
FLAGS=$(sed -n 's/^CFLAGS_HOST = //p' "$R/lib/Makefile")
# shellcheck disable=SC2086
cc $FLAGS -fno-sanitize-recover=undefined "$R/lib/test_fuzz.c" $(ls "$R"/lib/*.c | grep -v '/test_') -o "$O/test_fuzz" ||
    { echo "fuzz: build failed"; exit 1; }
base=${FUZZ_SEED:-$(($(date +%s) * 2654435761 % 4294967296))}
echo "fuzz: $JOBS jobs for $SECS s, from $(date '+%Y-%m-%d %H:%M:%S')"
j=0
while [ $j -lt "$JOBS" ]; do
    seed=$(printf '0x%08x' $(((base + j * 40503 * 65536) % 4294967296)))
    FUZZ_SECONDS=$SECS FUZZ_SEED=$seed "$O/test_fuzz" > "$O/job$j.out" 2>&1 &
    j=$((j + 1))
done
wait
cases=0 failed=0 j=0
while [ $j -lt "$JOBS" ]; do
    f=$O/job$j.out
    n=$(sed -n 's/^fuzz: \([0-9]*\) cases.*/\1/p' "$f")
    seed=$(sed -n 's/.*FUZZ_SEED=\(0x[0-9a-f]*\).*/\1/p' "$f" | head -1)
    if tail -1 "$f" | grep -q ' 0 failed$'; then r=ok; else r=FAILED; failed=$((failed + 1)); fi
    echo "  job $j: seed $seed, ${n:-0} cases, $r"
    [ "$r" = ok ] || { grep -E '^FAIL|ERROR|runtime error|Sanitizer' "$f" | head -20 | sed 's/^/    /'; }
    cases=$((cases + ${n:-0}))
    j=$((j + 1))
done
echo "fuzz: $cases cases in $SECS s, $failed of $JOBS jobs failed (outputs: build/fuzz/job*.out)"
sed -n '/^  sources/,/^  failures/p' "$O/job0.out"
[ $failed -eq 0 ]
