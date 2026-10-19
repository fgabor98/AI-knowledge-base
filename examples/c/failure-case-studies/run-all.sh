#!/usr/bin/env bash
# Run every failure case study and print the full transcript.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

for script in "$root"/[0-9][0-9]-*/run.sh; do
    printf '\n################ %s\n' "$(basename "$(dirname "$script")")"
    bash "$script"
done
