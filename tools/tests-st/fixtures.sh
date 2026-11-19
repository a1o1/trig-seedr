# tools/tests-st/fixtures.sh: the Syntakt test files' shared states and checks (sourced after st.sh). The
# Syntakt's preset project has trigs on every voice track of A01, so the tests work on track 4 (EMU_TRACK 3,
# 5 steps, trigs on steps 2 and 4) with its two trigs turned off in grid recording: an empty track.
EMU_TRACK=3

# check_take LABEL [SEEDREF-ARG...]: the track in scope is what seedref --device st renders for those settings
check_take() {
    _ct_l=$1; shift
    emu_check "$_ct_l" "$(seedref track=$EMU_TRACK "$@")" "$(emu_dump $EMU_TRACK)"
}

# save_orig NAME: the track in scope as it is now, as $D/NAME.txt, for seedref's orig=
save_orig() { emu_dump $EMU_TRACK > "$D/$1.txt"; }

# The Rytm's kept2 on the Syntakt: boot; track 4 selected and emptied with stock's keys; seedr opened (the
# empty track's new recipe is RAND, V6); YES seed 1; seed 2 from the original; NO; FUNC + YES; NO closes.
# Ends with seed 2 kept on track 4 and seedr closed; $D/empty.txt is the original.
# track 4 selected and emptied with stock's keys, seedr not opened yet; $D/empty.txt is the track
fix_empty() {
    emu_boot
    emu_rec 0
    emu_keys TRK+T4                     # TRK + a trig key selects a track with grid recording off
    emu_rec 1
    emu_keys T2 T4                      # its two trigs off
    emu_rec 0
    emu_check "track 4 emptied with stock's keys" 0 "$(emu_dump $EMU_TRACK | grep '^trig' | grep -c x)"
    save_orig empty
}

fix_kept2() {
    fix_empty
    emu_open
    emu_check "open: the empty track's new recipe is RAND (V6)" 1 "$(emu_get "recipe[$EMU_TRACK].src")"
    emu_yes
    emu_shot take1
    check_take "YES: seed 1" src=random seed=1 orig="$D/empty.txt"
    emu_knob TAKE A:seed 2
    emu_yes
    check_take "SEED 2, YES: from the original" src=random seed=2 orig="$D/empty.txt"
    emu_no
    check_take "NO: back to the original" orig="$D/empty.txt"
    emu_knob TAKE A:seed 1
    emu_knob TAKE A:seed 2
    emu_yes
    emu_keep
    check_take "YES, FUNC + YES: kept" src=random seed=2 orig="$D/empty.txt"
    emu_no
    check_take "NO: closed, the take kept" src=random seed=2 orig="$D/empty.txt"
    emu_check "NO: seedr is closed" 0 "$(emu_get open)"
}
