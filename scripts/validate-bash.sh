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

printf 'Running Bash example smoke tests...\n'
bash examples/bash/execution-model-demo.sh >/dev/null 2>&1
REQUIRED_VALUE=present bash examples/bash/expansion-demo.sh Example >/dev/null 2>&1
bash examples/bash/quoting-demo.sh >/dev/null 2>&1
bash examples/bash/exit-code-demo.sh >/dev/null 2>&1
bash examples/bash/pipefail-demo.sh >/dev/null 2>&1
bash examples/bash/conditionals-demo.sh docs/bash verbose >/dev/null 2>&1
bash examples/bash/loops-demo.sh "one arg" "" >/dev/null 2>&1
bash examples/bash/functions-demo.sh docs/bash/quoting.md >/dev/null 2>&1
bash examples/bash/robust-script-template.sh --dry-run examples/bash >/dev/null 2>&1
bash examples/bash/bash-vs-posix-demo.sh >/dev/null 2>&1
bash examples/bash/redirection-demo.sh >/dev/null 2>&1
bash examples/bash/subshell-demo.sh >/dev/null 2>&1
bash examples/bash/read-lines-safely.sh >/dev/null 2>&1
bash examples/bash/getopts-demo.sh -v -o out.txt input.txt >/dev/null 2>&1
bash examples/bash/manual-args-demo.sh --verbose --output out.txt -- input.txt >/dev/null 2>&1
bash examples/bash/logging-demo.sh --verbose >/dev/null 2>&1
bash examples/bash/trap-cleanup.sh >/dev/null 2>&1
bash examples/bash/retry-demo.sh >/dev/null 2>&1
bash examples/bash/safe-filesystem-operations.sh >/dev/null 2>&1
bash examples/bash/errexit-err-trap-demo.sh >/dev/null 2>&1
bash examples/bash/advanced-fd-demo.sh >/dev/null 2>&1
bash examples/bash/process-management-demo.sh >/dev/null 2>&1
bash examples/bash/bounded-parallelism-demo.sh >/dev/null 2>&1
bash examples/bash/advanced-parameter-expansion-demo.sh >/dev/null 2>&1
bash examples/bash/associative-arrays-demo.sh >/dev/null 2>&1
bash examples/bash/eval-injection-demo.sh >/dev/null 2>&1
bash examples/bash/tracing-demo.sh >/dev/null 2>&1
bash examples/bash/completion-demo.sh >/dev/null 2>&1
bash examples/bash/testable-calculator.sh add 2 3 >/dev/null 2>&1
bash examples/bash/portability-matrix-demo.sh >/dev/null 2>&1
bash examples/bash/performance-boundaries-demo.sh >/dev/null 2>&1

if command -v bats >/dev/null 2>&1; then
    printf 'Running Bats tests...\n'
    bats examples/bash/testable-calculator.bats >/dev/null
else
    printf 'Skipping Bats tests; bats is not installed.\n'
fi

printf 'Building documentation...\n'
mkdocs build --strict

printf 'Bash validation passed.\n'
