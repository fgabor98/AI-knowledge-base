#!/usr/bin/env bash
# Case 05: uint16_t * uint16_t is a signed int multiplication.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Same source, two answers"
compile bug-O0 -O0 bug.c
execute bug-O0
compile bug-O2 -O2 -Wall -Wextra -Wconversion bug.c
execute bug-O2

section "The whole function at -O2"
show_asm bug.c product_is_large -O2

section "UndefinedBehaviorSanitizer"
compile bug-ubsan -O2 -g -fsanitize=undefined bug.c
execute bug-ubsan

section "Fixed"
show_asm fixed.c product_is_large -O2
compile fixed -O2 -Wall -Wextra -Wconversion fixed.c
execute fixed
