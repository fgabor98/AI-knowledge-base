#!/usr/bin/env bash
# Case 09: use-after-free through a pointer into a realloc()ed array.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Looks correct: the result depends on whether realloc() moved the block"
compile bug -O2 -Wall -Wextra bug.c
execute bug

section "GCC static analyzer"
compile bug-analyzer -O2 -fanalyzer bug.c

section "AddressSanitizer"
compile bug-asan -O1 -g -fsanitize=address bug.c
execute bug-asan

section "Fixed"
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed
compile fixed-asan -O1 -g -fsanitize=address fixed.c
execute fixed-asan
