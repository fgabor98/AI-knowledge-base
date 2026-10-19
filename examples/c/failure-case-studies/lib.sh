#!/usr/bin/env bash
# Shared helpers for the C failure case-study reproducers.
#
# Each case directory has a run.sh that sources this file, builds the buggy
# and fixed programs into a temporary directory, and prints every command
# next to its output so the transcript can be compared with the docs page.

set -euo pipefail

CC=${CC:-gcc}

# Directory of the run.sh that sourced this file.
case_dir=$(cd "$(dirname "${BASH_SOURCE[1]}")" && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT

section() {
    printf '\n== %s\n' "$*"
}

# compile OUTPUT ARGS... - compile inside the case directory, show diagnostics.
compile() {
    local output=$1
    shift
    printf '$ %s %s -o %s\n' "$CC" "$*" "$output"
    (cd "$case_dir" && "$CC" "$@" -o "$build_dir/$output" 2>&1)
}

# compile_expect_failure ARGS... - a build that is supposed to be rejected.
compile_expect_failure() {
    printf '$ %s %s\n' "$CC" "$*"
    if (cd "$case_dir" && "$CC" "$@" -o "$build_dir/rejected" 2>&1); then
        printf '[unexpected: build succeeded]\n'
        return 1
    fi
    printf '[build rejected, as intended]\n'
}

# execute PROGRAM ARGS... - run a built program; never abort the script.
execute() {
    local program=$1
    shift
    printf '$ ./%s %s\n' "$program" "$*"
    local status=0
    timeout 5 "$build_dir/$program" "$@" 2>&1 || status=$?
    if ((status == 124)); then
        printf '[killed after 5 s: program did not terminate]\n'
    elif ((status != 0)); then
        printf '[exit status %d]\n' "$status"
    fi
}

# show_asm SOURCE FUNCTION ARGS... - Intel-syntax assembly for one function.
show_asm() {
    local source=$1 function=$2
    shift 2
    printf '$ %s -S %s %s  # function %s\n' "$CC" "$*" "$source" "$function"
    (cd "$case_dir" &&
        "$CC" -S -o - -masm=intel -fno-asynchronous-unwind-tables \
            -fcf-protection=none "$@" "$source") |
        awk -v fn="$function" '
            $0 ~ "^" fn "(\\.[a-z0-9.]+)?:" { printing = 1 }
            printing && $0 ~ "^\t\\.size\t" fn { exit }
            printing && $0 !~ "^\t\\." { print }
        '
}

# without_aslr COMMAND... - ThreadSanitizer needs a fixed address-space
# layout on kernels with high mmap randomization.
without_aslr() {
    if command -v setarch >/dev/null 2>&1; then
        setarch "$(uname -m)" -R "$@"
    else
        "$@"
    fi
}
