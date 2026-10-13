#!/usr/bin/env bash
# Case 15: -ffast-math lets GCC assume NaN never occurs, so isnan() is false.
# shellcheck source=../lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib.sh"

section "Default build: the guard works"
compile bug -O2 -Wall -Wextra bug.c
execute bug nan

section "-ffast-math: no diagnostic, NaN flows into the filter state"
compile bug-fast -O2 -Wall -Wextra -ffast-math bug.c
execute bug-fast nan

section "The guard is gone from the generated code"
show_asm bug.c smooth -O2 -ffast-math

section "-ffinite-math-only alone is enough to cause it"
compile bug-finite -O2 -ffinite-math-only bug.c
execute bug-finite nan

section "Fixed: representation check survives -ffast-math"
compile fixed -O2 -Wall -Wextra -ffast-math fixed.c
execute fixed nan
execute fixed 30
