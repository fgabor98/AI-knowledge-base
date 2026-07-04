---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Problem Modeling

Problem modeling is the act of converting an informal task into a precise algorithm contract. A good model says what data exists, what result is required, what can go wrong, and which constraints must be respected.

Skipping this step makes code harder to review because the reviewer must infer the problem from implementation details. In systems code, that is a serious risk: unclear bounds become buffer bugs, unclear ownership becomes lifetime bugs, and unclear error policy becomes unreliable behavior.

## What A Problem Model Contains

A useful problem model answers these questions:

- What is the input?
- What is the output?
- Who owns each buffer or object?
- Which inputs are valid?
- Which inputs are invalid but expected?
- What are the minimum and maximum sizes?
- What are the valid value ranges?
- What must happen on failure?
- Is the input mutated?
- Is allocation allowed?
- What are the runtime and memory bounds?

The answer does not need to be verbose, but it must be precise enough that two people would implement compatible behavior.

## Inputs And Outputs

Inputs are more than function parameters. Inputs include:

- arrays, lengths, indexes, and flags
- current state stored in a structure
- configuration values
- hardware or protocol limits
- sortedness, uniqueness, or alignment assumptions
- ownership and lifetime assumptions

Outputs are also more than return values. Outputs include:

- status codes
- output buffers
- mutated state
- counters and diagnostic values
- emitted events
- selected indexes or handles

For C APIs, a common pattern is to return a status code and write the actual result through an output pointer. That lets the caller distinguish "the result is zero" from "the algorithm failed."

## Preconditions And Postconditions

A precondition is required before the algorithm runs.

Examples:

- `readings != NULL` when `count > 0`
- `out_avg != NULL`
- `capacity > 0`
- input values are in a documented range
- an array is already sorted

A postcondition is guaranteed after the algorithm returns successfully.

Examples:

- `*out_avg` contains the average of all valid readings
- `history->count <= history->capacity`
- output indexes are less than the input length
- the input array was not modified

Failure postconditions matter too. If the function returns an error, say which outputs are unchanged, which may be partial, and which are invalid.

## Edge Cases

Edge cases are not rare cases. They are boundary cases that define the shape of the algorithm.

For a bounded sequence of readings, check:

- no readings
- exactly one reading
- maximum allowed count
- minimum valid value
- maximum valid value
- value just below the minimum
- value just above the maximum
- sum near the integer limit
- null input pointer
- null output pointer

Writing these cases before implementation often reveals missing policy. For example, "average of no readings" is not a mathematical implementation detail. It is a product or API decision.

## Example Model: Average Of Sensor Readings

Informal request:

> Compute the average of a set of recent temperature readings.

Problem model:

- Readings are signed integers in tenths of a degree Celsius.
- Valid readings are from `-400` to `1250`, inclusive.
- The caller provides an array and count.
- `count` must be greater than zero for a successful average.
- The algorithm scans exactly `count` elements.
- The input array is read-only.
- The function does not allocate memory.
- The function reports invalid input with a status code.
- The average is rounded toward zero.
- Runtime is O(n), where `n` is `count`.
- Additional memory is O(1).

This model is enough to write tests and implementation without guessing.

## Programming Examples

### C: Model As API Contract

The following C code expresses the problem model directly in the function boundary.

```c
#include <stddef.h>
#include <stdint.h>

enum reading_status {
    READING_OK = 0,
    READING_ERR_NULL,
    READING_ERR_EMPTY,
    READING_ERR_RANGE
};

struct reading_bounds {
    int min_tenths_c;
    int max_tenths_c;
};

static int reading_in_bounds(int value, struct reading_bounds bounds)
{
    return value >= bounds.min_tenths_c && value <= bounds.max_tenths_c;
}

enum reading_status average_readings(const int *readings,
                                     size_t count,
                                     struct reading_bounds bounds,
                                     int *out_avg)
{
    int64_t sum = 0;

    if (out_avg == NULL)
        return READING_ERR_NULL;
    if (count == 0)
        return READING_ERR_EMPTY;
    if (readings == NULL)
        return READING_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        if (!reading_in_bounds(readings[i], bounds))
            return READING_ERR_RANGE;
        sum += readings[i];
    }

    *out_avg = (int)(sum / (int64_t)count);
    return READING_OK;
}
```

Notice what is visible:

- the input pointer is `const`
- the output is explicit
- the valid range is passed as data
- errors are part of the API
- no allocation is needed

The implementation is short because the problem model is clear.

### Python: Test Case Generator

Python can represent the same model compactly and is useful for generating expected results.

```python
def average_readings(readings, minimum=-400, maximum=1250):
    if readings is None:
        raise ValueError("readings must not be None")
    if not readings:
        raise ValueError("readings must not be empty")
    for value in readings:
        if value < minimum or value > maximum:
            raise ValueError(f"out of range: {value}")
    return int(sum(readings) / len(readings))


TEST_CASES = [
    [200],
    [-400, 0, 1250],
    [199, 200, 201],
    [0, 1],
]

for case in TEST_CASES:
    print(case, average_readings(case))
```

Use this style when Python makes expected behavior easier to audit. Do not use it to hide constraints that the C implementation must still handle explicitly.

## Common Modeling Mistakes

- Treating a vague requirement as enough to implement from.
- Forgetting to model invalid input.
- Using `0`, `NULL`, or `-1` for multiple meanings.
- Saying "array" without saying who owns it and how long it is.
- Assuming allocation is acceptable without checking the target context.
- Assuming sortedness, uniqueness, or bounds without documenting them.
- Testing only ordinary cases and not boundary cases.

## Modeling Checklist

Before implementing, write down:

- input fields and valid ranges
- output fields and failure behavior
- ownership and mutation rules
- maximum sizes and memory policy
- expected ordering, uniqueness, or alignment
- required runtime behavior
- edge cases and test cases

## Embedded And Systems Angle

- make resource limits explicit before choosing an algorithm
- separate hardware facts from software policy
- treat invalid input and partial data as normal cases to model
- prefer contracts that can be enforced with simple checks near API boundaries

## Related Topics

- [Algorithmic Foundations](index.md)
- [Invariants And Correctness](invariants-and-correctness.md)
- [Data Modeling And Abstract Data Types](data-modeling-and-abstract-data-types.md)
