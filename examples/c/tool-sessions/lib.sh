#!/usr/bin/env bash
# Shared helpers for the tool-session scripts.
#
# A session script copies its sources into a scratch directory, runs real
# tool commands there with short relative paths, and prints each command with
# its output. `expect` checks that a key line is still present, so the
# validation script notices when a toolchain change alters a transcript that
# the docs quote.

set -euo pipefail

CC=${CC:-gcc}

session_dir=$(cd "$(dirname "${BASH_SOURCE[1]}")" && pwd)
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT
last_output="$work_dir/.last-output"

cp -R "$session_dir"/. "$work_dir"/
cd "$work_dir"

section() {
    printf '\n== %s\n' "$*"
}

# run COMMAND - print and execute a shell command in the scratch directory.
# The output is shown and kept for `expect`; a failing command does not stop
# the session, because several sessions demonstrate failures on purpose.
run() {
    printf '$ %s\n' "$*"
    local status=0
    # The trailing "exit" keeps bash from exec'ing the command directly, so a
    # crash is reported by this inner shell (and captured) rather than by the
    # session script.
    bash -c "$*"$'\n''exit $?' 2>&1 | normalize >"$last_output" || status=${PIPESTATUS[0]}
    if ((status != 0)); then
        printf '[exit status %d]\n' "$status" >>"$last_output"
    fi
    cat "$last_output"
}

# normalize - make transcripts readable and stable: show the scratch
# directory as ".", strip PIDs from crash notices, and drop GDB's
# thread-library banner lines.
normalize() {
    sed -E -e "s|$work_dir|.|g" \
        -e 's/^bash: line [0-9]+: +[0-9]+ ([A-Z][a-z ]*[a-z]) +(\(core dumped\) )?.*/\1 \2/' \
        -e 's/ +$//' |
        grep -Ev '^(\[Thread debugging using libthread_db enabled\]|Using host libthread_db library)' || true
}

# quiet COMMAND - execute without printing (setup steps).
quiet() {
    bash -c "$*" >/dev/null 2>&1
}

# expect REGEX - fail the session if the last output does not match.
expect() {
    if ! grep -Eq -- "$1" "$last_output"; then
        printf '\nsession check failed: expected output matching /%s/\n' "$1" >&2
        exit 1
    fi
}

# without_aslr COMMAND... - ThreadSanitizer needs a fixed address-space layout
# on kernels with high mmap randomization.
without_aslr() {
    if command -v setarch >/dev/null 2>&1; then
        printf 'setarch %s -R' "$(uname -m)"
    fi
}
