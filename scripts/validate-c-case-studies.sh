#!/usr/bin/env bash
# Validate the C failure case studies:
# - every buggy program still compiles (the docs depend on it);
# - every fixed program builds warning-free and runs cleanly under
#   AddressSanitizer and UndefinedBehaviorSanitizer.
set -euo pipefail

root=examples/c/failure-case-studies
cc=${CC:-gcc}
strict_flags=(-O2 -Wall -Wextra -Wconversion -Wsign-conversion -Werror -pthread)
sanitize_flags=(-O1 -g '-fsanitize=address,undefined' -fno-sanitize-recover=all -pthread)

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

sources_of() {
    local case_dir=$1 variant=$2
    if [[ -f $case_dir/$variant.c ]]; then
        printf '%s\n' "$case_dir/$variant.c"
    else
        find "$case_dir/$variant" -maxdepth 1 -name '*.c' ! -name 'layout_assert.c' | sort
    fi
}

# Docs under docs/c embed parts of example sources as --8<-- "file:START:END". Each range is
# meant to end on a closing brace; anything else means the source moved.
printf 'Checking snippet line ranges in the docs...\n'
range_errors=0
while IFS= read -r snippet; do
    file=${snippet%%:*}
    range=${snippet#*:}
    end=${range#*:}
    last_line=$(sed -n "${end}p" "$file")
    if [[ $last_line != '}' && $last_line != '};' ]]; then
        printf 'error: %s ends on "%s", expected a closing brace\n' "$snippet" "$last_line" >&2
        range_errors=1
    fi
done < <(grep -rohE --include='*.md' -- '--8<-- "[^"]+:[0-9]+:[0-9]+"' docs/c |
    sed -E 's/^--8<-- "(.*)"$/\1/')
((range_errors == 0))

printf 'Running ShellCheck on case-study scripts...\n'
shellcheck -x -P SCRIPTDIR "$root"/lib.sh "$root"/run-all.sh "$root"/*/run.sh "$root"/*/fixed/*.sh

for case_dir in "$root"/[0-9][0-9]-*/; do
    case_dir=${case_dir%/}
    name=$(basename "$case_dir")
    printf 'Checking %s...\n' "$name"

    mapfile -t bug_sources < <(sources_of "$case_dir" bug)
    mapfile -t fixed_sources < <(sources_of "$case_dir" fixed)

    # Some buggy programs only link with -fcommon (case 14); compiling is enough.
    for source in "${bug_sources[@]}"; do
        "$cc" -c -O2 -pthread "$source" -o "$work/bug.o"
    done

    "$cc" "${strict_flags[@]}" "${fixed_sources[@]}" -o "$work/$name"
    "$work/$name" >/dev/null

    "$cc" "${sanitize_flags[@]}" "${fixed_sources[@]}" -o "$work/$name-sanitized"
    "$work/$name-sanitized" >/dev/null
done

printf 'C case-study validation passed.\n'
