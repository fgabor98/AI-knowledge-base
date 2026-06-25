#!/usr/bin/env bash
set -euo pipefail

path=${1:-.}
mode=${2:-status}

if [[ -d "$path" ]]; then
    printf 'directory: %s\n' "$path"
elif [[ -f "$path" ]]; then
    printf 'file: %s\n' "$path"
else
    printf 'not a regular file or directory: %s\n' "$path"
fi

case "$mode" in
    status)
        printf 'mode: status\n'
        ;;
    verbose)
        printf 'mode: verbose\n'
        ;;
    *)
        printf 'unknown mode: %s\n' "$mode" >&2
        exit 2
        ;;
esac

count=3
if ((count > 1)); then
    printf 'count is greater than one\n'
fi
