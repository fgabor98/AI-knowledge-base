#!/usr/bin/env bash
set -u

show_result() {
    local label=$1
    local status=$2
    shift 2
    local parts=("$@")

    printf '%s\n' "$label"
    printf 'pipeline status: %d\n' "$status"
    printf 'PIPESTATUS:'

    local part
    for part in "${parts[@]}"; do
        printf ' %d' "$part"
    done

    printf '\n\n'
}

set +o pipefail
false | true
status=$? parts=("${PIPESTATUS[@]}")
show_result 'Without pipefail: false | true' "$status" "${parts[@]}"

set -o pipefail
false | true
status=$? parts=("${PIPESTATUS[@]}")
show_result 'With pipefail: false | true' "$status" "${parts[@]}"

set +o pipefail
grep -q -- "needle" /path/that/does/not/exist | true
status=$? parts=("${PIPESTATUS[@]}")
show_result 'Without pipefail: failed grep hidden by true' "$status" "${parts[@]}"

set -o pipefail
grep -q -- "needle" /path/that/does/not/exist | true
status=$? parts=("${PIPESTATUS[@]}")
show_result 'With pipefail: failed grep affects pipeline' "$status" "${parts[@]}"
