#!/bin/sh
# Task M2.2: a sample of the host fuzzer's random recipes (lib/test_fuzz.c --sample, props.h: every
# source and layer) replayed on A01 BD in the emulator, from the smp3 fixture (BD with trigs and
# sample locks). Each recipe is written into the recipe store with the pattern's MAP settings, at a
# length of 16, 24, 37 or 64 steps, and generated with YES:
#   I2 on the device  the take is tools/seedref's for the same recipe, original and context (the
#                     other twelve tracks' dumps), lane 2's locks included
#   I7                NO puts BD back byte for byte (its step record and lock store); FUNC + YES
#                     keeps the take (the third recipe)
#   I9                an applied take plays (PLAY, STOP), saves and reloads (SAVE / RELOAD PATTERN)
#                     and survives a power cycle
#   I8                on a build with no lock rows left, a recipe with a lock lane is refused with
#                     NO LOCK ROWS LEFT and nothing written
#   two lanes on one parameter (found by the fuzzer) are refused with nothing written
# FUZZ_SAMPLES (default 6) and FUZZ_SAMPLE_SEED (default 41) pick the sample.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 41-fuzz-sample
emu_build
# shellcheck disable=SC2046
emu_cc fuzz "$R/lib/test_fuzz.c" $(ls "$R"/lib/*.c | grep -v '/test_')
N=${FUZZ_SAMPLES:-6} SEED=${FUZZ_SAMPLE_SEED:-41}
"$W/fuzz" --sample "$N" "$SEED" > "$D/sample.txt" || emu_die "fuzz --sample"

BD=0x407dd8e3                                   # A01 BD's record (tools/emu.py REC0)
LEN=$((BD + 0x2c5))
# the lock lanes' sound indexes by parameter (device/rytm_params.h RT_PARAMS, in order)
IDS="17 18 20 19 21 22 24 26 27 28 29 25 30 31 33 34 35 36 40 41 38 37 5 6 7 8"
# bd: a digest of BD's step record and lock store (12-lock-rows.sh)
bd() { python3 -c "import sys; sys.path.insert(0, '$T'); import emu, hashlib
print(hashlib.sha1(emu.mem('$EMU_SNAP', $BD, 0x395) + emu.mem('$EMU_SNAP', $BD + 0x2e91, 0x162a)).hexdigest())"; }
# lane2 PARM: BD's locks on that parameter, as seedref's lane2 row
lane2() {
    _l2_k=$(echo $IDS | cut -d' ' -f$(($1 + 1)))
    python3 -c "import sys; sys.path.insert(0, '$T'); import emu
n = emu.mem('$EMU_SNAP', $LEN, 1)[0]
l = emu.mem('$EMU_SNAP', $BD + 0x2e91, 88 * n)
v = [int.from_bytes(l[88 * i + 2 * $_l2_k:88 * i + 2 * $_l2_k + 2], 'big', signed=True) for i in range(n)]
print('lane2 ' + ' '.join('-' if x < 0 else str(x >> 8) for x in v))"
}
# others: the other twelve tracks' dumps, as seedref's other= arguments
others() {
    _o_a=
    for _o_t in 1 2 3 4 5 6 7 8 9 10 11 12; do
        emu_dump $_o_t > "$D/other$_o_t.txt"
        _o_a="$_o_a other=$_o_t:$D/other$_o_t.txt"
    done
    echo "$_o_a"
}
# arg KEY ARGS...: KEY's value in a sample line (the default when it is not there)
arg() { _a_k=$1; shift; for _a in "$@"; do case $_a in "$_a_k"=*) echo "${_a#*=}"; return ;; esac; done; echo "${DEFAULT:-}"; }

# prepare LENGTH ARGS...: seedr closed, grid recording off; BD's length, the recipe, the pattern's MAP
# settings and a fresh YES (no seed advance) written; seedr opened; the original saved
prepare() {
    _p_len=$1; shift
    [ "$(emu_get open)" = 0 ] || emu_keys NO
    emu_rec 0
    EMU_POKES="$EMU_POKES $(printf '0x%08x' $LEN):1=$_p_len $(emu_addr turned):2=65535"
    emu_recipe 0 "$@"
    emu_poke shared.map_x "$(DEFAULT=128 arg map_x "$@")"
    emu_poke shared.map_y "$(DEFAULT=128 arg map_y "$@")"
    emu_poke shared.map_chaos "$(DEFAULT=0 arg map_chaos "$@")"
    emu_open
    save_orig "orig$K"
    O="orig=$D/orig$K.txt $(others)"
    B0=$(bd)
}
# want ARGS...: seedref's take (lane2 for lane 2's parameter), or "refused N"
want() {
    _w_p=$(DEFAULT=12 arg lane2.parm "$@")
    # shellcheck disable=SC2086
    _w_o=$(seedref all=1 "$@" $O 2>&1) || { echo "refused $_w_o"; return; }
    echo "$_w_o" | sed -n '1,12p'
    echo "$_w_o" | grep '^lane2 '
}
got() { emu_dump; lane2 "$(DEFAULT=12 arg lane2.parm "$@")"; }

emu_fixture smp3 fix_smp3
K=0
while read -r line <&3; do
    K=$((K + 1))
    # shellcheck disable=SC2086
    set -- $line
    case $K in 1|5) L=16 ;; 2|6) L=64 ;; 3|7) L=24 ;; *) L=37 ;; esac
    prepare $L "$@"
    W0=$(want "$@")
    emu_yes
    case $W0 in
    refused*)
        emu_check "sample $K ($L steps): refused on the host, nothing written on the device" "$B0 0" "$(bd) $(emu_get auditioning)"
        echo "  sample $K: $line"
        echo "  $W0, device: $(emu_msg)"
        continue ;;
    esac
    F0=$EMU_FAIL
    emu_check "sample $K ($L steps): YES = seedref (I2)" "$W0" "$(got "$@")"
    [ "$EMU_FAIL" -gt "$F0" ] && echo "  sample $K: $line"
    if [ $K -ne 3 ]; then
        emu_no
        emu_check "sample $K: NO, BD byte for byte (I7)" "$B0" "$(bd)"
        continue
    fi
    emu_keep
    emu_check "sample $K: FUNC + YES applies (I7)" "APPLIED" "$(emu_msg)"
    B1=$(bd)
    emu_keys NO                                 # close
    emu_rec 0
    emu_keys PLAY w:2000 STOP
    emu_check "sample $K: the applied take plays; BD as applied" "0 $B1" "$(emu_get open) $(bd)"
    emu_keys YES+SCNE w:300
    emu_open
    emu_page TAKE
    emu_knob TAKE A:seed $(( $(arg seed "$@") == 9999 ? 9998 : $(arg seed "$@") + 1 ))
    emu_yes
    emu_keep
    emu_keys NO
    emu_rec 0
    emu_keys NO+SCNE w:300
    emu_check "sample $K: SAVE, another take kept, RELOAD PATTERN: the applied take (I9)" "$B1" "$(bd)"
    # a power cycle now, before a later sample sets BD's length again (lengths are poked into the live
    # pattern, which a power cycle need not keep as poked)
    PD=$(emu_dump)
    emu_power_off
    emu_boot "$EMU_CARD"
    emu_check "sample $K: power cycle: BD byte for byte, the applied take in it (I9)" "$B1 $PD" "$(bd) $(emu_dump)"
done 3< "$D/sample.txt"

# two lanes on one parameter: lane 1 at 100 % on the sample slot, lane 2 left on it too
K=dup
prepare 16 src=random dens=50 lane1.pct=100 lane1=10:20 lane2.parm=3
emu_yes
emu_check "two lanes on SMP: refused, nothing written" "SETTING OUT OF RANGE $B0 0" "$(emu_msg) $(bd) $(emu_get auditioning)"
if [ "$(emu_get auditioning)" != 0 ]; then
    echo "  written: lane 1 as seedref renders it, and as the device stored it:"
    seedref src=random dens=50 lane1.pct=100 lane1=10:20 lane2.parm=3 $O | grep '^lane1' | sed 's/^/  /'
    emu_dump | grep '^lane1' | sed 's/^/  /'
fi
emu_keys NO

# I8: no lock rows left (a test build): a sample recipe with lane 1 on is refused, nothing written
EMU_VERSION=GR0T emu_build norows -DRT_ROWS_MAX=0
emu_boot
K=rows
# shellcheck disable=SC2046
prepare 16 $(head -1 "$D/sample.txt") src=random dens=60 mask=off skip=0 lane1.pct=50 lane1=1:8
emu_yes
emu_check "no lock rows: refused with NO LOCK ROWS LEFT, nothing written (I8)" "NO LOCK ROWS LEFT $B0 0" \
    "$(emu_msg) $(bd) $(emu_get auditioning)"
emu_done
