#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

touch "$tmpdir/one.log" "$tmpdir/two words.log"

printf 'Arguments:\n'
for arg in "$@"; do
    printf '  <%s>\n' "$arg"
done

printf 'Array values:\n'
items=("alpha" "two words" "")
for item in "${items[@]}"; do
    printf '  <%s>\n' "$item"
done

printf 'Glob values:\n'
for path in "$tmpdir"/*.log; do
    [[ -e "$path" ]] || continue
    printf '  <%s>\n' "$path"
done

printf 'Counter values:\n'
for ((i = 0; i < 3; i += 1)); do
    printf '  %d\n' "$i"
done
