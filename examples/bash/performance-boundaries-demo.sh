#!/usr/bin/env bash
set -euo pipefail

tmpfile=$(mktemp)
cleanup() {
    rm -f -- "$tmpfile"
}
trap cleanup EXIT

printf 'alpha\nbeta\ngamma\n' > "$tmpfile"

printf 'Bash loop for orchestration:\n'
while IFS= read -r line; do
    printf '  item=%s\n' "$line"
done < "$tmpfile"

printf 'Single streaming tool for transformation:\n'
tr '[:lower:]' '[:upper:]' < "$tmpfile"
