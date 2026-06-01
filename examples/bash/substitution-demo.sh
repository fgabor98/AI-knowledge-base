#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

left=$tmpdir/left.txt
right=$tmpdir/right.txt

printf 'banana\napple\n' > "$left"
printf 'apple\nbanana\n' > "$right"

base_name=$(basename "$left")
line_count=$(wc -l < "$left")

printf 'command substitution basename: %s\n' "$base_name"
printf 'command substitution line count: %s\n' "$line_count"

if grep_output=$(grep -n -- 'apple' "$left"); then
    printf 'captured grep output: %s\n' "$grep_output"
fi

if missing_output=$(grep -n -- 'missing' "$left"); then
    printf 'unexpected match: %s\n' "$missing_output"
else
    status=$?
    printf 'captured failed grep status: %d\n' "$status"
fi

if diff -u <(sort "$left") <(sort "$right") >/dev/null; then
    printf 'process substitution diff: sorted contents match\n'
fi

printf 'process substitution common lines:\n'
comm -12 <(sort "$left") <(sort "$right")
