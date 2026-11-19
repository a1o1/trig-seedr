#!/bin/sh
# Task S0.5 (docs/plan-syntakt.md): the Syntakt's lockable parameters, device/st_params.h.
#   1. device/st_params.h is what tools/st_params.py makes from the stock OS 1.42 file
#   2. screenshots in analog-emu of what the header's track facts were read from (stock 1.42, the
#      default project): each kind of track's machine menu (FUNC + SYN, scrolled), the SYN, FLTR, AMP
#      and LFO pages on tracks 1 (digital), 9 (analog drum) and the FX track, and the FX track's Drive page
# Screens and the regenerated header stay in build/emu/st-s05/.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu_st.sh"
emu_init st-s05

# 1. the header, regenerated (synfact-dev's elekloader reads the container)
EL=${SYNFACT:-$E/external/synfact-dev}
python3 -I "$T/st_params.py" "$STOCK" "$EL" > "$D/st_params.h" || emu_die "tools/st_params.py"
emu_check "device/st_params.h is tools/st_params.py's output" "$(cat "$R/device/st_params.h")" "$(cat "$D/st_params.h")"

# 2. screens
menu() {                    # menu NAME: FUNC + SYN, 15 screens three steps apart, from the top
    emu_keys F+SYN
    _m_i=1; while [ $_m_i -le 9 ]; do emu_keys UP UP UP; _m_i=$((_m_i + 1)); done
    emu_shot "$1-menu-00"
    _m_i=1; while [ $_m_i -le 8 ]; do emu_keys DOWN DOWN DOWN; emu_shot "$1-menu-$(printf %02d $_m_i)"; _m_i=$((_m_i + 1)); done
    emu_keys NO
}
pages() {                   # pages NAME: SYN, then FLTR, AMP and LFO, each key twice
    for _p_k in SYN FLTR AMP LFO; do
        emu_keys $_p_k; emu_shot "$1-$_p_k-1"; emu_keys $_p_k; emu_shot "$1-$_p_k-2"
    done
}
emu_boot
pages t1
menu t1
emu_keys TRK+T9
pages t9
menu t9
emu_keys TRK+T12
menu t12
emu_keys FX
pages fx
echo "screens: $D/*.png"
emu_done
