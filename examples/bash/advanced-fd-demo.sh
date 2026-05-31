#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

log_file=$tmpdir/script.log
trace_file=$tmpdir/trace.log

exec {log_fd}> "$log_file"
printf 'log: starting\n' >&"$log_fd"

exec {trace_fd}> "$trace_file"
BASH_XTRACEFD=$trace_fd
PS4='+ ${LINENO}: '
set -x
value="two words"
printf 'data=<%s>\n' "$value"
set +x
unset BASH_XTRACEFD
exec {trace_fd}>&-

printf 'log: finished\n' >&"$log_fd"
exec {log_fd}>&-

printf 'log file:\n'
sed 's/^/  /' "$log_file"
printf 'trace file:\n'
sed 's/^/  /' "$trace_file"
