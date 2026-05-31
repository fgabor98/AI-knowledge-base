#!/usr/bin/env bash
set -euo pipefail

max_jobs=2
pids=()
status=0

run_task() {
    local item=$1
    printf 'start %s\n' "$item"
    sleep "0.$item"
    printf 'done %s\n' "$item"
}

wait_oldest() {
    local pid=${pids[0]}
    if ! wait "$pid"; then
        status=1
    fi
    pids=("${pids[@]:1}")
}

for item in 1 2 3 4; do
    run_task "$item" &
    pids+=("$!")

    if ((${#pids[@]} >= max_jobs)); then
        wait_oldest
    fi
done

while ((${#pids[@]})); do
    wait_oldest
done

exit "$status"
