#!/usr/bin/env bash
set -euo pipefail

usage() {
    printf 'Usage: %s [--verbose] [--output FILE] INPUT\n' "${0##*/}"
}

verbose=0
output=-

while (($#)); do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        --verbose)
            verbose=1
            shift
            ;;
        --output)
            (($# >= 2)) || {
                printf '--output requires an argument\n' >&2
                usage >&2
                exit 2
            }
            output=$2
            shift 2
            ;;
        --)
            shift
            break
            ;;
        -*)
            printf 'unknown option: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
        *)
            break
            ;;
    esac
done

(($# == 1)) || {
    usage >&2
    exit 2
}

input=$1
printf 'verbose=%d\n' "$verbose"
printf 'output=%s\n' "$output"
printf 'input=%s\n' "$input"
