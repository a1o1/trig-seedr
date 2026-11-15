#!/bin/sh
# tools/golden.sh: the golden vectors (plan section 7, gate 3; design V11). Each line of
# lib/golden/recipes.txt is NAME KEY=VALUE... (tools/seedref's keys; orig=FILE is relative to
# lib/golden); its vector is lib/golden/NAME.txt, seedref's output with all=1.
#     sh tools/golden.sh                 check: every frozen vector still comes out the same
#     sh tools/golden.sh --write [NAME]  write the vectors (all, or those named); the lead freezes
#                                        them at gate G0, and after that a change needs a reason in
#                                        the workstream's status file
# A recipe whose layer is not built yet (seedref: result -1) is pending: not written, not a
# failure. A recipe with no vector is reported as not frozen. Exit status: 0 when nothing differs.
R=$(cd "$(dirname "$0")/.." && pwd); . "$R/tools/emu.sh"
G=$R/lib/golden
write=; [ "$1" = --write ] && { write=1; shift; }
mkdir -p "$W"
same=0 differ=0 pending=0 unfrozen=0 written=0
while read -r name args; do
    case $name in ''|\#*) continue ;; esac
    if [ $# -gt 0 ]; then
        hit=; for a in "$@"; do [ "$a" = "$name" ] && hit=1; done
        [ -n "$hit" ] || continue
    fi
    # shellcheck disable=SC2086
    got=$(cd "$G" && seedref all=1 $args 2> "$W/golden.err"); rc=$?
    if [ $rc -eq 3 ]; then
        pending=$((pending + 1))
        [ -f "$G/$name.txt" ] && { echo "FAIL golden $name: frozen, but seedref now says $(cat "$W/golden.err")"; differ=$((differ + 1)); }
        continue
    fi
    [ $rc -eq 0 ] || { echo "FAIL golden $name: seedref: $(cat "$W/golden.err")"; differ=$((differ + 1)); continue; }
    if [ -n "$write" ]; then
        echo "$got" > "$G/$name.txt"; written=$((written + 1))
    elif [ ! -f "$G/$name.txt" ]; then
        unfrozen=$((unfrozen + 1))
    elif [ "$got" = "$(cat "$G/$name.txt")" ]; then
        same=$((same + 1))
    else
        differ=$((differ + 1)); echo "FAIL golden $name"
        echo "$got" | diff "$G/$name.txt" - | sed 's/^/  /'
    fi
done < "$G/recipes.txt"
if [ -n "$write" ]; then echo "golden: $written written, $pending pending (layer not built)"
else echo "golden: $same same, $differ differ, $unfrozen not frozen, $pending pending (layer not built)"
fi
[ $differ -eq 0 ]
