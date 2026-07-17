---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Invariants And Correctness

An invariant is a fact that must remain true. Correctness is the argument that an algorithm returns the required result when its preconditions hold.

Invariants are one of the most practical tools in algorithm design. They let you reason about loops, data structures, state machines, queues, pools, and ownership rules. They also give reviewers a way to decide whether a change preserves behavior.

## Types Of Invariants

Loop invariant:
: A fact that is true before and after every loop iteration.

Data-structure invariant:
: A fact that is true for every valid state of a structure.

State-machine invariant:
: A fact that is true across allowed state transitions.

Representation invariant:
: A fact that must hold because of the chosen data representation.

Ownership invariant:
: A fact about who owns memory, handles, or objects at each point.

## Correctness Shape

For many beginner algorithms, a correctness argument has three parts:

1. Initialization: the invariant is true before the loop or operation starts.
2. Preservation: each step keeps the invariant true.
3. Termination: when the algorithm stops, the invariant plus the stopping condition implies the required result.

This is not academic ceremony. It catches real bugs such as off-by-one errors, forgotten updates, and invalid empty-state handling.

## Example: Minimum Reading

Problem:

> Find the minimum valid reading in a non-empty array.

Preconditions:

- `readings != NULL`
- `count > 0`
- every reading is in the valid range

Postcondition:

- output is one of the readings
- output is less than or equal to every reading

Loop invariant after processing index `i`:

- `min_value` is the minimum of `readings[0]` through `readings[i]`

Why it works:

- Initialization: before the loop, `min_value = readings[0]`, so it is the minimum of the first processed element.
- Preservation: for each next reading, either the current minimum remains smaller, or the new reading becomes the minimum.
- Termination: after the final element, `min_value` is the minimum of the entire array.

## Data-Structure Invariant Example

For a fixed-size history buffer:

- `count <= capacity`
- `next < capacity` when `capacity > 0`
- the number of valid readings is exactly `count`
- if `count < capacity`, not every slot has been initialized
- if `count == capacity`, adding one reading overwrites the oldest reading

These rules are more valuable than a diagram alone. They tell you what every insertion, reset, and query must preserve.

## Programming Examples

### C: Invariant Checks For A Bounded History

This example stores recent readings in a fixed-size circular history. The invariant check is small enough to use in tests or debug builds.

```c
#include <stddef.h>

enum {
    HISTORY_CAPACITY = 8
};

struct reading_history {
    int values[HISTORY_CAPACITY];
    size_t count;
    size_t next;
};

static int history_invariant_holds(const struct reading_history *history)
{
    if (history == NULL)
        return 0;
    if (history->count > HISTORY_CAPACITY)
        return 0;
    if (history->next >= HISTORY_CAPACITY)
        return 0;
    return 1;
}

void history_init(struct reading_history *history)
{
    if (history == NULL)
        return;

    history->count = 0;
    history->next = 0;
}

int history_push(struct reading_history *history, int value)
{
    if (!history_invariant_holds(history))
        return -1;

    history->values[history->next] = value;
    history->next = (history->next + 1) % HISTORY_CAPACITY;

    if (history->count < HISTORY_CAPACITY)
        history->count++;

    return history_invariant_holds(history) ? 0 : -1;
}
```

The invariant is checked before and after mutation. Production code may compile out some checks, but the design should still say what must remain true.

### C: Loop Correctness For Minimum

```c
#include <stddef.h>

int min_reading(const int *readings, size_t count, int *out_min)
{
    int min_value;

    if (readings == NULL || out_min == NULL || count == 0)
        return -1;

    min_value = readings[0];

    for (size_t i = 1; i < count; i++) {
        if (readings[i] < min_value)
            min_value = readings[i];
    }

    *out_min = min_value;
    return 0;
}
```

The loop invariant is: after processing elements `0..i`, `min_value` is the smallest processed value.

### Python: Reference Check For Invariants

Python can help generate simple checks against the C behavior.

```python
class ReadingHistory:
    def __init__(self, capacity):
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self.capacity = capacity
        self.values = [None] * capacity
        self.count = 0
        self.next = 0

    def invariant_holds(self):
        return (
            0 <= self.count <= self.capacity
            and 0 <= self.next < self.capacity
        )

    def push(self, value):
        assert self.invariant_holds()
        self.values[self.next] = value
        self.next = (self.next + 1) % self.capacity
        self.count = min(self.count + 1, self.capacity)
        assert self.invariant_holds()
```

This is useful as a reference model, but the C code still needs explicit checks because it runs in the constrained target environment.

## Assertions Vs Error Handling

Assertions are for catching programmer mistakes during development. Error handling is for conditions the caller or environment can legitimately cause.

Good assertion candidates:

- internal invariants after mutation
- impossible state transitions
- assumptions guaranteed by a private helper

Good error-handling candidates:

- null pointers at public API boundaries
- empty input
- out-of-range data from a device, file, or network
- capacity exhaustion
- allocation failure

Do not use assertions as the only defense against external input.

## Counterexamples

A counterexample is an input or state that breaks a claimed algorithm property.

Claim:

> This function finds the minimum reading.

Counterexamples to test:

- empty array
- null pointer
- first element is minimum
- last element is minimum
- all elements equal
- negative values mixed with positive values

Counterexamples are efficient. One good counterexample is often enough to reveal that a precondition, invariant, or implementation is wrong.

## Common Mistakes

- Writing the invariant after the code instead of before or during design.
- Checking only final output and not intermediate state.
- Forgetting that an empty structure has invariants too.
- Mutating two related fields but checking only one of them.
- Using assertions for recoverable external failures.
- Optimizing code in a way that silently breaks a representation invariant.

## Embedded And Systems Angle

- use invariants to protect state machines, queues, pools, and ownership rules
- prefer checks that fail close to the broken assumption
- distinguish debug assertions from production error handling
- make invariants cheap enough to test repeatedly for small structures

## Related Topics

- [Problem Modeling](problem-modeling.md)
- [Loop Invariants And Termination](../control-flow-and-recursion/loop-invariants-and-termination.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
