#!/usr/bin/env bash
set -euo pipefail

printf 'Bash version: %s\n' "$BASH_VERSION"
printf 'Bash major=%s minor=%s\n' "${BASH_VERSINFO[0]}" "${BASH_VERSINFO[1]}"

if ((BASH_VERSINFO[0] < 4)); then
    printf 'Bash 4 or newer is required for associative arrays\n' >&2
    exit 2
fi

for cmd in bash find mktemp shellcheck; do
    if command -v "$cmd" >/dev/null 2>&1; then
        printf 'found: %s -> %s\n' "$cmd" "$(command -v "$cmd")"
    else
        printf 'missing: %s\n' "$cmd"
    fi
done
