#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

input=$tmpdir/input.txt
printf ' leading space\ntrailing space \nbackslash \\ value\nfinal-no-newline' > "$input"

printf 'Reading text lines safely:\n'
while IFS= read -r line || [[ -n "$line" ]]; do
    printf '<%s>\n' "$line"
done < "$input"

printf '\nReading NUL-delimited paths safely:\n'
touch "$tmpdir/file one" "$tmpdir/file two"
find "$tmpdir" -maxdepth 1 -type f -print0 |
while IFS= read -r -d '' path; do
    printf '<%s>\n' "$path"
done
