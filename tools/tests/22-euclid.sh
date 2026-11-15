#!/bin/sh
# F2: EUCLID gives the patterns the Rytm's own Euclidean mode plays for the same settings
# (STEPS = LEN, PULS = PL1; stock's PL2 0, RO1, RO2 and TRO at 0, OP as it starts).
#
# 1. Stock: on A01 BD's EUCLIDEAN page (TRIG, TRIG) EUC is turned on and LEN and PL1 set; stock's
#    pattern is read from the track's record and compared with tools/seedref (the host build of the
#    chain) for SOURCE = EUCLID with STEPS = LEN, PULS = PL1. LEN 16 with PL1 0..16, then other
#    lengths from 2 (stock's shortest LEN) to 64.
# 2. The mod: EUC off again, seedr's EUCLID rendered on the same track (YES) for two of those
#    settings; its trigs are compared with what stock played there, and the whole track with seedref.
#
# Stock's Euclidean settings are in the track's record (firmware-notes-os174.md section 2): +0x2C5
# the track length (LEN), +0x2CA EUC, +0x2CB PL1, +0x2CC PL2; its pattern is a table of 64 bytes at
# +0x2D1, per step the pulse's number or 0xFF for none, after PL2 and OP are combined. The trigs'
# flag words are not changed. Entries past LEN keep old values, so only the first LEN are read.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 22-euclid
emu_build

BD=0x407dd8e3                                           # A01 BD's record (tools/emu.py REC0)
LEN=$((BD + 0x2c5)) EUC=$((BD + 0x2ca)) PL1=$((BD + 0x2cb)) PL2=$((BD + 0x2cc)) TAB=$((BD + 0x2d1))
# stock's EUCLIDEAN page knobs: A PL1, B PL2, C LEN, D EUC, E RO1, F RO2, G TRO, H OP
K_PL1=0 K_LEN=2 K_EUC=3

byte() { emu_py peek "$EMU_SNAP" "$1" 1; }

# stock_pattern: the first LEN steps of stock's Euclidean table as seedref's trig line
stock_pattern() {
    # shellcheck disable=SC2046
    set -- $(emu_py peek "$EMU_SNAP" "$TAB" 1 "$(byte "$LEN")")
    _sp=trig
    for _sp_b in "$@"; do if [ "$_sp_b" = 255 ]; then _sp="$_sp ."; else _sp="$_sp x"; fi; done
    echo "$_sp"
}

# euclid_trigs STEPS PULS: seedref's trig line for SOURCE = EUCLID on a track of STEPS steps
euclid_trigs() { seedref src=euclid eu_steps="$1" eu_puls="$2" length="$1" | grep '^trig'; }

# check_stock LEN PL1: stock set to it, its pattern against seedref's
check_stock() {
    emu_sknob $K_LEN "$LEN" "$1"
    emu_sknob $K_PL1 "$PL1" "$2"
    emu_check "stock LEN $1, PL1 $2 = EUCLID STEPS $1, PULS $2" "$(euclid_trigs "$1" "$2")" "$(stock_pattern)"
}

# ---- 1. stock's Euclidean mode ----
emu_boot
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's EUCLIDEAN page" 1 "$(emu_py trigpage "$MAP" "$EMU_SNAP")"
emu_sknob $K_EUC "$EUC" 1
emu_check "EUC on; PL2 0, the track's flag words without trigs" "0 0" "$(byte "$PL2") $(emu_dump | grep -c '^trig.*x')"
emu_shot stock-euclidean
for p in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
    check_stock 16 "$p"
done
for s in "13 5" "12 7" "7 3" "5 2" "3 1" "2 1" "9 4" "24 7" "32 13" "48 17" "64 23" "64 64" "13 5"; do
    # shellcheck disable=SC2086
    check_stock $s
done
emu_shot stock-13-5
STOCK13=$(stock_pattern)

# ---- 2. seedr's EUCLID on the same track, stock's Euclidean mode off ----
emu_sknob $K_EUC "$EUC" 0
emu_check "EUC off: LEN stays 13, no trigs on the track" "13 0" "$(byte "$LEN") $(emu_dump | grep -c '^trig.*x')"
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 2
emu_knob RHYTHM B:eu_steps 13
emu_knob RHYTHM C:eu_puls 5
emu_yes
emu_shot euclid
emu_check "seedr EUCLID 13 / 5 plays what stock's LEN 13, PL1 5 played" "$STOCK13" "$(emu_dump | grep '^trig')"
check_take "seedr EUCLID 13 / 5: the whole track as seedref" src=euclid eu_steps=13 eu_puls=5 length=13
emu_keep
emu_no
emu_check "kept and closed: EUC still off" "0 0" "$(emu_get open) $(byte "$EUC")"

# the same at 64 steps: stock's pattern, then seedr's (BD's recipe is still EUCLID)
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's EUCLIDEAN page again" 1 "$(emu_py trigpage "$MAP" "$EMU_SNAP")"
emu_sknob $K_EUC "$EUC" 1
check_stock 64 23
STOCK64=$(stock_pattern)
emu_sknob $K_EUC "$EUC" 0
emu_open
emu_page RHYTHM
emu_knob RHYTHM B:eu_steps 64
emu_knob RHYTHM C:eu_puls 23
emu_yes
emu_check "seedr EUCLID 64 / 23 plays what stock's LEN 64, PL1 23 played" "$STOCK64" "$(emu_dump | grep '^trig')"
emu_done
