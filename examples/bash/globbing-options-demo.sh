#!/usr/bin/env bash
# shellcheck disable=SC2035
set -euo pipefail

shopt -s extglob

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

mkdir -p "$tmpdir/src/sub"
touch "$tmpdir/src/a.txt" "$tmpdir/src/b.log" "$tmpdir/src/.hidden" "$tmpdir/src/sub/c.txt"
cd "$tmpdir/src"

show_matches() {
    local label=$1
    shift

    printf '%s\n' "$label"
    if (($#)); then
        printf '  <%s>\n' "$@"
    else
        printf '  <no matches>\n'
    fi
}

show_matches 'default *.txt:' *.txt
show_matches 'default no-match pattern stays literal:' *.md

(
    shopt -s nullglob
    matches=(*.md)
    show_matches 'nullglob no-match pattern disappears:' "${matches[@]}"
)

if (
    shopt -s failglob
    printf '%s\n' *.md
) >/dev/null 2>"$tmpdir/failglob.err"; then
    printf 'failglob unexpectedly matched\n'
else
    failglob_message=$(< "$tmpdir/failglob.err")
    printf 'failglob no-match pattern fails: %s\n' "$failglob_message"
fi

(
    shopt -s globstar
    show_matches 'globstar **/*.txt:' **/*.txt
)

show_matches 'extglob !(*.log):' !(*.log)

(
    shopt -s dotglob
    show_matches 'dotglob * includes dotfiles:' *
)
