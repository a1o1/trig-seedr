#!/bin/sh
# The host fuzzer's random recipes (lib/test_fuzz.c --sample: every source and layer) replayed on the Syntakt
# (plan-syntakt.md S4; the Rytm's 41). Each recipe is written into seedr's store with the pattern's MAP
# settings, on a track of the preset A01 (64, 16 or 5 steps, with the preset's trigs), and generated with YES:
#   I2  the take is tools/seedref --device st's for the same recipe, original and context (the other twelve
#       tracks' dumps), both lock lanes included
#   I7  NO puts the track's record and lock store back byte for byte; FUNC + YES keeps the take (sample 3)
#   I8  a recipe seedref refuses writes nothing
# The sample's lane parameters are lib/'s indexes into the track's list (seedref --device st --ids). FUZZ_SAMPLES
# (default 6) and FUZZ_SAMPLE_SEED (default 41) pick the sample.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st41-fuzz-sample
st_build
# shellcheck disable=SC2046
emu_cc fuzz "$R/lib/test_fuzz.c" $(ls "$R"/lib/*.c | grep -v '/test_')
N=${FUZZ_SAMPLES:-6} SEED=${FUZZ_SAMPLE_SEED:-41}
"$W/fuzz" --sample "$N" "$SEED" > "$D/sample.txt" || emu_die "fuzz --sample"

# arg KEY ARGS...: KEY's value in a sample line (DEFAULT when it is not there)
arg() { _a_k=$1; shift; for _a in "$@"; do case $_a in "$_a_k"=*) echo "${_a#*=}"; return ;; esac; done; echo "${DEFAULT:-}"; }
# digest: the track's record and lock store
digest() { echo "$(st_mem rec $EMU_TRACK)$(st_mem locks $EMU_TRACK)" | shasum | cut -d' ' -f1; }
# others: the other twelve tracks' dumps, as seedref's other= arguments
others() {
    _o_a=
    for _o_t in 0 1 2 3 4 5 6 7 8 9 10 11 12; do
        [ $_o_t = "$EMU_TRACK" ] && continue
        st_peek dump "$EMU_SNAP" $_o_t 0 0 --seed > "$D/other$_o_t.txt"
        _o_a="$_o_a other=$_o_t:$D/other$_o_t.txt"
    done
    echo "$_o_a"
}
# lanes ARGS...: lane 1's and lane 2's lock slots on the track in scope
slots() {
    L1=$(st_slot $EMU_TRACK "$(DEFAULT=0 arg lane1.parm "$@")")
    L2=$(st_slot $EMU_TRACK "$(DEFAULT=$(seedref --keys track=$EMU_TRACK | awk '$1 == "lane2.parm" {print $2}') arg lane2.parm "$@")")
}

# prepare TRACK ARGS...: seedr closed, grid recording off; the track selected; the recipe, the pattern's MAP
# settings and a fresh YES (no seed advance) written; seedr opened; the original saved
prepare() {
    EMU_TRACK=$1; shift
    [ "$(emu_get open)" = 0 ] || emu_keys NO
    emu_rec 0
    emu_keys "TRK+T$((EMU_TRACK + 1))"
    st_seen
    EMU_POKES="$EMU_POKES $(emu_addr turned):2=65535"
    emu_recipe $EMU_TRACK "$@"
    emu_poke shared.map_x "$(DEFAULT=128 arg map_x "$@")"
    emu_poke shared.map_y "$(DEFAULT=128 arg map_y "$@")"
    emu_poke shared.map_chaos "$(DEFAULT=0 arg map_chaos "$@")"
    emu_open
    save_orig "orig$K"
    O="orig=$D/orig$K.txt $(others)"
    B0=$(digest)
    slots "$@"
}
# want ARGS...: seedref's take with lane 2's row, or "refused ..."
want() {
    # shellcheck disable=SC2086
    _w_o=$(seedref track=$EMU_TRACK all=1 "$@" $O 2>&1) || { echo "refused $_w_o"; return; }
    echo "$_w_o" | sed -n '1,12p'
    echo "$_w_o" | grep '^lane2 '
}
got() {
    st_peek dump "$EMU_SNAP" $EMU_TRACK 0 0 --seed --lane "$L1"
    st_peek dump "$EMU_SNAP" $EMU_TRACK 0 0 --seed --lane "$L2" | sed -n 's/^lane1 /lane2 /p'
}

emu_boot
K=0
while read -r line <&3; do
    K=$((K + 1))
    # shellcheck disable=SC2086
    set -- $line
    case $K in 1|4) TK=0 ;; 2|5) TK=11 ;; *) TK=3 ;; esac       # 64, 16 and 5 steps
    prepare $TK "$@"
    W0=$(want "$@")
    emu_yes
    case $W0 in
    refused*)
        emu_check "sample $K (track $((TK + 1))): refused on the host, nothing written on the device" "$B0 0" \
            "$(digest) $(emu_get auditioning)"
        echo "  sample $K: $line"
        echo "  $W0, device: $(emu_msg)"
        continue ;;
    esac
    F0=$EMU_FAIL
    emu_check "sample $K (track $((TK + 1)), $(sed -n 's/^length //p' "$D/orig$K.txt") steps): YES = seedref (I2)" "$W0" "$(got "$@")"
    [ "$EMU_FAIL" -gt "$F0" ] && echo "  sample $K: $line"
    if [ $K -ne 3 ]; then
        emu_no
        emu_check "sample $K: NO, the track byte for byte (I7)" "$B0" "$(digest)"
        continue
    fi
    emu_keep
    emu_check "sample $K: FUNC + YES applies (I7)" "APPLIED" "$(emu_msg)"
    B1=$(digest)
    emu_keys NO
    emu_check "sample $K: closed, the applied take stays" "0 $B1" "$(emu_get open) $(digest)"
done 3< "$D/sample.txt"
emu_done
