#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

trace_file=$tmpdir/trace.log

demo_function() {
    local value=$1
    printf 'value=<%s>\n' "$value"
}

exec {trace_fd}> "$trace_file"
BASH_XTRACEFD=$trace_fd
PS4='+ ${BASH_SOURCE##*/}:${LINENO}:${FUNCNAME[0]:-main}: '
set -x
demo_function "two words"
set +x
unset BASH_XTRACEFD
exec {trace_fd}>&-

printf 'trace file:\n'
sed 's/^/  /' "$trace_file"
