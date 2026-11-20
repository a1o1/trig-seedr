#!/bin/sh
# Task T2 (docs/plan-turing.md): the Rytm's 46 on the Syntakt. TURING offline through the pages' knobs, against
# tools/seedref --device st:
#   1. RHYTHM SRC = TURG: DJVU at AMT 25 and 50, CRPT at 40 and 100, SHFT at 0 and 50, STRT = TRACK
#   2. NOTE from the machine (NSRC = TURG), SCALE MAJ, then ROOT one up with FUNC + knob
#   3. VEL (DYNAMICS VSRC = TURG) and LOCK lane 1 (SRC = TURG) from the machine
#   4. NO puts the original back; YES brings the take back; FUNC + YES keeps it (APPLIED)
# On track 4 from the kept2 fixture (seed 2 kept).
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st46-turing
st_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"

# ---- 1. SRC = TURG and its knobs ----
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 5
emu_yes
emu_shot rhythm-turg
TU="src=turing tur_flav=djvu tur_loop=8 tur_amt=25 tur_bias=50"
check_take "SRC TURG, DJVU AMT 25 (the defaults)" seed=2 $TU "$K"
emu_knob RHYTHM D:tur_amt 50
emu_yes
TU="src=turing tur_flav=djvu tur_loop=8 tur_amt=50 tur_bias=50"
check_take "DJVU AMT 50: the loop repeats" seed=2 $TU "$K"
emu_knob RHYTHM B:tur_flav:one 1
emu_knob RHYTHM D:tur_amt 40
emu_knob RHYTHM C:tur_loop 6
emu_yes
TU="src=turing tur_flav=crpt tur_loop=6 tur_amt=40 tur_bias=50"
check_take "CRPT AMT 40, LOOP 6" seed=2 $TU "$K"
emu_knob RHYTHM D:tur_amt 100
emu_knob RHYTHM E:tur_bias 70
emu_yes
TU="src=turing tur_flav=crpt tur_loop=6 tur_amt=100 tur_bias=70"
check_take "CRPT AMT 100, BIAS 70" seed=2 $TU "$K"
emu_knob RHYTHM B:tur_flav:one 2
emu_knob RHYTHM D:tur_amt 0
emu_yes
TU="src=turing tur_flav=shft tur_loop=6 tur_amt=0 tur_bias=70"
check_take "SHFT AMT 0: the loop inverts each pass" seed=2 $TU "$K"
emu_knob RHYTHM D:tur_amt 50
emu_knob RHYTHM F:tur_strt:one 1
emu_yes
TU="src=turing tur_flav=shft tur_loop=6 tur_amt=50 tur_bias=70 tur_strt=track"
check_take "SHFT AMT 50, STRT TRACK" seed=2 $TU "$K"

# ---- 2. NOTE from the machine, SCALE and ROOT ----
emu_page NOTE
emu_run -s 300M --nudge 2:1@+600M                     # NOTE leaves DEF: 0..0
# the range by poke: dtboot's FUNC-held nudge does not reach the Syntakt's FUNC, so FUNC + knob (the low end)
# cannot be driven here; the Rytm's 46 turns the same knob to -12, and 14-fields sets its ranges by poke too
emu_poke "recipe[$EMU_TRACK].note.hi" 12
emu_poke "recipe[$EMU_TRACK].note.lo" -12
emu_run -s 200M
emu_check "NOTE -12..+12 (poked)" "-12 12" "$(emu_get "recipe[$EMU_TRACK].note.lo") $(emu_get "recipe[$EMU_TRACK].note.hi")"
emu_knob NOTE D:note.src:one 3
emu_yes
N="note=-12:12 note.src=turg"
check_take "NSRC TURG over -12..+12" seed=2 $TU $N "$K"
emu_knob NOTE H:scale:one 2
emu_yes
emu_shot note-scale
check_take "SCALE MAJ (ROOT C)" seed=2 $TU $N scale=maj root=0 "$K"
# ROOT by poke: dtboot's FUNC-held nudge does not reach the Syntakt's FUNC (the same cause as the ranges above); the
# Rytm's 46 turns ROOT with FUNC + SCALE
emu_poke "recipe[$EMU_TRACK].root" 1
emu_run -s 200M
emu_check "ROOT C# (poked)" 1 "$(emu_get "recipe[$EMU_TRACK].root")"
emu_yes
emu_shot note-scale-root
N="$N scale=maj root=1"
check_take "SCALE MAJ, ROOT C#" seed=2 $TU $N "$K"

# ---- 3. VEL and LOCK lane 1 from the machine ----
emu_page DYNAMICS
emu_run -s 300M --nudge 0:1@+600M                     # VEL leaves DEF: 100..100
emu_poke "recipe[$EMU_TRACK].vel.lo" 40
emu_run -s 200M
emu_knob DYNAMICS H:vel.src:one 3
emu_yes
V="vel=40:100 vel.src=turg"
check_take "DYNAMICS VSRC TURG over 40..100" seed=2 $TU $N $V "$K"
emu_page LOCK
emu_knob LOCK B:lane1.pct 100
emu_knob LOCK D:lane1.src:one 3
emu_yes
ST_SLOT=$(st_slot $EMU_TRACK "$(emu_get "recipe[$EMU_TRACK].lane[0].parm")")
L="lane1.pct=100 lane1.src=turg"
check_take "LOCK lane 1 SRC TURG" seed=2 $TU $N $V $L "$K"

# ---- 4. NO, YES, FUNC + YES ----
emu_no
check_take "NO: the original" "$K"
emu_yes
check_take "YES after NO: the same take" seed=2 $TU $N $V $L "$K"
emu_keep
emu_check "FUNC + YES: APPLIED" "APPLIED" "$(emu_msg)"
check_take "kept" seed=2 $TU $N $V $L "$K"
ST_SLOT=
emu_done
