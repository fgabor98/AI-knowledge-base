#!/usr/bin/env bash
# Linking session: archives, shared libraries, the loader, and symbol export.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Build objects and a static archive"
for unit in sensor_read sensor_version sensor_debug main; do
    run "$CC -c -O2 -g $unit.c -o $unit.o"
done
run "ar rcs libsensor.a sensor_read.o sensor_version.o sensor_debug.o"
run "ar t libsensor.a"

section "Archive order matters: the linker only pulls members for undefined symbols it already knows about"
run "$CC -L. -lsensor main.o -o app_static"
expect "undefined reference to .sensor_read'"
run "$CC main.o -L. -lsensor -Wl,-Map=app_static.map -o app_static"
run "./app_static"
expect "sensor 1.0: 420"
run "sed -n '1,6p' app_static.map"
expect "libsensor.a\(sensor_read.o\) +main.o \(sensor_read\)"

section "Unused code: archive members vs objects vs --gc-sections"
run "$CC main.o sensor_read.o sensor_version.o sensor_debug.o -o app_objects"
for unit in sensor_read sensor_version sensor_debug; do
    quiet "$CC -c -O2 -g -ffunction-sections -fdata-sections $unit.c -o gc_$unit.o"
done
run "$CC main.o gc_sensor_read.o gc_sensor_version.o gc_sensor_debug.o -Wl,--gc-sections -Wl,--print-gc-sections -o app_gc"
expect "removing unused section '.rodata.sensor_calibration_dump'"
run "size app_objects app_static app_gc"

section "A shared library with a SONAME"
run "$CC -O2 -g -fPIC -shared -Wl,-soname,libsensor.so.1 sensor_read.c sensor_version.c -o libsensor.so.1.0"
run "ln -sf libsensor.so.1.0 libsensor.so.1 && ln -sf libsensor.so.1 libsensor.so"
run "readelf -d libsensor.so.1.0 | grep SONAME"
expect "Library soname: \[libsensor.so.1\]"

section "The link succeeds; the program does not start"
run "$CC main.o -L. -lsensor -o app_dynamic"
run "./app_dynamic"
expect "cannot open shared object file"
run "readelf -d app_dynamic | grep -E 'NEEDED|RUNPATH'"

section "Tell the loader where to look: RUNPATH relative to the executable"
run "$CC main.o -L. -lsensor -Wl,-rpath,'\$ORIGIN' -o app_dynamic"
run "./app_dynamic"
expect "sensor 1.0: 420"
run "readelf -d app_dynamic | grep -E 'NEEDED|RUNPATH'"
expect "Library runpath: \[\\\$ORIGIN\]"
run "LD_DEBUG=bindings ./app_dynamic 2>&1 | grep -E \"symbol .sensor_(read|version)\" | sed -E 's/^[[:space:]]+[0-9]+:[[:space:]]+//'"
expect "normal symbol .sensor_read'"

section "Everything non-static is exported - and can be interposed"
run "nm -D --defined-only libsensor.so.1.0 | grep sensor"
expect "T sensor_scale"
run "$CC -O2 main_scale.c -L. -lsensor -Wl,-rpath,'\$ORIGIN' -o app_scale"
run "./app_scale"
expect "sensor 1.0: 42 "
run "objdump -d --no-show-raw-insn -M intel --disassemble=sensor_read libsensor.so.1.0 | awk '/>:\$/ { p = 1 } /^Disassembly/ { p = 0 } p && NF'"
expect "sensor_scale@plt"

section "Export only the API: -fvisibility=hidden plus SENSOR_API"
run "$CC -O2 -g -fPIC -shared -fvisibility=hidden -Wl,-soname,libsensor.so.1 sensor_read.c sensor_version.c -o libsensor.so.1.0"
run "nm -D --defined-only libsensor.so.1.0 | grep sensor"
expect "T sensor_version"
run "./app_scale"
expect "sensor 1.0: 420 "
