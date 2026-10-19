#!/usr/bin/env bash
# Case 01: NULL check deleted because it follows a dereference.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Buggy function at -O2: the NULL test is gone"
show_asm bug.c device_flags -O2

section "Same source with -fno-delete-null-pointer-checks: the test is back"
show_asm bug.c device_flags -O2 -fno-delete-null-pointer-checks

section "Warnings: silent on the function alone (library-style code)"
compile bug.o -c -O2 -Wall -Wextra -fanalyzer -DNO_MAIN bug.c

section "-fanalyzer reports it once a NULL caller is visible in the same file"
compile bug -O2 -fanalyzer bug.c

section "UndefinedBehaviorSanitizer catches the dereference at run time"
compile bug-ubsan -O2 -g -fsanitize=undefined bug.c
execute bug-ubsan null

section "Fixed: check first"
show_asm fixed.c device_flags -O2
compile fixed -O2 -Wall -Wextra fixed.c
execute fixed
execute fixed null
