#!/usr/bin/env bash
# Case 13: a misspelled handler leaves the weak default in the vector table.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Builds cleanly; the interrupt goes to the default handler"
compile bug -O2 -Wall -Wextra bug/vectors.c bug/uart.c bug/main.c
execute bug

section "The symbol table shows it: W at the default handler's address"
printf '$ nm bug | grep -E "handler|hander"\n'
nm "$build_dir/bug" | grep -E 'handler|hander'

section "-Wmissing-prototypes flags the stray definition"
compile bug-proto -O2 -Wmissing-prototypes bug/vectors.c bug/uart.c bug/main.c

section "Fixed: shared declarations + -Werror=missing-prototypes"
compile fixed -O2 -Wall -Wextra -Werror=missing-prototypes \
    fixed/vectors.c fixed/uart.c fixed/main.c
execute fixed

section "Post-link check: every required handler is implemented"
printf '$ check_handlers.sh fixed uart1_irq_handler\n'
"$case_dir/fixed/check_handlers.sh" "$build_dir/fixed" uart1_irq_handler && printf 'ok\n'
printf '$ check_handlers.sh bug uart1_irq_handler\n'
"$case_dir/fixed/check_handlers.sh" "$build_dir/bug" uart1_irq_handler || printf '[check failed, as intended]\n'
