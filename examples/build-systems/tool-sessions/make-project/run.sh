#!/usr/bin/env bash
# Make project session: incremental builds, generated dependencies, order-only
# prerequisites, .DELETE_ON_ERROR, parallel races, and recursive make.
# shellcheck source=../../../c/tool-sessions/lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../../../c/tool-sessions/lib.sh"

unset CC CFLAGS CPPFLAGS LDFLAGS LDLIBS CROSS_COMPILE MAKEFLAGS MFLAGS
M="make --no-print-directory"

section "First build"
run "$M"
expect "gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/main.c -o build/main.o"
run "./build/board-tool"
expect "board-tool 1.0: line 3 = 1"
run "$M"
expect "Nothing to be done for 'all'"

section "The compiler wrote the header dependencies"
run "cat build/main.d"
expect "^include/legacy.h:"

section "Touch one header: only its users rebuild, and --trace says why"
run "touch include/gpio.h && $M --trace"
expect "update target 'build/gpio.o' due to: include/gpio.h"

section "Delete a header and stop including it: -MP keeps the old .d file harmless"
run "rm include/legacy.h && sed -i -e '/legacy.h/d' -e 's/LEGACY_BANNER/\"board-tool\"/' src/main.c && $M"
expect "-c src/main.c -o build/main.o"

section "Order-only prerequisite: a new file in build/ does not rebuild objects"
run "touch build/notes.txt && $M --trace"
expect "Nothing to be done for 'all'"
run "sed 's/ | \\\$(BUILD)\$/ \\\$(BUILD)/' Makefile > no-order-only.mk && touch build/notes.txt && $M -n -f no-order-only.mk --trace | grep 'due to'"
expect "update target 'build/gpio.o' due to: build"

section "A changed variable is not a changed file - unless a stamp records it"
run "$M VERSION=2.1 --trace"
expect "update target 'build/version.o' due to: build/version.stamp"
expect "-DTOOL_VERSION='\"2.1\"'"
run "./build/board-tool && $M VERSION=2.1"
expect "Nothing to be done for 'all'"

section "The print-VAR helper"
run "$M print-CC print-CFLAGS print-KB_CFLAGS CFLAGS=-Os"
expect "CFLAGS=-Os  \[origin: command line\]"
run "CC=clang $M print-CC"
expect "CC=clang  \[origin: environment\]"

section "Dry run a cross build without a cross compiler installed"
run "$M -n --always-make CROSS_COMPILE=aarch64-linux-gnu- | head -n 3"
expect "^aarch64-linux-gnu-gcc -Iinclude"

section "Stage the install with DESTDIR"
run "$M install DESTDIR=\$PWD/stage && find stage -type f"
expect "TOOL_VERSION='\"1.0\"'"
expect "stage/usr/bin/board-tool"

section "Without .DELETE_ON_ERROR a failed recipe leaves a truncated target behind"
run "$M -f delete-on-error.mk"
expect "Error 1"
run "ls table.h && $M -f delete-on-error.mk"
expect "'table.h' is up to date"
run "rm -f table.h && $M -f delete-on-error.mk --eval .DELETE_ON_ERROR:"
expect "Deleting file 'table.h'"

section "A missing prerequisite works serially and breaks with -j"
run "$M -f race.mk"
expect "cc app.o -o app"
run "rm -f app app.o config.h && $M -f race.mk -j4"
expect "config.h: No such file or directory"
run "rm -f app app.o config.h && $M -f race-fixed.mk -j4"
expect "cc app.o -o app"

section "Recursive make: \$(MAKE) forwards flags and command-line variables"
run "$M -f recursive.mk -j4 CC=clang"
expect "lib: CC=clang \[command line\]  MAKEFLAGS=.*j4"
expect "jobserver unavailable: using -j1"
run "$M -f recursive.mk -n | head -n 4"
expect "lib: CC=cc"
