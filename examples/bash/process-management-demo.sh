#!/usr/bin/env bash
# shellcheck disable=SC2317
set -euo pipefail

children=()

cleanup() {
    local status=$?
    if ((${#children[@]})); then
        kill "${children[@]}" 2>/dev/null || true
        wait "${children[@]}" 2>/dev/null || true
    fi
    exit "$status"
}
trap cleanup EXIT INT TERM

start_child() {
    "$@" &
    children+=("$!")
    printf 'started pid=%s command=%s\n' "$!" "$1"
}

start_child sleep 0.1
start_child sleep 0.2

status=0
for pid in "${children[@]}"; do
    if wait "$pid"; then
        printf 'child completed pid=%s\n' "$pid"
    else
        printf 'child failed pid=%s\n' "$pid" >&2
        status=1
    fi
done

children=()
exit "$status"
