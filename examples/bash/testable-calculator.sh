#!/usr/bin/env bash
set -euo pipefail

usage() {
    printf 'Usage: %s add A B\n' "${0##*/}"
}

main() {
    (($# == 3)) || {
        usage >&2
        exit 2
    }

    local op=$1
    local a=$2
    local b=$3

    case "$op" in
        add)
            [[ $a =~ ^-?[0-9]+$ && $b =~ ^-?[0-9]+$ ]] || {
                printf 'operands must be integers\n' >&2
                exit 2
            }
            printf '%d\n' "$((a + b))"
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
}

main "$@"
