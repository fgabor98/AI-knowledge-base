#!/usr/bin/env bash
# Sanitizer session: reading ASan, LSan, UBSan, and TSan reports.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Without instrumentation the off-by-one goes unnoticed"
run "$CC -O0 -g frame_copy.c -o frame_copy"
run "./frame_copy"
expect "STATUS=OK"

section "AddressSanitizer: heap-buffer-overflow"
run "$CC -O0 -g -fsanitize=address frame_copy.c -o frame_copy_asan"
run "./frame_copy_asan"
expect "ERROR: AddressSanitizer: heap-buffer-overflow"
expect "0 bytes after 9-byte region"
expect "=>.*00\[01\]"

section "LeakSanitizer (part of ASan on Linux): direct and indirect leaks"
run "$CC -O0 -g -fsanitize=address leaks.c -o leaks"
run "./leaks"
expect "Direct leak of 16 byte"
expect "Indirect leak of 256 byte"
expect "exit status 1"

section "Suppress a known third-party leak, and audit the suppression"
run "cat lsan.supp"
run "LSAN_OPTIONS=suppressions=lsan.supp:print_suppressions=1 ./leaks"
expect "Suppressions used:"
expect "64 vendor_sdk_init"

section "UndefinedBehaviorSanitizer: reports, continues, exits 0 by default"
run "$CC -O0 -g -fsanitize=undefined checks.c -o checks"
run "./checks"
expect "signed integer overflow"
expect "shift exponent 32"
run "UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ./checks"
expect "exit status 1"
run "$CC -O0 -g -fsanitize=undefined -fno-sanitize-recover=undefined checks.c -o checks_strict"
run "./checks_strict"
expect "exit status 1"

section "ThreadSanitizer: a lock-order inversion that never deadlocked"
run "$CC -O1 -g -fsanitize=thread lock_order.c -o lock_order"
run "$(without_aslr) ./lock_order"
expect "lock-order-inversion \(potential deadlock\)"
expect "exit status 66"

section "ASan and TSan cannot share a build"
run "$CC -fsanitize=address,thread lock_order.c -o lock_order_both"
expect "incompatible"
