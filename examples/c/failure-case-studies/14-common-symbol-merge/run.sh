#!/usr/bin/env bash
# Case 14: two unrelated globals merged into one common symbol.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "GCC 10+ default (-fno-common): the linker rejects the clash"
compile_expect_failure -O2 bug/uart.c bug/stats.c bug/main.c

section "With -fcommon (GCC <= 9 default; still added to keep old code linking)"
compile bug -O2 -Wall -Wextra -fcommon bug/uart.c bug/stats.c bug/main.c
execute bug
printf '$ nm -S bug | grep rx_buffer\n'
nm -S "$build_dir/bug" | grep rx_buffer

section "The linker can say so, if asked"
compile bug-warn -O2 -fcommon -Wl,--warn-common bug/uart.c bug/stats.c bug/main.c

section "Fixed: static module state, declarations in headers"
compile fixed -O2 -Wall -Wextra -fcommon \
    fixed/uart.c fixed/stats.c fixed/main.c
execute fixed
