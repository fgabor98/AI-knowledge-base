#!/usr/bin/env bash
set -euo pipefail

verbose=0

if [[ ${1:-} == --verbose ]]; then
    verbose=1
fi

log() {
    printf '%s\n' "$*" >&2
}

debug() {
    ((verbose)) || return 0
    printf 'debug: %s\n' "$*" >&2
}

log 'querying device state'
debug 'verbose diagnostics enabled'
printf '{"state":"ready"}\n'
