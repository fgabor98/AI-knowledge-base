---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Algorithmic Foundations

Algorithms start before code. The important first step is to turn an informal task into a precise model: what data enters the algorithm, what result leaves it, which cases are valid, which cases are errors, what must always remain true, and which representation makes the operations simple.

This section is the base layer for the rest of Algorithms And Data Models. It is not about memorizing named algorithms. It is about learning how to design, explain, test, and review an algorithm before optimizing it.

## Learning Goals

After this section, you should be able to:

- restate a vague requirement as inputs, outputs, constraints, and edge cases
- separate a problem model from a particular implementation
- identify preconditions, postconditions, and invariants
- choose a data representation by operations and bounds
- explain why an algorithm is correct for the modeled problem
- build small C examples that expose bounds, error handling, and memory behavior
- use Python when useful as a compact reference model or test oracle

## Core Vocabulary

Problem statement:
: The human-level description of what must be solved.

Input:
: Data the algorithm receives. Inputs include values, buffers, lengths, configuration, state, and environmental constraints.

Output:
: Data the algorithm returns or mutates. Outputs include return values, status codes, output buffers, state changes, and diagnostics.

Precondition:
: Something that must be true before the algorithm runs. For example, a pointer is non-null or a sequence is sorted.

Postcondition:
: Something that must be true after the algorithm finishes. For example, an output count is within capacity or a result is the minimum valid reading.

Invariant:
: Something that remains true throughout execution or throughout the lifetime of a data structure.

Data model:
: The chosen representation of the problem data, including fields, relationships, ownership, bounds, and valid states.

Abstract data type:
: A named set of operations and behavior that can have more than one implementation.

## Foundation Workflow

Use this order before writing the final implementation:

1. Write the problem in one sentence.
2. Identify the input data and who owns it.
3. Identify the output data and how errors are reported.
4. List constraints: maximum size, timing, memory, ordering, valid ranges, and failure modes.
5. List edge cases: empty input, full output, malformed data, duplicates, overflow, and unavailable resources.
6. Define preconditions and postconditions.
7. Choose a data model that makes the required operations straightforward.
8. Write down the invariants that must remain true.
9. Sketch the algorithm in simple steps.
10. Test examples, counterexamples, and boundary cases.

For systems and embedded work, steps 4 through 8 are not optional. They decide whether the implementation can be reviewed, bounded, and debugged.

## Running Example

The examples in this section use a small repeated problem:

> Given a bounded sequence of sensor readings, compute useful summary information or maintain recent readings under explicit constraints.

This example is intentionally ordinary. It exposes the foundational issues without requiring advanced algorithms:

- readings may be missing or invalid
- input length may be zero
- values may have a documented range
- sums may overflow if the type is too small
- storage may be fixed-size
- the caller needs explicit success or failure status

## From Requirement To Contract

Informal requirement:

> Compute the average temperature from recent readings.

Better algorithm contract:

- Input: array of signed integer readings in tenths of a degree Celsius.
- Input length: `0 <= n <= max_readings`.
- Valid reading range: `-400 <= reading <= 1250`.
- Output: average in tenths of a degree Celsius, rounded toward zero.
- Error cases: null output pointer, null input with nonzero length, empty input, invalid reading, sum overflow.
- Does not allocate memory.
- Does not mutate the input array.
- Runtime is O(n).
- Additional memory is O(1).

That contract is longer than the requirement, but it is much easier to implement and review.

## Algorithm Design Vs Implementation

Algorithm design answers:

- what problem is being solved
- what assumptions are allowed
- what result is required
- which data model makes the work simple
- why the steps are correct

Implementation answers:

- which language constructs and types are used
- how errors are represented
- how memory is allocated or avoided
- how overflow, bounds, and invalid input are handled
- how the implementation is tested

Treating these as separate questions prevents a common failure mode: writing code that is locally plausible but solving an underspecified problem.

## Programming Examples

### C: Contract-First Average

This small example is deliberately explicit about its contract. It shows how foundational design decisions appear in C as types, status codes, range checks, and output parameters.

```c
#include <stddef.h>
#include <stdint.h>

enum avg_status {
    AVG_OK = 0,
    AVG_ERR_NULL,
    AVG_ERR_EMPTY,
    AVG_ERR_RANGE,
    AVG_ERR_OVERFLOW
};

enum {
    TEMP_MIN_TENTHS_C = -400,
    TEMP_MAX_TENTHS_C = 1250
};

static int add_would_overflow_i64(int64_t a, int64_t b)
{
    if (b > 0 && a > INT64_MAX - b)
        return 1;
    if (b < 0 && a < INT64_MIN - b)
        return 1;
    return 0;
}

enum avg_status average_temp_tenths_c(const int *readings,
                                      size_t count,
                                      int *out_avg)
{
    int64_t sum = 0;

    if (out_avg == NULL)
        return AVG_ERR_NULL;
    if (count == 0)
        return AVG_ERR_EMPTY;
    if (readings == NULL)
        return AVG_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        int value = readings[i];

        if (value < TEMP_MIN_TENTHS_C || value > TEMP_MAX_TENTHS_C)
            return AVG_ERR_RANGE;
        if (add_would_overflow_i64(sum, value))
            return AVG_ERR_OVERFLOW;

        sum += value;
    }

    *out_avg = (int)(sum / (int64_t)count);
    return AVG_OK;
}
```

The algorithm is simple: scan once, validate each reading, accumulate the sum, divide by the count. The important foundation work is not the loop itself. It is the explicit handling of empty input, null pointers, valid ranges, overflow, and output ownership.

### Python: Reference Model

Python is useful here as a compact reference model for test generation. It should not replace the C implementation for embedded code, but it can make expected behavior easy to state.

```python
TEMP_MIN_TENTHS_C = -400
TEMP_MAX_TENTHS_C = 1250


def average_temp_tenths_c(readings):
    if readings is None:
        raise ValueError("readings must not be None")
    if len(readings) == 0:
        raise ValueError("at least one reading is required")

    for value in readings:
        if value < TEMP_MIN_TENTHS_C or value > TEMP_MAX_TENTHS_C:
            raise ValueError(f"reading out of range: {value}")

    return int(sum(readings) / len(readings))
```

For later testing, Python can generate expected outputs for valid cases and expected failures for invalid cases.

## Worked Review Checklist

When reviewing an algorithm at this stage, ask:

- Is the input model complete?
- Is the output model complete?
- Are invalid inputs defined?
- Are empty, full, minimum, maximum, duplicate, and overflow cases covered?
- Are memory ownership and mutation rules clear?
- Are runtime and memory bounds stated?
- Are invariants visible enough to test or assert?
- Could changing the data model make the algorithm simpler?

## Embedded And Systems Angle

- make failure cases explicit before implementation
- record assumptions about size, timing, memory, and ordering
- choose data models that match hardware, protocol, or kernel API constraints
- keep invariants visible in C structs, C++ types, and tests
- prefer fixed bounds and clear status codes when allocation or exceptions are not appropriate

## Pages In This Section

- [Problem Modeling](problem-modeling.md)
- [Invariants And Correctness](invariants-and-correctness.md)
- [Data Modeling And Abstract Data Types](data-modeling-and-abstract-data-types.md)

## Related Topics

- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
- [Systems And Embedded Architecture](../../systems-and-embedded-architecture/index.md)
