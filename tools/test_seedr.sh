#!/bin/sh
# seedr (trig-seedr) in analog-emu: open, GEN 1, YES (seed 1), seed 2 + YES, NO (revert), YES, FUNC + YES
# (keep), NO (close). After each stage the A01 BD track is compared with genref.
# Needs ../elekloader, ../analog-emu (built), the stock OS in "../Official Firmwares/..." and the m68k-elf
# toolchain (README.md). Run from the repository: sh tools/test_seedr.sh   (PART=1..6: one part)
set -e
R=$(cd "$(dirname "$0")/.." && pwd)                 # this repository
E=${WORKSPACE:-$(cd "$R/.." && pwd)}                  # the folder holding elekloader/ and analog-emu/
T=$R/tools; W=$R/build; mkdir -p $W
STOCK=${STOCK:-"$E/Official Firmwares/Analog-Rytm_MKII_OS1.74_dist/Analog-Rytm_MKII_OS1.74.syx"}
B=${B:-$CARGO_TARGET_DIR/release}; FW=$W/seedr.syx
P=${PART:-all}   # PART=1: build + generate/audition; 2: GROOVE, regenerate; 3: save paths; 4: sample locks; 5: ROTATE (2..5 resume from 1's snapshots);
#                6: the lock-row refusal (its own test build, RT_ROWS_MAX 0)
part() { [ "$P" = all ] || [ "$P" = "$1" ]; }
cc -std=c99 -O2 -Wall $T/genref.c $R/lib/gen.c $R/lib/groove.c -o $T/genref
cd $E/elekloader
if part 1; then
python3 -m elekloader.sdk.build mods/core-ar2 --stock "$STOCK" | head -1
python3 -m elekloader.sdk.build $R/mods/seedr-ar2 --stock "$STOCK" | head -1
python3 -m elekloader.patch --stock "$STOCK" --mod mods/core-ar2/out/core-0.13.elemod \
    --mod $R/mods/seedr-ar2/out/seedr-0.3.elemod \
    --out $FW --lockfile $W/seedr.lock.json --version GR01 | tail -1
fi
cd $E/analog-emu
run() { snap_in=$1; snap_out=$2; shift 2; $B/dtboot $FW --load $snap_in "$@" --save $snap_out > /dev/null; }
check() { # name snapshot expected (genref's output; its first line unless the name says groove)
    got=$(python3 $T/emucmp.py $B/dtpeek $FW $2 | head -1)
    case "$1" in *SMP*|*smp*) # also the sample locks: genref's 3rd line, emucmp's 4th
        gl=$(echo "$3" | sed -n 3p); el=$(python3 $T/emucmp.py $B/dtpeek $FW $2 | sed -n 4p)
        want=$(echo "$3" | head -1)
        if [ "$got" = "$want" ] && [ "$gl" = "$el" ]; then echo "PASS $1   $el"; else echo "FAIL $1"; echo "  want $want / $gl"; echo "  got  $got / $el"; fi
        return ;;
    esac
    case "$1" in *GROOVE*|*groove*) # also the µtimes: genref's 2nd line, emucmp's 3rd
        gu=$(echo "$3" | sed -n 2p); eu=$(python3 $T/emucmp.py $B/dtpeek $FW $2 | sed -n 3p)
        want=$(echo "$3" | head -1)
        if [ "$got" = "$want" ] && [ "$gu" = "$eu" ]; then echo "PASS $1   $eu"; else echo "FAIL $1"; echo "  want $want / $gu"; echo "  got  $got / $eu"; fi
        return ;;
    esac
    rest=$(python3 $T/emucmp.py $B/dtpeek $FW $2 | tail -2 | tr '\n' ' ')
    exp=$(echo "$3" | head -1)
    if [ "$got" = "$exp" ]; then echo "PASS $1   $rest"; else echo "FAIL $1"; echo "  want $exp"; echo "  got  $got"; fi
}
ORIG=$(echo ".ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff ")
if part 1; then
$B/dtcard gr-test.img format > /dev/null
$B/dtboot $FW --instr 5.6G --card gr-test.img --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G --save gr0.snap > /dev/null
check "boot: empty track      " gr0.snap "$ORIG"
run gr0.snap gr1.snap --instr 1.5G --press TRIG@5.8G:0.4G --press PAGE@5.95G --press PAGE@6.4G --press YES@6.7G
DTEMU_FW=$FW $B/dtpeek gr1.snap fb | python3 $T/fb2png.py $W/seedr-gen1.png 4 > /dev/null
check "YES, seed 1            " gr1.snap "$($T/genref 1)"
run gr1.snap gr2.snap --instr 0.9G --nudge 6:1@7.3G --press YES@7.6G
DTEMU_FW=$FW $B/dtpeek gr2.snap fb | python3 $T/fb2png.py $W/seedr-gen1-seed2.png 4 > /dev/null
check "seed 2, YES (from orig) " gr2.snap "$($T/genref 2)"
run gr2.snap gr3.snap --instr 0.6G --press NO@8.2G
check "NO: back to original   " gr3.snap "$ORIG"
# SEED turned away and back: a knob turn, so this YES takes seed 2 again (fresh from the original)
run gr3.snap gr4.snap --instr 1.2G --nudge 6:-1@8.65G --nudge 6:1@8.72G --press YES@8.8G --press FUNC@9.2G:0.4G --press YES@9.35G
check "YES, FUNC+YES: kept    " gr4.snap "$($T/genref 2)"
run gr4.snap gr5.snap --instr 0.6G --press NO@10.0G
check "NO: page closed, kept  " gr5.snap "$($T/genref 2)"
f2=$(python3 $T/emucmp.py $B/dtpeek $FW gr2.snap | sed -n 2p); f4=$(python3 $T/emucmp.py $B/dtpeek $FW gr4.snap | sed -n 2p)
if [ "$f2" = "$f4" ]; then echo "PASS audition take = fresh generate (flag words identical)"; else echo "FAIL audition take differs from a fresh generate"; echo "  take  $f2"; echo "  fresh $f4"; fi
DTEMU_FW=$FW $B/dtpeek gr5.snap fb | python3 $T/fb2png.py $W/seedr-closed.png 4 > /dev/null
fi

