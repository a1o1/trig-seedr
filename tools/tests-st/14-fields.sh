#!/bin/sh
# Every field the Syntakt has, and a lock lane, against tools/seedref --device st (plan-syntakt.md S4): a whole
# recipe written into seedr's store (emu_recipe) with velocity, length, note, condition (MIX: A:B codes, which
# the device layer translates), random micro timing and LOCK lane 1 at 100 %; YES writes it as seedref renders
# it, locks included; NO puts the record and the lock store back byte for byte (I7).
#   1. track 4 (5 steps, emptied with stock's keys): SRC RAND, lane 1 on its default, the machine's knob A, over
#      that parameter's whole range (S3b)
#   2. track 1 (64 steps, the preset's trigs): SRC TRACK reshaped, lane 1 on ADEC 0..127
#   3. both lanes on one parameter (FRQ, lane 2's default): refused, SETTING OUT OF RANGE, nothing written (I8)
# Parameters are found by name in the track's list (seedr reads the machine first: st_seen).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st14-fields
st_build
F="vel=30:120 len=10:40 note=40:80 cond=60 ctyp=mix rand=40 lane1.pct=100"

# fields_case TRACK LABEL KEYS...: the recipe poked, seedr opened, YES = seedref; NO byte for byte
fields_case() {
    _fc_t=$1 _fc_l=$2; shift 2
    EMU_TRACK=$_fc_t
    save_orig "orig$_fc_t"
    st_seen
    REC0=$(st_mem rec $_fc_t) LKS0=$(st_mem locks $_fc_t)
    # shellcheck disable=SC2086
    emu_recipe $_fc_t "$@"
    emu_open
    emu_check "$_fc_l: the poked recipe is seedr's" "$(seedref --fields track=$_fc_t "$@" | cut -d' ' -f2 | tr '\n' ' ')" \
        "$(seedref --fields track=$_fc_t "$@" | while read -r p v; do printf '%s ' "$(emu_get "recipe[$_fc_t].$p")"; done)"
    ST_SLOT=$(st_slot $_fc_t "$(emu_get "recipe[$_fc_t].lane[0].parm")")
    emu_yes
    emu_shot "take$_fc_t"
    check_take "$_fc_l: YES = seedref, lane 1's locks included" "$@" orig="$D/orig$_fc_t.txt"
    emu_check "$_fc_l: the take has locks" 1 "$([ "$(emu_dump $_fc_t | grep '^lane1' | tr -d ' -' | wc -c)" -gt 6 ] && echo 1)"
    emu_check "$_fc_l: the take has A:B conditions" 1 "$(emu_dump $_fc_t | grep '^cond' | awk '{for (i = 2; i <= NF; i++) if ($i != "-" && $i >= 30) f = 1} END {print f + 0}')"
    emu_no
    emu_check "$_fc_l: NO, the record byte for byte" "$REC0" "$(st_mem rec $_fc_t)"
    emu_check "$_fc_l: NO, the lock store byte for byte" "$LKS0" "$(st_mem locks $_fc_t)"
    emu_no
    ST_SLOT=
}

# 1. track 4, empty: RAND
emu_fixture empty fix_empty
# shellcheck disable=SC2086
fields_case 3 "track 4, RAND" src=random seed=7 dens=80 $F

# 2. track 1, the preset's trigs: SRC TRACK with every field reshaped, lane 1 on ADEC
emu_rec 0
emu_keys TRK+T1
# shellcheck disable=SC2086
fields_case 0 "track 1, TRACK" src=track seed=3 $F lane1.parm="$(st_parm 0 ADEC)" lane1=0:127

# 3. lane 1 on FRQ, lane 2's default: two lanes on one parameter are refused before anything is written
FRQ=$(st_parm 0 FRQ)
emu_check "FRQ is lane 2's default" "$FRQ" "$(seedref --keys track=0 | awk '$1 == "lane2.parm" { print $2 }')"
emu_check "seedref refuses two lanes on FRQ" 3 "$(seedref track=0 src=random lane1.parm=$FRQ lane1.pct=100 > /dev/null 2>&1; echo $?)"
REC0=$(st_mem rec 0) LKS0=$(st_mem locks 0)
emu_recipe 0 src=random lane1.parm=$FRQ lane1.pct=100
emu_open
emu_yes
emu_check "two lanes on FRQ: SETTING OUT OF RANGE, nothing pending" "SETTING OUT OF RANGE 0" "$(emu_msg) $(emu_get auditioning)"
emu_check "two lanes on FRQ: the record as it was" "$REC0" "$(st_mem rec 0)"
emu_check "two lanes on FRQ: the lock store as it was" "$LKS0" "$(st_mem locks 0)"
emu_done
