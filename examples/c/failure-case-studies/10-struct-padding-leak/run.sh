#!/usr/bin/env bash
# Case 10: struct padding carries stale stack bytes onto the wire.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Padding bytes carry the previous function's data (A..Z pattern)"
compile bug-O0 -O0 bug.c
execute bug-O0
compile bug-O2 -O2 -Wall -Wextra bug.c
execute bug-O2

section "-Wpadded shows where the holes are"
compile bug-padded -O2 -Wpadded bug.c

section "Zero-initializing automatic variables (GCC 12+) removes the leak as a hardening backstop"
compile bug-zero-init -O2 -ftrivial-auto-var-init=zero bug.c
execute bug-zero-init

section "Fixed: explicit encoding, 7 bytes, no padding"
compile fixed -O2 -Wall -Wextra -Wconversion fixed.c
execute fixed
