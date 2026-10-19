#!/usr/bin/env bash
# Case 08: strict aliasing - the optimizer assumes float* and uint32_t* differ.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Same source, two answers (-Wall -Wextra silent)"
compile bug-O0 -O0 bug.c
execute bug-O0
compile bug-O2 -O2 -Wall -Wextra bug.c
execute bug-O2

section "At -O2 the return value is the constant 1.0f, not a reload"
show_asm bug.c store_and_negate -O2

section "-fno-strict-aliasing restores the -O0 answer"
compile bug-nsa -O2 -fno-strict-aliasing bug.c
execute bug-nsa

section "Only the most aggressive warning level notices the cast"
compile bug-warn -O2 -Wstrict-aliasing=1 bug.c

section "Fixed: memcpy compiles to register moves, no library call"
show_asm fixed.c negate_bits -O2
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed
