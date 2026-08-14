---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Backtracking

Backtracking explores candidate solutions one choice at a time. It applies a choice to a partial solution, rejects the branch if the constraints fail, continues when the partial solution remains possible, and undoes the choice before trying the next alternative.

The pattern is simple, but correctness depends on state restoration. Every field changed while applying a choice must have a defined value after undo. This makes backtracking a useful way to practice invariants, recursion, bounded state, and failure propagation.

## The Backtracking Template

A generic backtracking step looks like this:

```text
if the current state is a goal:
    report success
for each legal choice:
    apply the choice
    if the partial state is still valid:
        search the next state
        if it succeeds:
            propagate success
    undo the choice
report failure
```

The order of operations matters. Do not undo before the recursive call returns, and do not leave a successful path partially undone before the caller has copied or consumed its result.

## Partial-Solution Invariant

For a valid backtracking implementation:

- the state before exploring a choice represents the parent partial solution
- the state during exploration represents exactly the child partial solution
- after the child returns failure, the state again represents exactly the parent

This is stronger than saying "the array looks right." It includes counters, bitsets, resource totals, output lengths, and any auxiliary indexes used by constraint checks.

## Recursive And Iterative Forms

Recursive backtracking stores the active path in the call stack. It is concise and maps directly to the search tree. Its cost is recursion depth and dependence on stack capacity.

Iterative backtracking stores one frame per active decision in an explicit array or other caller-owned structure. It is more verbose, but it makes maximum depth, cancellation, and memory usage visible.

Choose recursion when:

- maximum depth is small and explicit
- the target stack has sufficient margin
- the simpler control flow improves reviewability

Choose an explicit stack when:

- depth depends on external input
- stack usage must be bounded independently of call conventions
- search must pause, resume, or be cancelled at frame boundaries

## Running Example: N-Queens

The N-queens problem places `n` queens on an `n x n` board so that no two queens share a column or diagonal.

Model:

- decision level: one row
- choices: a column for the queen in that row
- state: occupied columns and both diagonal sets
- partial solution: one column position for each earlier row
- goal: all `n` rows have a queen
- failure: no legal column remains for the current row

The bitmask representation makes the constraint check constant time. For a row, a bit is unavailable when it is occupied by a column or either diagonal.

## Programming Examples

### C: Recursive N-Queens Solver

This function finds one solution for `1 <= n <= 32`. `positions[row]` contains one set bit for the selected column. No allocation is performed.

```c
#include <stddef.h>
#include <stdint.h>

enum queens_status {
    QUEENS_FOUND = 0,
    QUEENS_NOT_FOUND,
    QUEENS_ERR_NULL,
    QUEENS_ERR_SIZE
};

static int place_queen(size_t row,
                       size_t n,
                       uint32_t columns,
                       uint32_t diag_down,
                       uint32_t diag_up,
                       uint32_t full_mask,
                       uint32_t *positions)
{
    uint32_t available;

    if (row == n)
        return 1;

    available = full_mask & ~(columns | diag_down | diag_up);

    while (available != 0) {
        uint32_t bit = available & (0u - available);

        available &= available - 1u;
        positions[row] = bit;

        if (place_queen(row + 1,
                        n,
                        columns | bit,
                        (diag_down | bit) << 1,
                        (diag_up | bit) >> 1,
                        full_mask,
                        positions))
            return 1;
    }

    return 0;
}

enum queens_status solve_queens_recursive(size_t n, uint32_t *positions)
{
    uint32_t full_mask;

    if (positions == NULL)
        return QUEENS_ERR_NULL;
    if (n == 0 || n > 32)
        return QUEENS_ERR_SIZE;

    full_mask = n == 32
              ? UINT32_MAX
              : ((UINT32_C(1) << n) - 1u);

    return place_queen(0, n, 0, 0, 0, full_mask, positions)
         ? QUEENS_FOUND
         : QUEENS_NOT_FOUND;
}
```

The diagonal masks are shifted when moving to the next row. Bits outside `full_mask` are discarded when the next row computes `available`.

The solver explores at most O(`n!`) arrangements in the simple worst case, although diagonal constraints eliminate many branches. Its active recursion depth is O(`n`), and the bitmask state is O(1) per frame.

### C: Iterative Solver With An Explicit Stack

This version stores the state needed to resume each row in a fixed array. A frame remembers the masks before entering the row and the choices not yet tried.

