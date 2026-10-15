#!/usr/bin/env bash
# Binary inspection session: from a field crash record back to source lines.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Build with debug info, per-function sections, and a map file"
for unit in main frame crash_record; do
    run "$CC -c -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections $unit.c -o $unit.o"
done
run "$CC main.o frame.o crash_record.o -Wl,-Map=app.map -o app"
run "./app ok"
expect "accepted: 1"

section "What kind of file is it?"
run "readelf -h app | grep -E 'Class|Type|Machine|Entry'"
expect "DYN \(Position-Independent Executable file\)"
run "readelf -n app | grep 'Build ID'"
expect "Build ID: [0-9a-f]{40}"

section "Sections, sizes, and the largest symbols"
run "readelf -SW app | grep -E '\\] \\.(text|rodata|data|bss|debug_info|debug_line|symtab)'"
run "size app"
run "nm -S --size-sort app | tail -5"

section "Where did each function come from? The map file"
run "grep -A1 -E '^ \\.text\\.(frame_accept|on_fatal_signal|crash_record_install)' app.map"
expect "frame.o"

section "Disassemble one function - and its cold part"
run "objdump -d --no-show-raw-insn -M intel --disassemble=frame_accept app | awk '/>:\$/ { p = 1 } /^Disassembly/ { p = 0 } p && NF'"
expect "je +[0-9a-f]+ <frame_accept.cold>"
run "objdump -d --no-show-raw-insn -M intel --disassemble=frame_accept.cold app | awk '/>:\$/ { p = 1 } /^Disassembly/ { p = 0 } p && NF'"
expect "ud2"

section "Struct layout, holes included"
run "gdb -q -nx -batch -iex 'set debuginfod enabled off' -ex 'ptype /o struct frame_header' app"
expect "3-byte hole"

section "Ship a stripped binary; keep the debug info separately"
run "objcopy --only-keep-debug app app.debug"
run "objcopy --strip-debug --strip-unneeded --add-gnu-debuglink=app.debug app app.deployed"
run "ls -l app app.debug app.deployed | awk '{print \$5, \$9}'"
run "readelf -n app.deployed | grep 'Build ID'; readelf -n app.debug 2>/dev/null | grep 'Build ID'"

section "A crash in the field: the handler prints build ID and module offset"
run "./app.deployed"
expect "crash: signal 0x4 build-id [0-9a-f]{40} module-offset 0x[0-9a-f]+"
offset=$(grep -oE 'module-offset 0x[0-9a-f]+' "$last_output" | cut -d' ' -f2)

section "Symbolize offline - without, then with, the matching debug file"
run "mkdir -p elsewhere && mv app.debug elsewhere/"
run "addr2line -f -i -p -e app.deployed $offset"
expect "\?\?"
run "mv elsewhere/app.debug . && addr2line -f -i -p -e app.deployed $offset"
expect "frame_checksum at .*frame.c:13"
expect "inlined by\) frame_accept"

section "The build ID covers the debug info - including the build directory"
quiet "mkdir -p tree_a tree_b && cp main.c frame.c frame.h crash_record.c tree_a/ && cp main.c frame.c frame.h crash_record.c tree_b/"
run "for tree in tree_a tree_b; do (cd \$tree && $CC -O2 -g main.c frame.c crash_record.c -o app && readelf -n app | grep 'Build ID'); done"
run "for tree in tree_a tree_b; do (cd \$tree && $CC -O2 -g -ffile-prefix-map=\$PWD=. main.c frame.c crash_record.c -o app && readelf -n app | grep 'Build ID'); done"
expect_identical_build_ids() {
    local ids
    ids=$(grep -o 'Build ID: [0-9a-f]*' "$last_output" | sort -u | wc -l)
    if ((ids != 1)); then
        printf '\nsession check failed: expected identical build IDs\n' >&2
        exit 1
    fi
}
expect_identical_build_ids
