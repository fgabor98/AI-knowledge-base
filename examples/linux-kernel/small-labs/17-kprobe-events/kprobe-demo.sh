#!/usr/bin/env bash
# Trace lab_char's write callback through tracefs, without changing the module.
set -euo pipefail

T=/sys/kernel/tracing
GROUP=lab17

if (( EUID != 0 )); then
	echo "run as root: tracefs needs it" >&2
	exit 1
fi
if [[ ! -e /dev/demo0 ]]; then
	echo "/dev/demo0 missing: load lab 3 first" >&2
	exit 1
fi
[[ -e $T/kprobe_events ]] || mount -t tracefs tracefs "$T"

cleanup() {
	echo 0 > "$T/events/$GROUP/enable" 2>/dev/null || true
	echo "-:$GROUP/write" >> "$T/kprobe_events" 2>/dev/null || true
	echo "-:$GROUP/write_ret" >> "$T/kprobe_events" 2>/dev/null || true
}
trap cleanup EXIT

# Arguments of demo_write(file, user_buffer, count, position) are $arg1..$arg4.
# The module qualifier avoids a same-named function elsewhere in the kernel.
echo "p:$GROUP/write lab_char:demo_write count=\$arg3:u64 first=+0(\$arg2):ustring" >> "$T/kprobe_events"
echo "r:$GROUP/write_ret lab_char:demo_write ret=\$retval:s64" >> "$T/kprobe_events"

: > "$T/trace"
echo 1 > "$T/events/$GROUP/enable"

printf 'hello kprobes' > /dev/demo0
printf '%0200d' 0 > /dev/demo0

echo 0 > "$T/events/$GROUP/enable"
grep -E 'write' "$T/trace" | grep -v '^#'
