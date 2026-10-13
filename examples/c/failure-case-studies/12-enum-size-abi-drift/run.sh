#!/usr/bin/env bash
# Case 12: two objects compiled with different enum sizes share one struct.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Library built with defaults, application with -fshort-enums"
compile config.o -c -O2 -Wall -Wextra bug/config.c
compile main.o -c -O2 -Wall -Wextra -fshort-enums bug/main.c
compile drift "$build_dir/main.o" "$build_dir/config.o"
execute drift

section "Same mismatch without the stack protector: silent wrong values"
compile main-nsp.o -c -O2 -fshort-enums -fno-stack-protector bug/main.c
compile drift-nsp "$build_dir/main-nsp.o" "$build_dir/config.o"
execute drift-nsp

section "LTO with -Wlto-type-mismatch: no diagnostic"
compile config-lto.o -c -O2 -flto bug/config.c
compile main-lto.o -c -O2 -flto -fshort-enums bug/main.c
compile drift-lto -O2 -flto -Wlto-type-mismatch "$build_dir/main-lto.o" "$build_dir/config-lto.o"
execute drift-lto

section "Fixed: same flag mismatch, identical layout"
compile config-fixed.o -c -O2 -Wall -Wextra fixed/config.c
compile main-fixed.o -c -O2 -Wall -Wextra -fshort-enums fixed/main.c
compile fixed "$build_dir/main-fixed.o" "$build_dir/config-fixed.o"
execute fixed

section "A layout assertion in the original header would have stopped the build"
compile layout-assert.o -c -O2 bug/layout_assert.c
compile_expect_failure -c -O2 -fshort-enums bug/layout_assert.c
