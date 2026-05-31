#!/usr/bin/env bash
# The nested bash -c snippets intentionally contain shell variables that expand
# inside the child shell, not in this outer script.
# shellcheck disable=SC2016
set -euo pipefail

run_case() {
    local label=$1
    local script=$2

    printf '\n%s\n' "$label"
    if bash -Eeuo pipefail -c "$script"; then
        printf 'status=0\n'
    else
        printf 'status=%d\n' "$?"
    fi
}

run_case 'plain failure triggers ERR' '
trap '\''printf "ERR line=%d command=%s\n" "$LINENO" "$BASH_COMMAND" >&2'\'' ERR
false
printf "not reached\n"
'

run_case 'failure in if condition is handled explicitly' '
trap '\''printf "ERR line=%d command=%s\n" "$LINENO" "$BASH_COMMAND" >&2'\'' ERR
if false; then
    printf "unexpected\n"
else
    printf "condition failed without ERR trap\n"
fi
'

run_case 'pipefail makes upstream failure matter' '
trap '\''printf "ERR line=%d command=%s\n" "$LINENO" "$BASH_COMMAND" >&2'\'' ERR
set -o pipefail
false | cat >/dev/null
'
