#!/usr/bin/env bash
# Case 03: out-of-bounds read lets the optimizer conclude "always found".
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Same source, two answers (the table holds only zeros)"
compile bug-O0 -O0 bug.c
execute bug-O0
compile bug-O2 -O2 -Wall -Wextra -Warray-bounds=2 bug.c
execute bug-O2

section "The whole function at -O2"
show_asm bug.c exists_in_table -O2

section "Sanitizers report the read - and the symptom disappears"
compile bug-ubsan -O2 -g -fsanitize=undefined bug.c
execute bug-ubsan
compile bug-asan -O2 -g -fsanitize=address bug.c
execute bug-asan

section "Fixed"
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed
