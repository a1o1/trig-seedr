#!/bin/sh
# Task S3 (docs/plan-syntakt.md): seedr on the Syntakt OS 1.42 in analog-emu: mods/seedr-st on the hook-bus core
# (tools/st_build.sh --core mini), with the device layer (device/syntakt_st.h).
#   1. boot: the heap memory is attached at the first tick; TRIG, TRIG, TRIG opens seedr as a third TRIG page
#   2. LEFT / RIGHT reach every page the Syntakt shows; a screenshot of each (build/emu/st-s3/<page>.png)
#   3. knobs change the recipe; RECORD off: YES says RECORD TO GENERATE and writes nothing
#   4. RECORD on: YES writes a take on track 1 (a take pending, the track changed); NO puts the record and the
#      lock store back byte for byte; NO again closes
#   5. the FX track: YES says NOT ON THE FX TRACK
# The take's comparison with tools/seedref is task S4's. Needs ../analog-emu-st (status/S0.3.md; ST_B overrides).
# Writes only under build/emu/st-s3/ and build/emu-st/.
R=$(cd "$(dirname "$0")/.." && pwd)
case $R in
*/.claude/worktrees/*) _e=${WORKSPACE:-$(cd "${R%/.claude/worktrees/*}/.." && pwd)} ;;
*) _e=${WORKSPACE:-$(cd "$R/.." && pwd)} ;;
esac
B=${ST_B:-$_e/analog-emu-st/target/release}
. "$R/tools/emu_st.sh"
emu_init st-s3
FW=$(sh "$T/st_build.sh" seedr "$R/mods/seedr-st" | tail -1) || emu_die "st_build.sh"
MAP=$FW.map.json DTEMU_FW=$FW EMU_PROFILE=v1
export DTEMU_FW T
peek() { python3 "$T/st_peek.py" "$@"; }

# emu_rec 0|1: grid recording, read as the Syntakt keeps it (tools/st_peek.py rec)
emu_rec() {
    [ "$(peek rec "$EMU_SNAP")" = "$1" ] || emu_keys RECORD
    [ "$(peek rec "$EMU_SNAP")" = "$1" ] || emu_die "grid recording did not turn $1"
}
# mem rec|locks [TRACK]: a track's record or lock store as hex (pattern A01)
mem() {
    python3 - "$EMU_SNAP" "$1" "${2:-0}" <<'PY'
import os, sys
sys.path.insert(0, os.environ['T'])
import st_peek as s, emu
snap, what, t = sys.argv[1], sys.argv[2], int(sys.argv[3])
if what == 'rec':
    a, n = s.record(snap, t, 0), 0x499
else:
    a, n = s.u32(snap, s.pattern(snap, 0) + s.LOCK_MODEL + 0x10) + s.LOCK_TRACK * t, s.LOCK_TRACK
print(emu.mem(snap, a, n).hex())
PY
}

# 1. boot, open
emu_boot
emu_check "the heap memory is attached" 1 "$(emu_get st_ready)"
emu_keys TRIG TRIG
emu_check "TRIG, TRIG: stock's Euclidean page" 1 "$(peek trigpage "$EMU_SNAP")"
emu_keys TRIG
emu_check "TRIG on the Euclidean page: seedr opens" 1 "$(emu_get open)"
emu_shot open

# 2. every page: RIGHT from TAKE until TAKE again
SEEN=
for _i in 1 2 3 4 5 6 7 8 9 10 11; do
    _p=$(emu_get page)
    _n=$(echo $(emu_pages) | cut -d' ' -f$((_p + 1)))
    case " $SEEN " in *" $_n "*) break ;; esac
    SEEN="$SEEN $_n"
    emu_shot "page-$_n"
    emu_keys RIGHT
done
emu_check "RIGHT steps through the pages the Syntakt shows" \
    " TAKE RHYTHM PLACE DYNAMICS NOTE LOCK TRIG TIME VIEW TOOLS" "$SEEN"
emu_keys LEFT
emu_check "LEFT from TAKE: TOOLS" 9 "$(emu_get page)"
emu_page TAKE

# 3. knobs; YES with grid recording off
emu_knob TAKE A:seed 5
emu_check "knob A on TAKE: SEED 5" 5 "$(emu_get 'recipe[0].seed')"
emu_page RHYTHM
emu_knob RHYTHM A:src:one 1
emu_check "knob A on RHYTHM: SRC RAND" 1 "$(emu_get 'recipe[0].src')"
emu_shot rhythm-rand
REC0=$(mem rec) LKS0=$(mem locks)
emu_rec 0
emu_keys YES
emu_check "RECORD off, YES: RECORD TO GENERATE" "RECORD TO GENERATE" "$(emu_msg)"
emu_check "RECORD off, YES: nothing written" "$REC0$LKS0" "$(mem rec)$(mem locks)"

# 4. YES writes a take, NO restores it
emu_yes
emu_shot take
emu_check "RECORD on, YES: a take pending" 1 "$(emu_get auditioning)"
emu_check "RECORD on, YES: track 1 changed" 1 "$([ "$(mem rec)" != "$REC0" ] && echo 1)"
emu_check "the take has trigs" 1 "$(peek dump "$EMU_SNAP" 0 | grep -c '^trig.* x')"
emu_no
emu_check "NO: BACK TO THE ORIGINAL, nothing pending" "BACK TO THE ORIGINAL 0" "$(emu_msg) $(emu_get auditioning)"
emu_check "NO: the record byte for byte" "$REC0" "$(mem rec)"
emu_check "NO: the lock store byte for byte" "$LKS0" "$(mem locks)"
emu_no
emu_check "NO again: closed" 0 "$(emu_get open)"

# 5. the FX track (FX selects it), seedr opened with TRIG held + PAGE
emu_rec 0
emu_keys FX
emu_open
emu_check "FX: seedr opens" 1 "$(emu_get open)"
emu_shot fx
emu_yes
emu_check "FX track, YES: NOT ON THE FX TRACK" "NOT ON THE FX TRACK" "$(emu_msg)"

# 6. stock's popups (task P1): seedr steps aside while one shows, and draws again after it; FUNC held
# (QuickMuteMenuView) keeps seedr's screen, so FUNC + YES and FUNC + knob still show it
emu_keys TRK+T1
emu_open
emu_run -s 2400M                        # the strip's 2 s message gone
fb() { "$DTPEEK" "$EMU_SNAP" fb | shasum | cut -c1-16; }
OPEN=$(fb)
HOME_SNAP=$EMU_SNAP
held() {
    emu_at "$HOME_SNAP"
    if [ "${1#F+}" != "$1" ]; then
        emu_run -s 200M --press FUNC@+50M:2100M --press "${1#F+}@+100M:2000M"
    else
        emu_run -s 200M --press "$1@+100M:2000M"
    fi
}
for k in TEMPO PTN TRK F+TEMPO; do
    held "$k"
    emu_shot "popup-$(echo "$k" | tr '+' '_')"
    emu_check "$k held: seedr steps aside (stock's screen)" 1 "$([ "$(fb)" != "$OPEN" ] && echo 1)"
    emu_check "$k held: seedr stays open" 1 "$(emu_get open)"
done
emu_at "$HOME_SNAP"
emu_run -s 200M --press FUNC@+100M:2000M
emu_check "FUNC held (QuickMuteMenuView): seedr's screen stays" "$OPEN" "$(fb)"
emu_check "FUNC held: QuickMuteMenuView is up" 1 "$(python3 "$T/st_views.py" "$EMU_SNAP" "$(emu_get ui_ctrl)" | grep -c QuickMuteMenuView)"
# PTN latches stock's pattern select ("A??", waiting for a trig key); PTN again cancels it
emu_at "$HOME_SNAP"
emu_keys PTN
emu_check "PTN: pattern select, seedr aside" 1 "$([ "$(fb)" != "$OPEN" ] && echo 1)"
emu_keys PTN
emu_run -s 600M
emu_check "PTN again: pattern select cancelled, seedr draws again" "$OPEN" "$(fb)"
emu_done
