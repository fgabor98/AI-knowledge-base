#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${BASH_VERSION:-}" ]]; then
    printf 'running under Bash %s\n' "$BASH_VERSION"
else
    printf 'not running under Bash\n'
fi

items=("one" "two words" "")

printf 'Bash array elements:\n'
printf '<%s>\n' "${items[@]}"

printf 'Portable command lookup still works in Bash:\n'
command -v printf
