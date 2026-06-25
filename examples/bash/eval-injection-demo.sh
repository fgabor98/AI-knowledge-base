#!/usr/bin/env bash
set -euo pipefail

tmpfile=$(mktemp)
cleanup() {
    rm -f -- "$tmpfile"
}
trap cleanup EXIT

printf 'needle\nliteral; echo injected\n' > "$tmpfile"

pattern='literal; echo injected'
unsafe_preview="grep -n $pattern $tmpfile"
safe_cmd=(grep -n -- "$pattern" "$tmpfile")

printf 'unsafe command string, not executed:\n'
printf '  %s\n' "$unsafe_preview"

printf 'safe argument vector:\n'
printf '  <%s>\n' "${safe_cmd[@]}"

printf 'safe execution result:\n'
"${safe_cmd[@]}"
