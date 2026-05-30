#!/usr/bin/env bash
set -u

tmpfile=$(mktemp)
cleanup() {
    rm -f -- "$tmpfile"
}
trap cleanup EXIT

printf 'alpha\nbeta\n' > "$tmpfile"

check_pattern() {
    local pattern=$1
    local file=$2
    local status

    if grep -q -- "$pattern" "$file"; then
        printf 'match: pattern <%s> exists in %s\n' "$pattern" "$file"
    else
        status=$?
        case "$status" in
            1)
                printf 'no match: pattern <%s> was not found in %s\n' "$pattern" "$file"
                ;;
            *)
                printf 'grep error: status=%d pattern=<%s> file=%s\n' "$status" "$pattern" "$file" >&2
                ;;
        esac
    fi
}

check_pattern "alpha" "$tmpfile"
check_pattern "gamma" "$tmpfile"
check_pattern "alpha" "$tmpfile.missing"
