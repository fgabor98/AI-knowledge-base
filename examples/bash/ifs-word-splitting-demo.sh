#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

cd "$tmpdir"
touch literal.txt

value='one two *.txt'

printf 'quoted expansion keeps one argument:\n'
printf '  <%s>\n' "$value"

printf 'intentional unquoted splitting and globbing demonstration:\n'
# shellcheck disable=SC2086
set -- $value
printf '  <%s>\n' "$@"

record='alice,admin,active,extra'
IFS=, read -r name role rest <<< "$record"
printf 'csv-like read: name=%s role=%s rest=%s\n' "$name" "$role" "$rest"

line='  leading and trailing  '
IFS= read -r preserved <<< "$line"
printf 'preserved whitespace:<%s>\n' "$preserved"
