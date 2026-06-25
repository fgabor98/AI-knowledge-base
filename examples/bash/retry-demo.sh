#!/usr/bin/env bash
set -euo pipefail

retry() {
    local attempts=$1
    local delay=$2
    shift 2

    local attempt status=0
    for ((attempt = 1; attempt <= attempts; attempt += 1)); do
        if "$@"; then
            return 0
        else
            status=$?
        fi

        printf 'attempt %d/%d failed with status %d\n' "$attempt" "$attempts" "$status" >&2

        if ((attempt < attempts)); then
            sleep "$delay"
        fi
    done

    return "$status"
}

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

counter_file=$tmpdir/counter
printf '0\n' > "$counter_file"

flaky_command() {
    local count
    count=$(<"$counter_file")
    ((count += 1))
    printf '%d\n' "$count" > "$counter_file"

    if ((count < 3)); then
        printf 'simulated transient failure\n' >&2
        return 1
    fi

    printf 'success on attempt %d\n' "$count"
}

retry 5 0 flaky_command
