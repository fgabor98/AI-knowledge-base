#!/usr/bin/env bash
# Case 11: data race on a non-atomic flag; the load is hoisted out of the loop.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "-O0 terminates, -O2 spins forever"
compile bug-O0 -O0 -pthread bug.c
execute bug-O0
compile bug-O2 -O2 -Wall -Wextra -pthread bug.c
execute bug-O2

section "The flag is read once; the loop body is a jump to itself"
show_asm bug.c main -O2 -pthread

section "ThreadSanitizer (-O0 build) reports the flag and the payload"
compile bug-tsan -O0 -g -fsanitize=thread -pthread bug.c
printf '$ ./bug-tsan\n'
without_aslr timeout 5 "$build_dir/bug-tsan" 2>&1 |
    grep -E '^(WARNING|  (Write|Read|Previous)|SUMMARY)' || true

section "volatile terminates the loop but the payload is still racy"
compile bug-volatile -O2 -DVOLATILE_FLAG -pthread bug.c
execute bug-volatile
compile bug-volatile-tsan -O0 -g -fsanitize=thread -DVOLATILE_FLAG -pthread bug.c
printf '$ ./bug-volatile-tsan\n'
without_aslr timeout 5 "$build_dir/bug-volatile-tsan" 2>&1 |
    grep -E '^(WARNING|  (Write|Read|Previous)|SUMMARY)' || true

section "Fixed: release/acquire"
compile fixed -O2 -Wall -Wextra -pthread fixed.c
execute fixed
compile fixed-tsan -O1 -g -fsanitize=thread -pthread fixed.c
printf '$ ./fixed-tsan\n'
without_aslr timeout 5 "$build_dir/fixed-tsan" 2>&1
