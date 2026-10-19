#!/usr/bin/env bash
# Run every tool session and print the full transcript. Exits non-zero if a
# session's output no longer matches what the docs quote.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

for script in "$root"/*/run.sh; do
    printf '\n################ %s\n' "$(basename "$(dirname "$script")")"
    bash "$script"
done