if part 3; then
# ---- stock save paths with the generated track (gr5: seed 2 kept, page closed; clock 10.4 G)
mark() { python3 $T/emucmp.py $B/dtpeek $FW $1 | tail -1; }
# 1. SAVE PATTERN (YES + SCENE), generate seed 3 over it, RELOAD PATTERN (NO + SCENE): back to seed 2
run gr5.snap ts0.snap --instr 1.0G --press YES@10.5G:0.4G --press SCNE@10.65G
run ts0.snap ts1.snap --instr 2.8G --press TRIG@11.5G:0.4G --press PAGE@11.65G --nudge 6:1@12.4G --press YES@12.7G \
    --press FUNC@13.1G:0.4G --press YES@13.25G --press NO@13.9G
check "seed 3 over a saved ptn " ts1.snap "$($T/genref 3)"
run ts1.snap ts2.snap --instr 1.0G --press NO@14.3G:0.4G --press SCNE@14.45G
check "RELOAD PATTERN: seed 2  " ts2.snap "$($T/genref 2)"
# 2. power off (the working state goes to the card), cold boot on that card
cp gr-test.img pc-test.img
$B/dtboot $FW --load gr5.snap --card pc-test.img --instr 1.2G --power-off 11.0G > /dev/null
$B/dtboot $FW --instr 5.6G --card pc-test.img --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G --save pc1.snap > /dev/null
check "power cycle: seed 2     " pc1.snap "$($T/genref 2)"
# 3. modified mark: SAVE PROJECT AS slot 2 clears it; generate + keep sets it
cp gr-test.img mk-test.img
SAVEAS=$(python3 $T/presses.py 5700 "NO*6" GLOBAL "UP*9" YES "UP*2" DOWN YES "F+UP*16" DOWN YES w:100 YES w:500 2>/dev/null)
$B/dtboot $FW --load gr0.snap --card mk-test.img --instr 4.0G $SAVEAS --save mk0.snap > /dev/null
run mk0.snap mk1.snap --instr 1.0G --press NO@9.7G --press NO@9.8G --press NO@9.9G --press NO@10.0G
m1=$(mark mk1.snap)
run mk1.snap mk2.snap --instr 2.0G --press TRIG@10.7G:0.4G --press PAGE@10.85G --press PAGE@11.3G --press YES@11.6G \
    --press FUNC@12.0G:0.4G --press YES@12.15G --press NO@12.4G
m2=$(mark mk2.snap)
if [ "$m1" = "modified 0" ] && [ "$m2" = "modified 1" ]; then echo "PASS modified mark: 0 after SAVE PROJECT AS, 1 after generate + keep"
else echo "FAIL modified mark: after save '$m1', after generate '$m2'"; fi
check "generate after the save " mk2.snap "$($T/genref 1)"
fi

if part 2; then
# ---- GROOVE (gr5: seed 2 kept, page closed on GEN 1; clock 10.4 G). grooves[1] = MPC 16 57 (Live's Swing MPC 3000 16ths 57): odd steps late
run gr5.snap gv1.snap --instr 1.8G --press TRIG@10.5G:0.4G --press PAGE@10.65G --press PAGE@11.0G --press PAGE@11.15G --press PAGE@11.3G \
    --nudge 0:1@11.5G --press YES@11.8G
