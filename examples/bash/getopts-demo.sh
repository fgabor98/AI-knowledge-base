#!/usr/bin/env bash
set -euo pipefail

usage() {
    printf 'Usage: %s [-v] [-o FILE] INPUT\n' "${0##*/}"
}

verbose=0
output=-

while getopts ':hvo:' opt; do
    case "$opt" in
        h)
            usage
            exit 0
            ;;
        v)
            verbose=1
            ;;
        o)
            output=$OPTARG
            ;;
        :)
            printf 'option -%s requires an argument\n' "$OPTARG" >&2
            usage >&2
            exit 2
            ;;
        \?)
            printf 'unknown option: -%s\n' "$OPTARG" >&2
            usage >&2
            exit 2
            ;;
    esac
done
shift "$((OPTIND - 1))"

(($# == 1)) || {
    usage >&2
    exit 2
}

input=$1
printf 'verbose=%d\n' "$verbose"
printf 'output=%s\n' "$output"
printf 'input=%s\n' "$input"
