#!/usr/bin/env bash
set -euo pipefail

for name in cd printf test grep; do
    printf 'lookup for %s:\n' "$name"
    type -a "$name" || true
    printf '\n'
done

grep() {
    command grep "$@"
}

printf 'function wrapper lookup:\n'
type grep
printf 'command -v grep: %s\n' "$(command -v grep)"
