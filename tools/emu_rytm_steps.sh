#!/bin/sh
# Rytm MKII OS 1.74 in analog-emu (rytm-emu): place trigs and set step fields through the UI,
# then read the A01 BD track record, to check the layout in rytm-step-layout-os174.md.
#
# Run from an analog-emu checkout after `cargo build --release` (Rust 1.94 or newer), with your
# stock file at fw/Analog-Rytm_MKII_OS1.74.syx. Nothing here is written to a real unit.
set -e
FW=fw/Analog-Rytm_MKII_OS1.74.syx
B=${B:-./target/release}         # Linux shell: B=$CARGO_TARGET_DIR/release
export DTEMU_FW=$FW
R=0x407dd8e3                      # live pattern object 1 = A01, track 0 (BD) record
peek() { $B/dtpeek "$1" mem "$(printf '%x' $((R + $2)))" "$3"; }

[ -f ${CARD:-rytm-test.img} ] || $B/dtcard ${CARD:-rytm-test.img} format
# 1. Boot (about 16 s), close the start-up dialogs, main screen.
$B/dtboot $FW --instr 4G --card ${CARD:-rytm-test.img} --card-save --save boot.snap > /dev/null
$B/dtboot $FW --load boot.snap --instr 2G --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G \
    --save menu.snap > /dev/null
# 2. GRID REC on BD, trigs on steps 1, 5, 9, 13; open the TRIG page.
$B/dtboot $FW --load menu.snap --instr 1.6G --press RECORD@6.1G --press T1@6.3G --press T5@6.5G \
    --press T9@6.7G --press T13@6.9G --save trigs.snap > /dev/null
$B/dtboot $FW --load trigs.snap --instr 0.5G --press TRIG@7.75G --save trigpage.snap > /dev/null
# 3. Hold step 5: NOTE +2, VEL -10, LEN +3, PRB -3 (encoders are 0-based; --nudge is one value step).
$B/dtboot $FW --load trigpage.snap --instr 1.4G --press T5@8.2G:1.0G --nudge 0:1@8.35Gx2/50M \
    --nudge 1:-1@8.5Gx10/20M --nudge 2:1@8.75Gx3/40M --nudge 3:-1@8.9Gx3/60M --save cond.snap > /dev/null
# 4. Hold step 9: RIGHT x3 (micro timing). Hold step 13: UP x2 (retrig).
$B/dtboot $FW --load cond.snap --instr 1.6G --press T9@9.7G:0.6G --press RIGHT@9.85G --press RIGHT@10.0G \
    --press RIGHT@10.15G --press T13@10.4G:0.6G --press UP@10.55G --press UP@10.75G --save ut.snap > /dev/null

echo "flags  (+0x000)"; peek ut.snap 0x000 32
for o in 0x080 0x0c0 0x100 0x140 0x180 0x1c0 0x200 0x240; do echo "+$o"; peek ut.snap $o 16; done
echo "track settings (+0x2c0)"; peek ut.snap 0x2c0 16
