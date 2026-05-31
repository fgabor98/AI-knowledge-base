#!/usr/bin/env bash
set -euo pipefail

log() {
    printf '%s\n' "$*" >&2
}

die() {
    log "error: $*"
    exit 1
}

need_command() {
    command -v "$1" >/dev/null 2>&1 || {
        log "missing command: $1"
        return 1
    }
}

basename_without_ext() {
    local path=$1
    local name=${path##*/}
    printf '%s\n' "${name%.*}"
}

main() {
    (($# == 1)) || die "usage: ${0##*/} PATH"

    need_command printf

    local result
    result=$(basename_without_ext "$1")
    printf 'basename without extension: %s\n' "$result"
}

main "$@"
