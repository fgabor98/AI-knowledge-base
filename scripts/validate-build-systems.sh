#!/usr/bin/env bash
# Run every build-systems check in the repository:
# - tool sessions run and still produce the lines they `expect`;
# - every transcript quoted in docs/build-systems matches real session output
#   (scripts/check-build-transcripts.py; Yocto sessions are compared with
#   their recorded transcripts, refreshed by examples/build-systems/yocto/run.sh);
# - documentation build.
set -euo pipefail

mkdocs_cmd=${MKDOCS:-mkdocs}
if ! command -v "$mkdocs_cmd" >/dev/null 2>&1 && [[ -x .venv/bin/mkdocs ]]; then
    mkdocs_cmd=.venv/bin/mkdocs
fi

printf 'Linting session scripts...\n'
shellcheck -x -P SCRIPTDIR examples/build-systems/tool-sessions/*/run.sh \
    examples/build-systems/yocto/run.sh

printf 'Running tool sessions...\n'
for session in examples/build-systems/tool-sessions/*/run.sh; do
    printf '  %s\n' "$(basename "$(dirname "$session")")"
    bash "$session" >/dev/null
done

printf 'Checking quoted transcripts...\n'
"${PYTHON:-python3}" scripts/check-build-transcripts.py

printf 'Building documentation...\n'
"$mkdocs_cmd" build --strict

printf 'Build systems validation passed.\n'
