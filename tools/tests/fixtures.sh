# tools/tests/fixtures.sh: states and checks several test files share (sourced after
# tools/emu.sh; tools/run_tests.sh runs only NN-*.sh). The emulator's A01 BD is compared with
# tools/seedref, the host build of the same chain, in the dump format (tools/emu.py dump).

# check_take LABEL [SEEDREF-ARG...]: BD is what seedref renders for those settings (no settings:
# a new project's track, unchanged)
check_take() {
    _ct_l=$1; shift
    emu_check "$_ct_l" "$(seedref "$@")" "$(emu_dump)"
}

# save_orig NAME: BD as it is now, as $D/NAME.txt, for seedref's orig=
save_orig() { emu_dump > "$D/$1.txt"; }

# Release 0.3's part 1 on version 1: boot; open on the empty BD (its new recipe is RANDOM, V6);
# YES seed 1; seed 2 from the original; NO; FUNC + YES; closing keeps. Ends with seed 2 kept and
# the mod closed with NO on TAKE.
fix_kept2() {
    emu_boot
    check_take "boot: the track as seedref's new track"
    emu_open
    emu_check "open: the empty track's new recipe is RANDOM (V6)" 1 "$(emu_get 'recipe[0].src')"
    emu_yes
    emu_shot take1
    check_take "YES: seed 1" src=random seed=1
    emu_knob TAKE A:seed 2
    emu_yes
    check_take "SEED 2, YES: from the original" src=random seed=2
    emu_no
    check_take "NO: back to the original"
    # SEED turned away and back: a knob turn, so this YES takes seed 2 again, from the original
    emu_knob TAKE A:seed 1
    emu_knob TAKE A:seed 2
    emu_yes
    emu_keep
    check_take "YES, FUNC + YES: kept" src=random seed=2
    emu_no
    check_take "NO: closed, the take kept" src=random seed=2
    emu_check "NO: the mod is closed" 0 "$(emu_get open)"
}

# Release 0.3's part 4 on version 1: LOCK lane 1 (SMP, the sample slot) at 100 %, slots 1..8, kept,
# saved and reloaded. From kept2; ends with seed 3 kept with sample locks, closed on LOCK.
fix_smp3() {
    emu_fixture kept2 fix_kept2
    save_orig kept2
    emu_open
    emu_page LOCK
    emu_knob LOCK B:lane1.pct 100
    emu_yes
    emu_shot lock
    check_take "LOCK SMP 100 %, slots 1-8, seed 2" src=random seed=2 lane1.pct=100 orig="$D/kept2.txt"
    emu_no
    check_take "NO: no sample locks" src=random seed=2 orig="$D/kept2.txt"
    emu_yes
    emu_keep
    emu_no
    check_take "YES, keep: seed 3 with sample locks" src=random seed=3 lane1.pct=100 orig="$D/kept2.txt"
    emu_keys YES+SCNE w:300 NO+SCNE
    check_take "SAVE + RELOAD PATTERN keeps them" src=random seed=3 lane1.pct=100 orig="$D/kept2.txt"
}
