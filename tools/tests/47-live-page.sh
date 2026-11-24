#!/bin/sh
# Task T3 (docs/plan-turing.md): TURING's live page on the Rytm (design L6, L7, L13). On A01 BD:
#   1. TURING is the ring's last page; YES with RECORD off says RECORD TO GENERATE
#   2. YES (CRPT, AMT 80) writes the offline take of the run (seedref live=0:0); PLAY: the steps the
#      playhead passes change; the track is what seedref renders after the same steps from the same
#      start (live=FROM:N, the mod's live_from and live_count); the other tracks unchanged
#   3. FUNC + UP remembers the cells; AMT 80 for a while; AMT 0 (locked); FUNC + DOWN: the cells are the
#      ones remembered
#   4. FUNC + YES keeps the track as it stands and stops: no change after it
#   5. YES again, then NO: the applied track back byte for byte (I7, T3); YES, RECORD off: NOT APPLIED,
#      the track back; YES, TRIG (closing): NOT APPLIED, the track back
#   6. the FX track: YES on TURING says NOT ON THE FX TRACK and writes nothing
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 47-live-page
emu_build
pending() { emu_get auditioning; }             # 3: a live run (frame.c A_LIVE)
# every track but BD, as one digest
others() { _o_t=1; while [ $_o_t -le 12 ]; do emu_dump $_o_t; _o_t=$((_o_t + 1)); done | shasum | cut -d' ' -f1; }
# BD's recipe as seedref keys, every field
rec_keys() {
    seedref --recipe | while read -r _rk_p _rk_v; do
        _rk_k=$(echo "$_rk_p" | sed 's/^lane\[0\]\.val\./lane1./; s/^lane\[1\]\.val\./lane2./; s/^lane\[0\]/lane1/; s/^lane\[1\]/lane2/')
        printf '%s=%s ' "$_rk_k" "$(emu_get "recipe[0].$_rk_p")"
    done
}
# the live machine's cells (struct seed_turing: five int16_t, bit[16], val[16]) and the remembered ones
cells() { emu_py peek "$EMU_SNAP" "$(emu_addr "$1")" 1 58 | tr '\n' ' ' | cut -d' ' -f11-58; }

emu_boot
save_orig orig
emu_open
emu_page TURING
emu_check "TURING: the ring's last page" 10 "$(emu_get page)"
emu_shot turing-page
emu_knob TURING A:tur_flav:one 1
emu_knob TURING C:tur_amt 80
emu_rec 0
emu_keys YES
emu_check "RECORD off: RECORD TO GENERATE" "RECORD TO GENERATE" "$(emu_msg)"

# 2. YES, then PLAY
OT0=$(others)
emu_rec 1
emu_keys YES
emu_check "YES on TURING: a live run" "3 LIVE" "$(pending) $(emu_msg)"
K=$(rec_keys)
# shellcheck disable=SC2086
emu_check "YES: the offline take of the run" "$(seedref $K live=0:0 orig="$D/orig.txt")" "$(emu_dump)"
emu_dump > "$D/start.txt"
emu_keys PLAY
emu_run -s 3G                                   # about 15 s at 120 BPM: 120 sixteenths, 7 loops
emu_shot turing-live
N=$(($(emu_get live_count) - 16)) FROM=$(emu_get live_from)
emu_check "PLAY: steps played live" 1 "$([ "$N" -gt 32 ] && echo 1)"
# shellcheck disable=SC2086
emu_check "live: the track is seedref's after the same steps (live=$FROM:$N)" \
    "$(seedref $K live="$FROM:$N" orig="$D/orig.txt")" "$(emu_dump)"
emu_check "live: the track changed (CRPT 80 flips trigs)" 1 "$([ "$(emu_dump)" != "$(cat "$D/start.txt")" ] && echo 1)"
emu_check "live: the other tracks unchanged" "$OT0" "$(others)"

# 3. remember, change, lock, recall
emu_keys F+UP
emu_check "FUNC + UP: REMEMBERED" "REMEMBERED" "$(emu_msg)"
MEM=$(cells live_mem)
emu_run -s 1G
emu_knob TURING C:tur_amt 0
emu_keys F+DOWN
emu_check "FUNC + DOWN: RECALLED" "RECALLED" "$(emu_msg)"
emu_run -s 600M                                  # AMT 0 locks CRPT: the cells stay as recalled
emu_check "FUNC + DOWN: the cells remembered, held by AMT 0" "$MEM" "$(cells live_m)"

# 4. FUNC + YES: kept, stopped
emu_keep
emu_check "FUNC + YES: APPLIED, no run" "APPLIED 0" "$(emu_msg) $(pending)"
emu_dump > "$D/kept.txt"
emu_run -s 1G
emu_check "after FUNC + YES: nothing changes" "$(cat "$D/kept.txt")" "$(emu_dump)"

# 5. NO, RECORD off, closing: the track back
emu_knob TURING C:tur_amt 80
emu_keys YES
emu_run -s 1G
emu_no
emu_check "NO: BACK TO THE ORIGINAL, no run" "BACK TO THE ORIGINAL 0" "$(emu_msg) $(pending)"
emu_check "NO: the track byte for byte as before YES" "$(cat "$D/kept.txt")" "$(emu_dump)"
emu_keys YES
emu_run -s 1G
emu_rec 0
emu_check "RECORD off while live: NOT APPLIED" "NOT APPLIED 0" "$(emu_msg) $(pending)"
emu_check "RECORD off: the track back" "$(cat "$D/kept.txt")" "$(emu_dump)"
emu_rec 1
emu_keys YES
emu_run -s 1G
emu_keys TRIG
emu_check "closing while live: NOT APPLIED, closed" "NOT APPLIED 0 0" "$(emu_msg) $(pending) $(emu_get open)"
emu_check "closing: the track back" "$(cat "$D/kept.txt")" "$(emu_dump)"
emu_check "the other tracks unchanged throughout" "$OT0" "$(others)"

# 6. the FX track
emu_keys STOP
emu_rec 0
emu_keys TRK+FX
emu_open
emu_page TURING
emu_rec 1
emu_keys YES
emu_check "FX track: NOT ON THE FX TRACK, no run" "NOT ON THE FX TRACK 0" "$(emu_msg) $(pending)"
emu_done
