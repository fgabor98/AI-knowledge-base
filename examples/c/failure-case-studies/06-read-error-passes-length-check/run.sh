#!/usr/bin/env bash
# Case 06: ssize_t error value compared against sizeof.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "-Wall is silent; the error return is accepted as a huge frame"
compile bug -O2 -Wall bug.c
execute bug

section "-Wextra enables -Wsign-compare for C"
compile bug-wextra -O2 -Wall -Wextra bug.c

section "Fixed"
compile fixed -O2 -Wall -Wextra -Wconversion -Wsign-conversion fixed.c
execute fixed
