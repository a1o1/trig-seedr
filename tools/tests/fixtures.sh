# tools/tests/fixtures.sh: states and references several test files share (sourced after
# tools/emu.sh; tools/run_tests.sh runs only NN-*.sh). Release 0.3's checks (tools/test_seedr.sh
# parts 1-8) compare with tools/genref, the host build of lib/gen.c and lib/groove.c.

# A01 BD on a new project: no trigs, default velocities (genref's first line)
ORIG=".ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff .ff "

# genref ARG...: tools/genref's output (its usage: tools/genref.c)
genref() {
    emu_cc genref "$T/genref.c" "$R/lib/gen.c" "$R/lib/groove.c"
    "$W/genref" "$@"
}

# 1 BD line of the emulator against genref's first line, with the rest of emucmp's report
check_gen() { emu_check "$1" "$(echo "$2" | head -1)" "$(emu_cmp 1)"; }

# Release 0.3, part 1: boot, YES seed 1, seed 2 from the original, NO, FUNC + YES, closing keeps.
# Ends with seed 2 kept and the mod closed with NO on GEN 1 (tools/test_seedr.sh's gr5).
fix_kept2() {
    emu_boot
    check_gen "boot: empty track" "$ORIG"
    emu_check "boot: the track as seedref's new track (dump format)" "$(seedref)" "$(emu_dump)"
    emu_open
    emu_page GEN1
    emu_yes
    emu_shot gen1
    check_gen "YES: seed 1" "$(genref 1)"
    emu_knob GEN1 SEED 2
    emu_yes
    check_gen "seed 2, YES: from the original" "$(genref 2)"
    FLAGS_TAKE=$(emu_cmp 2)
    emu_no
    check_gen "NO: back to the original" "$ORIG"
    # SEED turned away and back: a knob turn, so this YES takes seed 2 again, from the original
    emu_knob GEN1 SEED 1
    emu_knob GEN1 SEED 2
    emu_yes
    emu_keep
    check_gen "YES, FUNC + YES: kept" "$(genref 2)"
    emu_check "audition take = fresh generate (flag words)" "$FLAGS_TAKE" "$(emu_cmp 2)"
    emu_no
    check_gen "NO: closed, the take kept" "$(genref 2)"
    emu_check "NO: the mod is closed" 0 "$(emu_get open)"
}

# check_groove LABEL GENREF: the BD line and the µtimes (genref's 2nd line, emucmp's 3rd)
check_groove() {
    emu_check "$1" "$(echo "$2" | sed -n 1p)
$(echo "$2" | sed -n 2p)" "$(emu_cmp 1)
$(emu_cmp 3)"
}

# check_smp LABEL GENREF: the BD line and the sample locks (genref's 3rd line, emucmp's 4th)
check_smp() {
    emu_check "$1" "$(echo "$2" | sed -n 1p)
$(echo "$2" | sed -n 3p)" "$(emu_cmp 1)
$(emu_cmp 4)"
}

# Release 0.3, part 4: sample locks on GEN 3 (SMP 100 %, slots 1..8), kept, saved and reloaded.
# From kept2; ends with seed 3 kept with sample locks, closed on GEN 3 (test_seedr.sh's sl4).
fix_smp3() {
    emu_fixture kept2 fix_kept2
    emu_open
    emu_page GEN3
    emu_knob GEN3 SMP 100
    emu_yes
    emu_shot gen3
    check_smp "SMP 100 %, slots 1-8, seed 2" "$(genref 2 16 0 0 0 0 0 0 1 50 100 1 8)"
    emu_no
    check_smp "NO: no sample locks" "$(genref 2)"
    emu_yes
    emu_keep
    emu_no
    check_smp "YES, keep: seed 3 with sample locks" "$(genref 3 16 0 0 0 0 0 0 1 50 100 1 8)"
    emu_keys YES+SCNE w:300 NO+SCNE
    check_smp "SAVE + RELOAD PATTERN keeps them" "$(genref 3 16 0 0 0 0 0 0 1 50 100 1 8)"
}
