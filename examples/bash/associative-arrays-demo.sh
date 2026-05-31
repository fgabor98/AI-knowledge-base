#!/usr/bin/env bash
set -euo pipefail

if ((BASH_VERSINFO[0] < 4)); then
    printf 'associative arrays require Bash 4 or newer\n' >&2
    exit 2
fi

declare -A counts=()
levels=(info error warning error info error)

for level in "${levels[@]}"; do
    ((counts[$level] += 1))
done

for level in "${!counts[@]}"; do
    printf '%s=%d\n' "$level" "${counts[$level]}"
done | sort

key="missing"
if [[ ${counts[$key]+present} ]]; then
    printf '%s exists\n' "$key"
else
    printf '%s is absent\n' "$key"
fi
