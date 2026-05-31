#!/usr/bin/env bash
set -euo pipefail

subshell_count=0

printf 'parent before: pwd=%s count=%d BASHPID=%s\n' "$PWD" "$subshell_count" "$BASHPID"

# This block intentionally demonstrates state that is lost when a subshell exits.
# shellcheck disable=SC2030
(
    cd /tmp
    subshell_count=42
    printf 'subshell: pwd=%s count=%d BASHPID=%s\n' "$PWD" "$subshell_count" "$BASHPID"
)

# shellcheck disable=SC2031
printf 'parent after subshell: pwd=%s count=%d BASHPID=%s\n' "$PWD" "$subshell_count" "$BASHPID"

pipeline_count=0

# This pipeline intentionally demonstrates state lost in a pipeline-fed loop.
# shellcheck disable=SC2030,SC2031
printf '%s\n' a b c | while IFS= read -r _line; do
    ((pipeline_count += 1))
done

# shellcheck disable=SC2031
printf 'parent after pipeline loop: count=%d\n' "$pipeline_count"

process_count=0
while IFS= read -r _line; do
    ((process_count += 1))
done < <(printf '%s\n' a b c)

printf 'parent after process-substitution loop: count=%d\n' "$process_count"
