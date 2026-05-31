#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

src=$tmpdir/source.txt
dst=$tmpdir/"output file.txt"
target=$tmpdir/config.txt

printf 'source data\n' > "$src"

[[ -f "$src" ]] || {
    printf 'source is not a file: %s\n' "$src" >&2
    exit 1
}

cp -- "$src" "$dst"
printf 'copied to: %s\n' "$dst"

tmp=$(mktemp "${target}.XXXXXX")
printf 'generated config\n' > "$tmp"
mv -- "$tmp" "$target"
printf 'atomically replaced: %s\n' "$target"

printf 'NUL-delimited file walk:\n'
find "$tmpdir" -type f -print0 |
while IFS= read -r -d '' path; do
    printf '<%s>\n' "$path"
done
