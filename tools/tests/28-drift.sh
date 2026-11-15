#!/bin/sh
# Task F8: DRFT on the device, each take against tools/seedref.
#   1. a RANDOM take (kept2's seed 2) with DRFT 100: the drift is written to the trigs' micro timing,
#      inside the Rytm's -23..+23; DRFT with RAND; VARY 100 (the next seed keeps the drift); NO
#   2. SOURCE = TRACK: DRFT on the kept take's own trigs; kept, SAVE and RELOAD PATTERN keep it (I9)
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 28-drift

# utimes: BD's micro timing on its trigs, as "count-not-zero lowest highest"
utimes() {
    emu_dump | python3 -c "
import sys
rows = {l.split()[0]: l.split()[1:] for l in sys.stdin if l.strip()}
u = [int(v) for t, v in zip(rows['trig'], rows['utime']) if t == 'x']
print(sum(1 for v in u if v), min(u), max(u))"
}

# ---- 1. a RANDOM take ----
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_page TIME
emu_knob TIME F:drft 100
emu_yes
emu_shot time-drift
check_take "RANDOM seed 2, DRFT 100" src=random seed=2 drft=100 "$K"
set -- $(utimes)
emu_check "DRFT 100: most trigs moved, inside -23..+23" "ok" \
    "$([ "$1" -ge 4 ] && [ "$2" -ge -23 ] && [ "$3" -le 23 ] && echo ok || echo "$*")"
emu_knob TIME E:rand 30
emu_knob TIME F:drft 40
emu_yes
check_take "RAND 30, DRFT 40" src=random seed=2 rand=30 drft=40 "$K"
emu_page TAKE
emu_knob TAKE B:vary 100
emu_yes
check_take "VARY 100, DRFT 40: seed 2" src=random seed=2 vary=100 rand=30 drft=40 "$K"
emu_yes
check_take "YES again: seed 3, the same take (VARY 100)" src=random seed=3 vary=100 rand=30 drft=40 "$K"
emu_check "seedref: VARY 100 gives seeds 2 and 3 the same take" \
    "$(seedref src=random seed=2 vary=100 rand=30 drft=40 "$K")" "$(seedref src=random seed=3 vary=100 rand=30 drft=40 "$K")"
emu_no
check_take "NO: back to the kept seed 2" "$K"

# ---- 2. SOURCE = TRACK ----
emu_knob TAKE B:vary 0
emu_knob TAKE A:seed 2
emu_page RHYTHM
emu_knob RHYTHM A:src:one 0
emu_page TIME
emu_knob TIME E:rand 0
emu_knob TIME F:drft 60
emu_yes
emu_shot time-drift-track
check_take "SOURCE = TRACK, DRFT 60: the kept take's trigs" src=track seed=2 drft=60 "$K"
emu_keep
emu_no
emu_keys YES+SCNE w:300 NO+SCNE
check_take "kept, SAVE + RELOAD PATTERN: the drift" src=track seed=2 drft=60 "$K"
emu_done
