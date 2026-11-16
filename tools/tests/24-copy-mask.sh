#!/bin/sh
# Task F4: RHYTHM SRC = COPY and PLACE MASK (AVOID, ONLY) on A01 BD against SD, each take compared
# with tools/seedref (SD's dump as seedref's other=1:FILE). The preview (F10) reads the other
# tracks from the context: after SD is changed with stock's keys (TRK + T2, grid recording, the TRIG keys,
# TRK + T1) it renders again and follows, for COPY and for MASK, and the next YES writes it.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 24-copy-mask
emu_build

# pv_trigs: the preview's trigs (pv_out, preview.c) as seedref's trig row
pv_trigs() {
    _pt_a=$(emu_addr pv_out)
    _pt_l=$(emu_field lib/seed.h:seed_track length | cut -d' ' -f1)
    _pt_s=$(emu_field lib/seed.h:seed_track s | cut -d' ' -f1)
    _pt_z=$(emu_field lib/seed.h:seed_step sizeof | cut -d' ' -f1)
    _pt_f=$(emu_field lib/seed.h:seed_step flags | cut -d' ' -f1)
    _pt_n=$(emu_py peek "$EMU_SNAP" "$(printf '0x%x' $((_pt_a + _pt_l)))" 2 1 -s)
    emu_py peek "$EMU_SNAP" "$(printf '0x%x' $((_pt_a + _pt_s)))" 2 $((_pt_n * _pt_z / 2)) |
        tr -s ' \n' '\n' | awk -v z=$((_pt_z / 2)) -v f=$((_pt_f / 2)) '
            NF { if ((NR - 1) % z == f) r = r " " ($1 % 2 ? "x" : ".") } END { print "trig" r }'
}
trigs() { sed -n 2p; }
renders() { emu_get pv_renders; }
# check_preview LABEL SEEDREF-ARG...: the preview's trigs are seedref's take's
check_preview() {
    _cp_l=$1; shift
    emu_check "$_cp_l: the preview" "$(seedref "$@" | trigs)" "$(pv_trigs)"
}
# sd_record KEY...: on SD, TRIG keys in grid recording, then back to BD (stock's keys)
sd_record() {
    emu_keys TRK+T2
    emu_rec 1                                   # grid recording (it may be on already)
    emu_keys "$@" TRK+T1
    emu_dump 1 > "$D/sd.txt"
}

# BD: seed 2 kept (RANDOM); SD: the trigs a new project's SD has
emu_fixture kept2 fix_kept2
save_orig kept2
emu_dump 1 > "$D/sd.txt"
K="orig=$D/kept2.txt"
O="other=1:$D/sd.txt"
emu_check "SD has trigs to copy" 1 "$(emu_dump 1 | trigs | grep -c x)"

# ---- 1. COPY: BD takes SD's trig positions ----
emu_open
emu_page RHYTHM
emu_knob RHYTHM A:src:one 4
emu_knob RHYTHM B:copy_trk 1
check_preview "SRC COPY, TRK SD, before YES" src=copy copy_trk=1 "$K" "$O"
emu_yes
emu_shot rhythm-copy
check_take "SRC COPY, TRK SD: YES" src=copy copy_trk=1 "$K" "$O"
emu_check "SRC COPY: BD's trigs are SD's" "$(emu_dump 1 | trigs)" "$(emu_dump 0 | trigs)"
emu_no
check_take "NO: back to the original" "$K"

# ---- 2. the preview follows a change on the copied track ----
r0=$(renders)
cp "$D/sd.txt" "$D/sd0.txt"
sd_record T1 T4 T16
emu_check "SD changed with stock's keys" 1 "$([ "$(cat "$D/sd.txt")" != "$(cat "$D/sd0.txt")" ] && echo 1)"
emu_check "back on BD, seedr open, BD untouched" "1 $(cat "$D/kept2.txt")" "$(emu_get open) $(emu_dump 0)"
emu_page RHYTHM
check_preview "SD changed: the preview follows" src=copy copy_trk=1 "$K" "$O"
emu_check "SD changed: rendered again" 1 "$([ $(($(renders) - r0)) -ge 1 ] && echo 1)"
emu_yes
check_take "SD changed: YES copies the new SD" src=copy copy_trk=1 "$K" "$O"
emu_no

