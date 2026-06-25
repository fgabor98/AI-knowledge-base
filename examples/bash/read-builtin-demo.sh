#!/usr/bin/env bash
set -euo pipefail

line='alpha beta gamma'
read -r first rest <<< "$line"
printf 'read words: first=%s rest=%s\n' "$first" "$rest"

record='alice,admin,active'
IFS=, read -r user role state <<< "$record"
printf 'read csv-like record: user=%s role=%s state=%s\n' "$user" "$role" "$state"

printf 'read NUL-delimited records:\n'
while IFS= read -r -d '' item; do
    printf '  <%s>\n' "$item"
done < <(printf '%s\0' 'one' 'two words')

tmpfile=$(mktemp)
cleanup() {
    rm -f -- "$tmpfile"
}
trap cleanup EXIT

printf 'unterminated final line' > "$tmpfile"
while IFS= read -r file_line || [[ -n "$file_line" ]]; do
    printf 'final-line-safe read: %s\n' "$file_line"
done < "$tmpfile"
