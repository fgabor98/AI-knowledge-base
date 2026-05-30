#!/usr/bin/env bash
set -euo pipefail

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

touch "$tmpdir/a.sh" "$tmpdir/b.sh"

print_args() {
    printf 'argc=%d\n' "$#"

    local i=1
    local arg
    for arg in "$@"; do
        printf 'arg[%d]=<%s>\n' "$i" "$arg"
        ((i += 1))
    done
}

(
    cd "$tmpdir"

    value='two words *.sh'

    printf 'Unquoted scalar expansion:\n'
    # Intentionally unquoted to demonstrate word splitting and glob expansion.
    # shellcheck disable=SC2086
    print_args $value

    printf '\nQuoted scalar expansion:\n'
    print_args "$value"

    items=("alpha" "two words" "")

    printf '\nArray expansion with "${items[@]}":\n'
    print_args "${items[@]}"

    printf '\nForwarding positional arguments with "$@":\n'
    set -- "first value" "" "third*value"
    print_args "$@"
)