DTEMU_FW=$FW $B/dtpeek gv1.snap fb | python3 $T/fb2png.py $W/seedr-groove.png 4 > /dev/null
check "GROOVE: MPC 16 57 on seed 2" gv1.snap "$($T/genref 2 16 1 100 0 0 0 0 1)"
run gv1.snap gv2.snap --instr 0.4G --press NO@12.3G
check "groove NO: seed 2, µtime 0" gv2.snap "$($T/genref 2 16 0 0 0 0 0 0 1)"
run gv2.snap gv3.snap --instr 2.0G --press PAGE@12.7G --press PAGE@12.85G --press PAGE@13.0G --nudge 6:-1@13.2G --press YES@13.5G \
    --press PAGE@13.8G --press PAGE@13.95G --press PAGE@14.1G --press YES@14.3G
check "GEN seed 1, then GROOVE    " gv3.snap "$($T/genref 1 16 1 100 0 0 0 0 1)"
run gv3.snap gv4.snap --instr 0.4G --press NO@14.7G
check "groove NO: the GEN take kept" gv4.snap "$($T/genref 1 16 0 0 0 0 0 0 1)"
fi
if part 2; then
# FUNK MODERN (grooves[44], Live's: only with grooves-live.h)
if [ -f $R/mods/seedr-ar2/grooves-live.h ]; then
# FUNK MODERN (grooves[44], velocities 40..127) with VEL +48 % on the kept seed-1 take
run gv4.snap gv5.snap --instr 2.0G --nudge 0:1:8@15.1Gx5/0.08G --nudge 0:1@15.6Gx3/0.08G --nudge 2:1:8@16.0Gx6/0.08G --press YES@16.7G
DTEMU_FW=$FW $B/dtpeek gv5.snap fb | python3 $T/fb2png.py $W/seedr-groove-funk.png 4 > /dev/null
check "GROOVE: FUNK MODERN, VEL +48" gv5.snap "$($T/genref 1 16 44 100 48 0 0 0 1)"
fi
fi
if part 2; then
# Regenerate: YES with no knob turned since the last YES takes the next seed (gr5: seed 2 kept)
run gr5.snap rg1.snap --instr 1.4G --press TRIG@10.5G:0.4G --press PAGE@10.65G --press YES@11.2G
check "reopen, YES: seed 3     " rg1.snap "$($T/genref 3)"
run rg1.snap rg2.snap --instr 0.4G --press YES@11.9G
check "YES again: seed 4       " rg2.snap "$($T/genref 4)"
run rg2.snap rg3.snap --instr 0.8G --nudge 1:1:8@12.3G --press YES@12.6G
check "DENS 58%, YES: seed 4   " rg3.snap "$($T/genref 4 16 0 0 0 0 0 0 1 58 | head -1)"
fi
if part 4; then
# Sample locks (GEN 3): SMP 100 %, slots 1..8, on gr5 (seed 2 kept, page GEN 1; clock 10.4 G).
# The nudges (8 value steps each) take time: YES well after them.
run gr5.snap sl1.snap --instr 3.2G --press TRIG@10.5G:0.4G --press PAGE@10.65G --press PAGE@11.0G --press PAGE@11.2G \
    --nudge 0:1:8@11.5Gx13/0.05G --press YES@13.4G
DTEMU_FW=$FW $B/dtpeek sl1.snap fb | python3 $T/fb2png.py $W/seedr-gen3.png 4 > /dev/null
check "SMP 100% 1-8, seed 2    " sl1.snap "$($T/genref 2 16 0 0 0 0 0 0 1 50 100 1 8)"
run sl1.snap sl2.snap --instr 0.4G --press NO@13.7G
check "smp NO: no locks        " sl2.snap "$($T/genref 2)"
run sl2.snap sl3.snap --instr 1.2G --press YES@14.1G --press FUNC@14.4G:0.4G --press YES@14.55G --press NO@15.0G
check "SMP seed 3 kept         " sl3.snap "$($T/genref 3 16 0 0 0 0 0 0 1 50 100 1 8)"
run sl3.snap sl4.snap --instr 1.6G --press YES@15.3G:0.4G --press SCNE@15.45G --press NO@16.0G:0.4G --press SCNE@16.15G
check "smp SAVE + RELOAD PATTERN" sl4.snap "$($T/genref 3 16 0 0 0 0 0 0 1 50 100 1 8)"
fi
if part 5; then
# ROTATE (gr5: seed 2 kept, page GEN 1, closed; clock 10.4 G). PAGE x4: GEN 2, GEN 3, GROOVE, ROTATE
run gr5.snap rt1.snap --instr 2.0G --press TRIG@10.5G:0.4G --press PAGE@10.65G --press PAGE@11.0G --press PAGE@11.15G \
    --press PAGE@11.3G --press PAGE@11.45G --nudge 0:1@11.7G --nudge 0:1@11.8G --press YES@12.2G
