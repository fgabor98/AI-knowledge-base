#!/usr/bin/env bash
# Compiler driver session: the stages behind one gcc command.
# shellcheck source=../../../c/tool-sessions/lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../../../c/tool-sessions/lib.sh"

section "Which target does this driver build for?"
run "$CC -dumpmachine"
expect "x86_64-linux-gnu"

section "One command, three programs: -### prints them without running anything"
run "$CC -### -Iinclude main.c -o app 2>&1 | grep -E '^ ' | awk '{print \$1}' | sed 's|.*/||'"
expect "^cc1$"
expect "^as$"
expect "^collect2$"

section "Keep every intermediate file"
run "$CC -save-temps -Iinclude -O2 main.c -o app && ls app*"
expect "app-main.i"
run "grep -c '' app-main.i && grep -n 'LEDs' app-main.i"
expect "printf\(\"%d LEDs\\\\n\", 4\);"
run "grep -E 'call|printf|puts' app-main.s"
expect "puts|printf"
run "file app-main.o app | sed 's/, BuildID.*//'"
expect "app-main.o: ELF 64-bit LSB relocatable"

section "Where #include looks, in order"
run "$CC -v -Iinclude -E main.c -o /dev/null 2>&1 | sed -n '/#include \"...\" search starts here:/,/End of search list./p'"
expect "^ include$"

section "What the driver adds at link time"
run "$CC -c -Iinclude main.c && $CC -v main.o -o app 2>&1 | grep collect2 | tr ' ' '\n' | grep -E 'crt|^-l|^-dynamic-linker|ld-linux' | sed 's|.*/||'"
expect "^Scrt1.o$"
expect "^-lc$"
run "$CC -print-file-name=Scrt1.o && $CC -print-prog-name=cc1 | sed 's|.*/gcc/|.../gcc/|'"
expect "Scrt1.o"

section "Each stage fails differently"
run "printf '#include \"missing.h\"\n' > pp.c && $CC -c pp.c"
expect "fatal error: missing.h: No such file or directory"
run "printf 'int f(void) { return undeclared; }\n' > cc.c && $CC -c cc.c"
expect "error: .undeclared. undeclared"
run "printf 'int helper(void);\nint main(void) { return helper(); }\n' > ld.c && $CC -c ld.c && $CC ld.o -o ld_app"
expect "undefined reference to .helper'"

section "What is inside the executable: sections grouped into loadable segments"
run "$CC -O2 -Iinclude main.c -o app && size app"
expect "filename"
run "readelf -lW app | grep -E '^ +(Type|INTERP|LOAD|DYNAMIC|GNU_RELRO)'"
expect "^ +INTERP"
run "readelf -lW app | sed -n '/Section to Segment mapping/,\$p'"
expect "\.text"
run "readelf -SW app | awk '\$2 ~ /^\\.(text|rodata|data|bss|dynamic|got|plt|interp|debug)/ {print \$2, \$3}' | column -t"
expect "^.bss +NOBITS"
