#!/usr/bin/env bash
# Run every C check in the repository:
# - standalone examples: strict build, run, ASan+UBSan, TSan (examples/c/Makefile);
# - C code blocks in docs/c compile (scripts/check-c-snippets.py);
# - test-out diagnostic answer keys (examples/c/diagnostics/check.py);
# - failure case studies (scripts/validate-c-case-studies.sh);
# - tool sessions quoted in the docs still produce the quoted output;
# - documentation build.
set -euo pipefail

mkdocs_cmd=${MKDOCS:-mkdocs}
if ! command -v "$mkdocs_cmd" >/dev/null 2>&1 && [[ -x .venv/bin/mkdocs ]]; then
    mkdocs_cmd=.venv/bin/mkdocs
fi

printf 'Building and running C examples...\n'
make --no-print-directory -C examples/c check sanitize tsan

printf 'Compiling C snippets in the documentation...\n'
python3 scripts/check-c-snippets.py docs/c

printf 'Checking the test-out answer keys...\n'
python3 examples/c/diagnostics/check.py

printf 'Validating C failure case studies...\n'
scripts/validate-c-case-studies.sh

printf 'Running tool sessions...\n'
shellcheck -x -P SCRIPTDIR examples/c/tool-sessions/lib.sh \
    examples/c/tool-sessions/run-all.sh examples/c/tool-sessions/*/run.sh
for session in examples/c/tool-sessions/*/run.sh; do
    printf '  %s\n' "$(basename "$(dirname "$session")")"
    bash "$session" >/dev/null
done

printf 'Building documentation...\n'
"$mkdocs_cmd" build --strict

printf 'C validation passed.\n'
