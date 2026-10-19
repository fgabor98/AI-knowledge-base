#!/usr/bin/env bash
# Shared libraries session: library names, symbol versions, search order,
# --as-needed, -z defs, and PIE.
# shellcheck source=../../../c/tool-sessions/lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../../../c/tool-sessions/lib.sh"

unset LD_LIBRARY_PATH

section "Three names for one library"
run "$CC -O2 -fPIC -shared -Wl,-soname,libsensor.so.1 -Wl,--version-script=libsensor-1.0.map sensor_v1.c -o libsensor.so.1.0.0"
run "readelf -d libsensor.so.1.0.0 | grep SONAME"
expect "Library soname: \[libsensor.so.1\]"
run "$CC app_v1.c -L. -lsensor -o app_v1"
expect "cannot find -lsensor"
run "ldconfig -n . && ln -sf libsensor.so.1 libsensor.so && ls -l libsensor.so* | awk '{print \$9, \$10, \$11}'"
expect "^libsensor.so.1 -> libsensor.so.1.0.0"
run "$CC app_v1.c -L. -lsensor -Wl,-rpath,'\$ORIGIN' -o app_v1 && readelf -d app_v1 | grep NEEDED"
expect "Shared library: \[libsensor.so.1\]"

section "Symbol versions: a newer library adds a function"
run "$CC -O2 -fPIC -shared -Wl,-soname,libsensor.so.1 -Wl,--version-script=libsensor-1.1.map sensor_v2.c -o libsensor.so.1.1.0"
run "ln -sf libsensor.so.1.1.0 libsensor.so.1 && $CC app.c -L. -lsensor -Wl,-rpath,'\$ORIGIN' -o app && ./app"
expect "read 420, average 421"
run "objdump -T app | grep sensor"
expect "\(LIBSENSOR_1.1\) sensor_read_average"
run "readelf -V app | sed -n '/.gnu.version_r/,\$p' | grep -E 'File|Name'"
expect "Name: LIBSENSOR_1.1"

section "Deploy the old library: the loader refuses to start the program"
run "ln -sf libsensor.so.1.0.0 libsensor.so.1 && ./app"
expect "version .LIBSENSOR_1.1' not found \(required by ./app\)"
run "./app_v1"
expect "^read 420"

section "The same check against glibc: the newest GLIBC_ version a binary needs"
run "objdump -T app | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -n 1"
expect "^GLIBC_2\."

section "Search order: DT_RPATH beats LD_LIBRARY_PATH, DT_RUNPATH does not"
run "mkdir -p v1 v2 && $CC -fPIC -shared -DWHICH='\"v1\"' which.c -o v1/libwhich.so && $CC -fPIC -shared -DWHICH='\"v2\"' which.c -o v2/libwhich.so"
run "$CC which_app.c -Lv1 -lwhich -Wl,-rpath,'\$ORIGIN/v1' -o app_runpath && readelf -d app_runpath | grep -E 'RPATH|RUNPATH'"
expect "Library runpath"
run "$CC which_app.c -Lv1 -lwhich -Wl,-rpath,'\$ORIGIN/v1' -Wl,--disable-new-dtags -o app_rpath && readelf -d app_rpath | grep -E 'RPATH|RUNPATH'"
expect "Library rpath"
run "./app_runpath && LD_LIBRARY_PATH=\$PWD/v2 ./app_runpath"
expect "loaded libwhich from v2"
run "LD_LIBRARY_PATH=\$PWD/v2 ./app_rpath"
expect "loaded libwhich from v1"
run "LD_DEBUG=libs LD_LIBRARY_PATH=\$PWD/v2 ./app_rpath 2>&1 | grep -E 'find library=libwhich|search path=|trying file=.*libwhich' | sed -E 's/^[[:space:]]+[0-9]+:[[:space:]]+//' | head -n 4"
expect "search path=./v1"

section "--as-needed: unused libraries are not recorded"
run "$CC plain.c -lm -o plain_default && readelf -d plain_default | grep NEEDED"
run "$CC plain.c -Wl,--no-as-needed -lm -o plain_all && readelf -d plain_all | grep NEEDED"
expect "Shared library: \[libm.so.6\]"

section "Undefined symbols in a shared library: allowed by default, an error with -z defs"
run "$CC -fPIC -c needs_log.c -o needs_log.o && $CC -shared needs_log.o -o libscale.so && echo linked"
expect "^linked"
run "$CC -shared -Wl,-z,defs needs_log.o -o libscale.so"
expect "undefined reference to .log'"
run "$CC -shared -Wl,-z,defs needs_log.o -lm -o libscale.so && readelf -d libscale.so | grep NEEDED"
expect "libm.so.6"

section "Executables are position independent by default"
run "file app | cut -d, -f1-2 && readelf -h app | grep Type"
expect "DYN \(Position-Independent Executable file\)"
run "$CC -no-pie plain.c -o plain_nopie && readelf -h plain_nopie | grep Type"
expect "EXEC \(Executable file\)"
