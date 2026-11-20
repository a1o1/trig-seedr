#!/bin/sh
# The Rytm's 47 on the Syntakt (plan-turing.md T3): TURING's live page (design L6, L7, L13). On track 4 from
# the kept2 fixture:
#   1. TURING is the ring's last page; YES with RECORD off says RECORD TO GENERATE
#   2. YES (CRPT, AMT 80) writes the offline take of the run (seedref live=0:0); PLAY: the steps the
#      playhead passes change; the track is what seedref renders after the same steps from the same start
#      (live=FROM:N); the other tracks unchanged
#   3. FUNC + UP remembers the cells; AMT 0 (locked) after a while; FUNC + DOWN: the cells remembered
#   4. FUNC + YES keeps the track as it stands and stops: no change after it
#   5. YES then NO, YES then RECORD off, YES then TRIG: the kept track back byte for byte (record and locks)
#   6. the FX track: YES on TURING says NOT ON THE FX TRACK and writes nothing
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/tests-st/st.sh"; . "$R/tools/tests-st/fixtures.sh"
emu_init st47-live-page
st_build
pending() { emu_get auditioning; }             # 3: a live run (frame.c A_LIVE)
# every track but track 4, records and lock stores, as one digest
others() {
    for _o_t in 0 1 2 4 5 6 7 8 9 10 11 12; do st_mem rec $_o_t; st_mem locks $_o_t; done | shasum | cut -d' ' -f1
}
mine() { st_mem rec $EMU_TRACK; st_mem locks $EMU_TRACK; }
# track 4's recipe as seedref keys, every field
rec_keys() {
    seedref --recipe | while read -r _rk_p _rk_v; do
        _rk_k=$(echo "$_rk_p" | sed 's/^lane\[0\]\.val\./lane1./; s/^lane\[1\]\.val\./lane2./; s/^lane\[0\]/lane1/; s/^lane\[1\]/lane2/')
        printf '%s=%s ' "$_rk_k" "$(emu_get "recipe[$EMU_TRACK].$_rk_p")"
    done
}
# the live machine's cells (struct seed_turing: five int16_t, bit[16], val[16])
cells() { emu_py peek "$EMU_SNAP" "$(emu_addr "$1")" 1 58 | tr '\n' ' ' | cut -d' ' -f11-58; }

emu_fixture kept2 fix_kept2
save_orig kept2
LEN=$(sed -n 's/^length //p' "$D/kept2.txt")
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
emu_check "YES: the offline take of the run" "$(seedref track=$EMU_TRACK $K live=0:0 orig="$D/kept2.txt")" \
    "$(emu_dump $EMU_TRACK)"
emu_dump $EMU_TRACK > "$D/start.txt"
emu_keys PLAY
emu_run -s 3G                                   # about 15 s of PLAY
emu_shot turing-live
N=$(($(emu_get live_count) - LEN)) FROM=$(emu_get live_from)
emu_check "PLAY: steps played live" 1 "$([ "$N" -gt 2 ] && echo 1)"
# shellcheck disable=SC2086
emu_check "live: the track is seedref's after the same steps (live=$FROM:$N)" \
    "$(seedref track=$EMU_TRACK $K live="$FROM:$N" orig="$D/kept2.txt")" "$(emu_dump $EMU_TRACK)"
emu_check "live: the track changed (CRPT 80)" 1 "$([ "$(emu_dump $EMU_TRACK)" != "$(cat "$D/start.txt")" ] && echo 1)"
emu_check "live: the other tracks unchanged" "$OT0" "$(others)"

# 3. remember, lock, recall
emu_keys F+UP
emu_check "FUNC + UP: REMEMBERED" "REMEMBERED" "$(emu_msg)"
MEM=$(cells live_mem)
emu_run -s 1G
emu_knob TURING C:tur_amt 0
emu_keys F+DOWN
emu_check "FUNC + DOWN: RECALLED" "RECALLED" "$(emu_msg)"
emu_run -s 600M
emu_check "FUNC + DOWN: the cells remembered, held by AMT 0" "$MEM" "$(cells live_m)"

# 4. FUNC + YES
emu_keep
emu_check "FUNC + YES: APPLIED, no run" "APPLIED 0" "$(emu_msg) $(pending)"
KEPT=$(mine)
emu_run -s 1G
emu_check "after FUNC + YES: nothing changes" "$KEPT" "$(mine)"

# 5. NO, RECORD off, closing
emu_knob TURING C:tur_amt 80
emu_keys YES
emu_run -s 1G
emu_no
emu_check "NO: BACK TO THE ORIGINAL, no run" "BACK TO THE ORIGINAL 0" "$(emu_msg) $(pending)"
emu_check "NO: record and locks byte for byte as before YES" "$KEPT" "$(mine)"
emu_keys YES
emu_run -s 1G
emu_rec 0
emu_check "RECORD off while live: NOT APPLIED" "NOT APPLIED 0" "$(emu_msg) $(pending)"
emu_check "RECORD off: the track back" "$KEPT" "$(mine)"
emu_rec 1
emu_keys YES
emu_run -s 1G
emu_keys TRIG
emu_check "closing while live: NOT APPLIED, closed" "NOT APPLIED 0 0" "$(emu_msg) $(pending) $(emu_get open)"
emu_check "closing: the track back" "$KEPT" "$(mine)"
emu_check "the other tracks unchanged throughout" "$OT0" "$(others)"

# 6. the FX track
emu_keys STOP
emu_rec 0
emu_keys FX
emu_open
emu_page TURING
emu_rec 1
emu_keys YES
emu_check "FX track: NOT ON THE FX TRACK, no run" "NOT ON THE FX TRACK 0" "$(emu_msg) $(pending)"
emu_done