# ---- 3. MASK on BD's RANDOM take against SD (section 2's YES with no knob turned took seed 3) ----
emu_knob RHYTHM A:src:one 1
emu_page TAKE
emu_knob TAKE A:seed 2
emu_page PLACE
emu_knob PLACE C:mask:one 1
emu_knob PLACE D:mtrk 1
check_preview "MASK AVOID SD, before YES" src=random seed=2 mask=avoid mtrk=1 "$K" "$O"
emu_yes
emu_shot place-mask
check_take "MASK AVOID SD: YES" src=random seed=2 mask=avoid mtrk=1 "$K" "$O"
emu_knob PLACE C:mask:one 2
emu_yes
check_take "MASK ONLY SD: YES" src=random seed=2 mask=only mtrk=1 "$K" "$O"
emu_no

# ---- 4. SOURCE = TRACK with ONLY: the kept steps keep their data ----
emu_page RHYTHM
emu_knob RHYTHM A:src:one 0
emu_page PLACE
check_preview "SRC TRACK, MASK ONLY SD, before YES" mask=only mtrk=1 "$K" "$O"
emu_yes
check_take "SRC TRACK, MASK ONLY SD: YES" mask=only mtrk=1 "$K" "$O"
emu_no

# ---- 5. the preview follows a change on the masked track; keep ----
r0=$(renders)
sd_record T2 T8
emu_page PLACE
check_preview "SD changed: the mask follows" mask=only mtrk=1 "$K" "$O"
emu_check "SD changed: rendered again" 1 "$([ $(($(renders) - r0)) -ge 1 ] && echo 1)"
emu_yes
emu_keep
check_take "SD changed: YES, keep" mask=only mtrk=1 "$K" "$O"
emu_no
emu_check "NO: closed" 0 "$(emu_get open)"
check_take "closed: the take kept" mask=only mtrk=1 "$K" "$O"

# ---- 6. COPY from a track in stock's Euclidean mode takes the pattern that mode plays (the device's
# context reads its table, record +0x2D1), not the trigs stored on the track (lead, after F4) ----
SD=$((0x407dd8e3 + 0x395))                      # A01 SD's record, after BD's (tools/emu.py)
sd_euclid() {                                   # SD's Euclidean table, its first 16 steps, as a trig row
    emu_py peek "$EMU_SNAP" "$((SD + 0x2d1))" 1 16 | tr -s ' \n' '\n' |
        awk 'NF { r = r ($1 == 255 ? " ." : " x") } END { print "trig" r }'
}
emu_keys TRK+T2
for _ in 1 2 3; do                              # TRIG until stock's EUCLIDEAN page, seedr closed
    [ "$(emu_get open) $(emu_py trigpage "$MAP" "$EMU_SNAP")" = "0 1" ] && break
    emu_keys TRIG
done
emu_check "SD: stock's EUCLIDEAN page, seedr closed" "0 1" "$(emu_get open) $(emu_py trigpage "$MAP" "$EMU_SNAP")"
emu_sknob 3 $((SD + 0x2ca)) 1                   # EUC on
emu_sknob 0 $((SD + 0x2cb)) 5                   # PL1 5
EU=$(sd_euclid)
emu_check "SD: Euclidean mode plays other steps than SD's stored trigs" 1 \
    "$([ "$EU" != "$(emu_dump 1 | trigs)" ] && echo 1)"
emu_keys TRK+T1
emu_open
emu_page PLACE
emu_knob PLACE C:mask:one 0
emu_page RHYTHM
emu_knob RHYTHM A:src:one 4
emu_yes
emu_shot rhythm-copy-euclid
emu_check "COPY SD in Euclidean mode: BD's trigs are the pattern that mode plays" "$EU" "$(emu_dump 0 | trigs)"
emu_done
