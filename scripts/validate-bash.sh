#!/usr/bin/env bash
set -euo pipefail

shopt -s nullglob

scripts=(scripts/validate.sh scripts/validate-bash.sh examples/bash/*.sh)

if ((${#scripts[@]} == 0)); then
    printf 'No Bash scripts found to validate.\n' >&2
    exit 1
fi

printf 'Checking Bash syntax...\n'
bash -n "${scripts[@]}"

printf 'Running ShellCheck...\n'
shellcheck "${scripts[@]}"

printf 'Building documentation...\n'
mkdocs build --strict

printf 'Bash validation passed.\n'
