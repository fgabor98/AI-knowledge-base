#!/usr/bin/env bash
# GDB session: from a crash to the write that caused it.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

gdb_batch="gdb -q -nx -batch -iex 'set debuginfod enabled off'"

section "Build for debugging and reproduce"
run "$CC -Og -g3 -Wall -Wextra sensor_log.c -o sensor_log"
run "./sensor_log"
expect "Segmentation fault|exit status 139"
run "./sensor_log adc0"
expect "adc0: 4 samples"

section "Where it crashed, and with what state"
run "$gdb_batch -ex run -ex bt -ex 'p ring->head' -ex 'p/x ring->head' -ex 'x/s (char *)&ring->head' ./sensor_log"
expect "SIGSEGV"
expect "in ring_push"
expect '"ure-sensor-0"'

section "A watchpoint on the corrupted field finds the writer"
run "$gdb_batch -ex 'break main' -ex run -ex 'watch -l dev.ring.head' -ex continue -ex bt -ex 'p i' -ex 'p length' -ex 'p sizeof dev->name' ./sensor_log"
expect "Hardware watchpoint 2: -location dev.ring.head"
expect "^device_set_name"
# shellcheck disable=SC2016  # \$1 is GDB's value history, not a shell variable
expect '^\$1 = 8$'

section "Capture a core file, then analyze it offline"
run "$gdb_batch -ex run -ex 'generate-core-file sensor_log.core' ./sensor_log"
expect "Saved corefile sensor_log.core"
run "$gdb_batch -x crash-report.gdb -ex crash_report ./sensor_log sensor_log.core"
expect "Program terminated with signal SIGSEGV"
expect "faulting instruction"

section "Trace without stopping: dprintf"
run "$gdb_batch -ex 'dprintf ring_push,\"push: head=%zu count=%zu value=%d\\n\", ring->head, ring->count, sample.value' -ex 'run adc0' ./sensor_log"
expect "push: head=3 count=3 value=203"

section "The same crash in an -O2 build"
run "$CC -O2 -g sensor_log.c -o sensor_log_O2"
run "$gdb_batch -ex run -ex bt -ex 'info frame' ./sensor_log_O2"
expect "inlined into frame 1"
run "$gdb_batch -ex 'break device_set_name' -ex 'run adc0' -ex 'info locals' ./sensor_log_O2"
expect "length = <optimized out>"
