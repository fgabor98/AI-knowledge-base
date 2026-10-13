#!/usr/bin/env bash
# Case 07: snprintf() return value used as an offset after truncation.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Without hardening (typical bare-metal newlib build): silent corruption"
compile bug-plain -O2 -Wall -Wextra -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fno-stack-protector bug.c
execute bug-plain

section "Distribution defaults (Ubuntu GCC: _FORTIFY_SOURCE=3, stack protector)"
compile bug-hardened -O2 bug.c
execute bug-hardened

section "AddressSanitizer"
compile bug-asan -O1 -g -fsanitize=address bug.c
execute bug-asan

section "Fixed"
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed
