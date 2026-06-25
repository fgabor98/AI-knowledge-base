#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

out=$tmpdir/out.log
err=$tmpdir/err.log
combined=$tmpdir/combined.log

demo_command() {
    printf 'stdout line\n'
    printf 'stderr line\n' >&2
}

demo_command > "$out" 2> "$err"
demo_command > "$combined" 2>&1

printf 'stdout file:\n'
sed 's/^/  /' "$out"

printf 'stderr file:\n'
sed 's/^/  /' "$err"

printf 'combined file:\n'
sed 's/^/  /' "$combined"
