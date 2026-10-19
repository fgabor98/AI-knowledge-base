#!/usr/bin/env bash
# Linker-script session: link the docs page's script and inspect the result.
#
# The image targets x86-64 (the host compiler), so it is inspected, not run;
# the linker behavior shown is the same for a microcontroller toolchain.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

cflags="-O2 -ffreestanding -fno-pic -fno-asynchronous-unwind-tables -fno-stack-protector"
ldflags="-nostdlib -static -no-pie -T firmware.ld"

section "Compile freestanding objects and link with the script"
run "$CC $cflags -c startup.c -o startup.o"
run "$CC $cflags -c app.c -o app.o"
run "$CC $ldflags -Wl,-Map=firmware.map -Wl,--print-memory-usage startup.o app.o -o firmware.elf"
expect "FLASH: +[0-9]+ B +256 KB"
expect "RAM: +[0-9]+ B +64 KB"

section "Load address vs run address"
run "objdump -h firmware.elf | grep -A1 -E ' \\.(text|data|bss) '"
expect "\\.data +0+4 +0+20000000 +0+800010c"
run "grep -A1 -E '^\\.data ' firmware.map"
expect "load address 0x000000000800010c"
run "readelf -lW firmware.elf | grep -E 'Type|LOAD'"

section "Linker-defined symbols seen by startup code"
run "nm -n firmware.elf | grep -E '__|boot_count|sample_buffer|firmware_name'"
expect "A __data_load_start__"

section "Sections the script does not mention still end up somewhere"
run "$CC $ldflags -Wl,--orphan-handling=warn startup.o app.o -o firmware_orphans.elf 2>&1 | head -4"
expect "orphan section .\\.note\\.gnu\\.build-id'"

section "Grow .bss past the RAM region"
run "$CC $cflags -DSAMPLE_BUFFER_BYTES=70000 -c app.c -o app_big.o"
run "$CC $ldflags -Wl,--print-memory-usage startup.o app_big.o -o firmware_big.elf"
expect "will not fit in region .RAM'"
expect "^/usr/bin/ld: RAM overflow"
expect "region .RAM' overflowed by 4496 bytes"
