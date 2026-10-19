#!/usr/bin/env bash
# Case 02: signed-overflow check folded away by the optimizer.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "The check never fires - not even at -O0"
compile bug-O0 -O0 bug.c
execute bug-O0
compile bug-O2 -O2 -Wall -Wextra bug.c
execute bug-O2

section "What GCC made of 'a + b < a' (identical at -O0: folded in the front end)"
show_asm bug.c add_overflows -O2
show_asm bug.c add_overflows -O0

section "-fwrapv defines wraparound and restores the -O0 answer"
compile bug-wrapv -O2 -fwrapv bug.c
execute bug-wrapv

section "UndefinedBehaviorSanitizer"
compile bug-ubsan -O2 -g -fsanitize=undefined bug.c
execute bug-ubsan

section "Fixed"
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed
