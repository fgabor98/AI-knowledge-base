#!/usr/bin/env bash
set -euo pipefail

tmpdir=

cleanup() {
    local status=$?
    if [[ -n "${tmpdir:-}" && -d "$tmpdir" ]]; then
        printf 'cleaning up: %s\n' "$tmpdir" >&2
        rm -rf -- "$tmpdir"
    fi
    exit "$status"
}
trap cleanup EXIT

tmpdir=$(mktemp -d)
printf 'created temporary directory: %s\n' "$tmpdir"

printf 'temporary data\n' > "$tmpdir/data.txt"
printf 'created file: %s\n' "$tmpdir/data.txt"
