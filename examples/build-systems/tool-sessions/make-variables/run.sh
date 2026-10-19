#!/usr/bin/env bash
# Make variables session: where CC and CFLAGS really come from.
# shellcheck source=../../../c/tool-sessions/lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../../../c/tool-sessions/lib.sh"

# Start from a clean environment so the host shell cannot leak CC or CFLAGS.
unset CC CFLAGS CROSS_COMPILE MAKEFLAGS

section "CC is a built-in variable, so ?= never assigns it"
run "make --no-print-directory -f conditional.mk"
expect "CC=cc  origin=default"
run "make --no-print-directory -p -f /dev/null 2>/dev/null | grep -E '^(CC|CXX|AR|CFLAGS|LDFLAGS|LDLIBS) ?='"
expect "^CC = cc"

section "Command line beats every ordinary assignment in the makefile"
run "make --no-print-directory -f conditional.mk CC=clang"
expect "CC=clang  origin=command line"
run "make --no-print-directory -f cross.mk CC=clang"
expect "CC=clang  origin=command line"

section "An exported environment CC is silently replaced by := or ="
run "CC='aarch64-poky-linux-gcc --sysroot=/sdk/sysroot' make --no-print-directory -f cross.mk"
expect "CC=gcc  origin=file"
run "CC='aarch64-poky-linux-gcc --sysroot=/sdk/sysroot' make --no-print-directory -e -f cross.mk"
expect "origin=environment override"

section "Guard on origin: respect every caller, still default sensibly"
run "make --no-print-directory -f respect.mk"
expect "CC=gcc  origin=file"
run "make --no-print-directory -f respect.mk CROSS_COMPILE=aarch64-linux-gnu-"
expect "CC=aarch64-linux-gnu-gcc"
run "CC='aarch64-poky-linux-gcc --sysroot=/sdk/sysroot' make --no-print-directory -f respect.mk"
expect "CC=aarch64-poky-linux-gcc --sysroot=/sdk/sysroot  origin=environment"
run "make --no-print-directory -f respect.mk CC=clang"
expect "CC=clang  origin=command line"

section "CFLAGS: environment values are appended to, command-line values are not"
run "make --no-print-directory -f flags.mk"
expect "CFLAGS=-O2 -Wall -Wextra  origin=file"
run "CFLAGS='-O0 -g' make --no-print-directory -f flags.mk"
expect "CFLAGS=-O0 -g -Wall -Wextra  origin=file"
run "make --no-print-directory -f flags.mk CFLAGS='-O0 -g'"
expect "CFLAGS=-O0 -g  origin=command line"
run "make --no-print-directory -f flags-override.mk CFLAGS='-O0 -g'"
expect "CFLAGS=-O0 -g -Wall -Wextra  origin=override"