```c
#include <stddef.h>
#include <stdint.h>

struct queen_frame {
    uint32_t available;
    uint32_t columns_before;
    uint32_t diag_down_before;
    uint32_t diag_up_before;
    int initialized;
};

enum queens_status solve_queens_iterative(size_t n, uint32_t *positions)
{
    struct queen_frame frames[32] = { 0 };
    uint32_t full_mask;
    uint32_t columns = 0;
    uint32_t diag_down = 0;
    uint32_t diag_up = 0;
    size_t row = 0;

    if (positions == NULL)
        return QUEENS_ERR_NULL;
    if (n == 0 || n > 32)
        return QUEENS_ERR_SIZE;

    full_mask = n == 32
              ? UINT32_MAX
              : ((UINT32_C(1) << n) - 1u);

    for (;;) {
        if (row == n)
            return QUEENS_FOUND;

        if (!frames[row].initialized) {
            frames[row].columns_before = columns;
            frames[row].diag_down_before = diag_down;
            frames[row].diag_up_before = diag_up;
            frames[row].available = full_mask &
                                     ~(columns | diag_down | diag_up);
            frames[row].initialized = 1;
        }

        if (frames[row].available != 0) {
            uint32_t bit = frames[row].available &
                           (0u - frames[row].available);

            frames[row].available &= frames[row].available - 1u;
            positions[row] = bit;
            columns |= bit;
            diag_down = (diag_down | bit) << 1;
            diag_up = (diag_up | bit) >> 1;
            row++;
            continue;
        }

        frames[row].initialized = 0;
        if (row == 0)
            return QUEENS_NOT_FOUND;

        row--;
        columns = frames[row].columns_before;
        diag_down = frames[row].diag_down_before;
        diag_up = frames[row].diag_up_before;
    }
}
```

When a child row fails, the loop moves back to its parent frame and restores the parent masks. The parent frame still contains the remaining choices, so the next iteration tries the next column.

The explicit frame array uses O(`n`) memory with a fixed maximum of 32 rows. This makes the stack requirement visible and independent of the C call stack.

### Python: Reference Backtracking

```python
def solve_queens(n):
    if n <= 0:
        raise ValueError("n must be positive")

    positions = [-1] * n
    columns = set()
    diagonals_down = set()
    diagonals_up = set()

    def visit(row):
        if row == n:
            return positions.copy()

        for column in range(n):
            down = row - column
            up = row + column
            if column in columns or down in diagonals_down or up in diagonals_up:
                continue

            positions[row] = column
            columns.add(column)
            diagonals_down.add(down)
            diagonals_up.add(up)

            result = visit(row + 1)
            if result is not None:
                return result

            columns.remove(column)
            diagonals_down.remove(down)
            diagonals_up.remove(up)
            positions[row] = -1

        return None

    return visit(0)
```

The Python sets make the constraint logic easy to read. The C bitmasks make the same logic bounded and compact for a fixed maximum board size.

## Finding All Solutions

To enumerate all solutions instead of stopping at the first one, report or copy the candidate at the goal and continue the loop. The result storage then becomes part of the resource contract.

Possible policies include:

- caller-provided output array with capacity and count
- callback invoked for each solution
- count-only mode
- stop after a caller-defined solution limit

Do not store every solution by default. Some search spaces have many valid results even when finding one result is cheap.

## Failure Propagation

There are two different kinds of failure:

Search failure:
: The explored space contains no solution under the constraints.

Operational failure:
: The search stopped because of cancellation, node limit, timeout, output capacity, or another resource condition.

Return distinct statuses when the caller needs to know whether absence was proven. A timeout must not be reported as `NOT_FOUND`.

## Common Mistakes

- Applying a choice without undoing every related field.
- Copying a solution pointer that points into mutable backtracking storage.
- Returning success before copying or publishing the final candidate.
- Treating recursion depth as free memory.
- Reusing a frame without clearing its remaining-choice state.
- Using a bit shift whose count or width is not validated.
- Mixing the constraint for the current row with constraints that belong to a different level.

## Embedded And Systems Angle

- prefer explicit stacks when maximum call-stack use is uncertain
- keep apply and undo operations adjacent and easy to compare
- expose node counts and deepest level reached for diagnosing search blowups
- use fixed-size candidate buffers and reject sizes above the documented limit
- make cancellation checks occur at every decision level or another bounded interval

## Related Topics

- [Searching And Backtracking](index.md)
- [Search-Space Modeling](search-space-modeling.md)
- [Pruning And Search Heuristics](pruning-and-search-heuristics.md)
- [Recursion Fundamentals](../control-flow-and-recursion/recursion-fundamentals.md)
- [Loop Invariants And Termination](../control-flow-and-recursion/loop-invariants-and-termination.md)
- [Recursion And Stack-Depth Policy](../embedded-linux-algorithmic-constraints/recursion-and-stack-depth-policy.md)
