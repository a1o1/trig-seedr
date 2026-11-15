#!/bin/sh
# Task F1: Barlow ranks and meters on the device. METR 6/8 and BEAT 2 (TAKE), BIAS with ROLE = HAT
# (RHYTHM), SHAPE = RANK and AMOD = TOP (DYNAMICS), each take against tools/seedref; NO; then a
# 7/8 KICK recipe written into the store.
R=$(cd "$(dirname "$0")/../.." && pwd); . "$R/tools/emu.sh"; . "$T/tests/fixtures.sh"
emu_init 21-rank
emu_build
emu_fixture kept2 fix_kept2
save_orig kept2
K="orig=$D/kept2.txt"
emu_open
emu_knob TAKE E:metr:one 2
emu_knob TAKE F:beat 2
emu_shot take
emu_page RHYTHM
emu_knob RHYTHM D:bias 80
emu_knob RHYTHM E:role:one 3
emu_yes
emu_shot rhythm
check_take "6/8, BEAT 2, BIAS 80, ROLE HAT" src=random seed=2 metr=6/8 beat=2 bias=80 role=hat "$K"
emu_page DYNAMICS
emu_knob DYNAMICS B:shape:one 6
emu_knob DYNAMICS C:amt 100
emu_knob DYNAMICS E:acc 30
emu_knob DYNAMICS F:amod:one 1
emu_yes
emu_shot dynamics
check_take "SHAPE RANK 100 %, ACC 30 % TOP" src=random seed=2 metr=6/8 beat=2 bias=80 role=hat \
    shape=rank amt=100 acc=30 amod=top "$K"
emu_no
check_take "NO: seed 2" src=random seed=2 "$K"
emu_boot
emu_recipe 0 src=random metr=7/8 beat=2 dens=30 bias=70 role=kick
emu_open
emu_yes
check_take "7/8 KICK, injected" src=random metr=7/8 beat=2 dens=30 bias=70 role=kick
emu_done
