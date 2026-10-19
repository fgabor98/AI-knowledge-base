#!/usr/bin/env bash
# Case 04: shift count equal to the type width.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "One binary, two answers for the same call"
compile bug-O0 -O0 bug.c
execute bug-O0 32
compile bug-O2 -O2 -Wall -Wextra bug.c
execute bug-O2 32

section "The runtime path: x86 SHL masks the count to 5 bits"
show_asm bug.c low_bits_mask -O2

section "Detection"
compile bug-analyzer -O2 -fanalyzer bug.c
compile bug-ubsan -O2 -g -fsanitize=undefined bug.c
execute bug-ubsan 32

section "Fixed"
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed 32
execute fixed 5