DTEMU_FW=$FW $B/dtpeek rt1.snap fb | python3 $T/fb2png.py $W/seedr-rotate.png 4 > /dev/null
check "ROTATE +3, whole steps  " rt1.snap "$($T/genref 2 16 0 0 0 0 0 0 1 50 0 1 8 3)"
run rt1.snap rt2.snap --instr 0.4G --press NO@12.5G
check "rotate NO: seed 2       " rt2.snap "$($T/genref 2)"
run rt2.snap rt3.snap --instr 0.8G --nudge 2:1@12.9G --press YES@13.3G
check "ROTATE +3 VEL (by trig) " rt3.snap "$(GENREF_ROT='3 1 0' $T/genref 2)"
# every track: SD (track 1) moved too; then NO puts it back
run rt3.snap rt4.snap --instr 1.0G --press NO@13.7G --nudge 1:1@13.9G --nudge 2:-1@14.0G --press YES@14.4G
check "ROTATE ALL +3: BD       " rt4.snap "$($T/genref 2 16 0 0 0 0 0 0 1 50 0 1 8 3)"
sd() { python3 $T/emucmp.py $B/dtpeek $FW $1 16 1 | head -1; }
want=$(python3 -c "import sys; v=sys.argv[1].split(); print(' '.join(v[(i-3)%16] for i in range(16))+' ')" "$(sd gr5.snap)")
if [ "$(sd rt4.snap)" = "$want" ]; then echo "PASS ROTATE ALL +3: SD moved   $(sd rt4.snap)"; else echo "FAIL ROTATE ALL: SD"; echo "  was  $(sd gr5.snap)"; echo "  want $want"; echo "  got  $(sd rt4.snap)"; fi
run rt4.snap rt5.snap --instr 0.4G --press NO@14.7G
if [ "$(sd rt5.snap)" = "$(sd gr5.snap)" ]; then echo "PASS rotate ALL NO: SD back"; else echo "FAIL rotate ALL NO: SD"; fi
check "rotate ALL NO: BD back  " rt5.snap "$($T/genref 2)"
# whole steps carry their locks: part 4's kept take with sample locks (sl4, page GEN 3, clock 16.8 G), +1
if [ -f sl4.snap ]; then
run sl4.snap rt6.snap --instr 1.2G --press TRIG@16.9G:0.4G --press PAGE@17.05G --press PAGE@17.4G --press PAGE@17.55G --press YES@17.8G
check "ROTATE +1 carries SMP locks" rt6.snap "$($T/genref 3 16 0 0 0 0 0 0 1 50 100 1 8 1)"
fi
fi
if part 6; then
# A test build where no new lock row fits (RT_ROWS_MAX 0): GEN 3 with SMP% > 0 must refuse and write nothing
M=$R/mods/seedr-norows-test
rm -rf $M && mkdir -p $M && cp $R/mods/seedr-ar2/seedr.c $R/mods/seedr-ar2/grooves-*.h $M/
python3 -c "import json,sys; j=json.load(open(sys.argv[1])); j['id']='seedr'; j['cflags']=['-DRT_ROWS_MAX=0']; json.dump(j, open(sys.argv[2],'w'), indent=1)" \
    $R/mods/seedr-ar2/mod.json $M/mod.json
(cd $E/elekloader && python3 -m elekloader.sdk.build $M --stock "$STOCK" | head -1 && \
 python3 -m elekloader.patch --stock "$STOCK" --mod mods/core-ar2/out/core-0.13.elemod --mod $M/out/seedr-0.3.elemod \
    --out $W/seedr-norows.syx --lockfile $W/seedr-norows.lock.json --version GR0T | tail -1)
FN=$W/seedr-norows.syx
$B/dtcard nr-test.img format > /dev/null
$B/dtboot $FN --instr 5.6G --card nr-test.img --press NO@4.2G --press NO@4.6G --press NO@5.0G --press NO@5.4G --save nr0.snap > /dev/null
$B/dtboot $FN --load nr0.snap --instr 2.45G --press TRIG@5.8G:0.4G --press PAGE@5.95G --press PAGE@6.4G --press PAGE@6.55G --press PAGE@6.7G \
    --nudge 0:1:8@6.9G --press YES@7.9G --save nr1.snap > /dev/null
DTEMU_FW=$FN $B/dtpeek nr1.snap fb | python3 $T/fb2png.py $W/seedr-norows.png 4 > /dev/null
got=$(python3 $T/emucmp.py $B/dtpeek $FN nr1.snap | sed -n '1p;4p' | tr '\n' '|')
if [ "$got" = "$ORIG|slock -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1|" ]; then echo "PASS no lock rows: nothing written ($W/seedr-norows.png)"; else echo "FAIL no lock rows: $got"; fi
fi
