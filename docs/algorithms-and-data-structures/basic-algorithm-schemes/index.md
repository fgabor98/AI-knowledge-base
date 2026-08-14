---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Basic Algorithm Schemes

Basic algorithm schemes are small reusable patterns: scan, count, sum, select, filter, search, and exploit sorted order. They are not glamorous, but they are the building blocks of larger algorithms.

Most real code contains more of these patterns than famous algorithms. A driver validates descriptors. A daemon filters events. A parser counts tokens. A diagnostic tool searches logs. A boot component scans a table. Getting these schemes right pays off everywhere.

## Learning Goals

After this section, you should be able to:

- recognize common one-pass scan patterns
- state the invariant behind counting, summation, min, max, filtering, and selection
- write C implementations with explicit bounds and failure behavior
- choose between linear search, ordered linear search, and binary search
- explain when simple O(n) schemes beat more complex structures
- handle empty input, invalid data, full output buffers, and duplicate matches
- use Python as a compact reference model for expected behavior

## Scheme Catalog

Summation:
: Combine all values with addition or another associative operation.

Counting:
: Count how many values satisfy a predicate.

Minimum and maximum search:
: Track the best value seen so far.

Existence check:
: Stop as soon as any value satisfies a predicate.

Decision check:
: Return true or false according to whether the input satisfies a rule.

Selection:
: Choose one element according to a policy, such as first matching, best priority, or smallest valid value.

Filtering:
: Copy or emit only elements that satisfy a predicate.

Accumulation:
: Maintain derived state while scanning, such as count, sum, min, max, checksum, or flags.

Linear search:
: Scan every candidate until the target is found or input ends.

Ordered linear search:
: Scan sorted data and stop once the current value proves the target cannot appear later.

Binary search:
: Repeatedly halve a sorted range.

## Why Schemes Matter

Recognizing the scheme clarifies:

- what state is needed
- what the loop invariant should be
- when the loop can stop early
- whether empty input is valid
- what error status is needed
- whether the data model should change

For example, "find the first invalid descriptor" is a linear search with early exit. "Count invalid descriptors" is a full scan. These are different algorithms even if the predicate is the same.

## Scheme Selection

Use this starting point:

| Need | Good first scheme |
| --- | --- |
| Need one result from all elements | summation or accumulation |
| Need number of matches | counting |
| Need first match | linear search |
| Need any match | existence check with early exit |
| Need all matches copied out | filtering |
| Need lookup in small unsorted data | linear search |
| Need lookup in sorted data | binary search or ordered linear search |
| Need repeated membership checks | consider bitmap, hash table, or sorted representation |

The simplest correct scheme is often the best scheme for bounded systems workloads.

## Empty Input Policy

Define empty input explicitly.

Examples:

- sum of empty input may be `0`
- count of empty input is usually `0`
- minimum of empty input is an error
- first match in empty input is "not found"
- filtering empty input writes zero outputs

Do not let empty behavior fall out accidentally from uninitialized state.

## Failure And Output Policy

For C APIs, be explicit about output values on failure. Common choices:

- leave output untouched on failure
- write output only on success
- write partial count and return a specific partial status
- return the required capacity without writing all output

The examples in this section write result outputs only on success unless stated otherwise.

## Programming Examples

### C: Summarize Readings In One Pass

This example combines several schemes: count, sum, minimum, maximum, and invalid-data detection.

```c
#include <stddef.h>
#include <stdint.h>

enum reading_summary_status {
    READING_SUMMARY_OK = 0,
    READING_SUMMARY_ERR_NULL,
    READING_SUMMARY_ERR_EMPTY,
    READING_SUMMARY_ERR_RANGE
};

struct reading_summary {
    int min_value;
    int max_value;
    int average;
    size_t count;
};

enum reading_summary_status summarize_readings(const int *readings,
                                               size_t count,
                                               int min_allowed,
                                               int max_allowed,
                                               struct reading_summary *out)
{
    int min_value;
    int max_value;
    int64_t sum = 0;

    if (out == NULL)
        return READING_SUMMARY_ERR_NULL;
    if (readings == NULL && count > 0)
        return READING_SUMMARY_ERR_NULL;
    if (count == 0)
        return READING_SUMMARY_ERR_EMPTY;
    if (min_allowed > max_allowed)
        return READING_SUMMARY_ERR_RANGE;

    min_value = readings[0];
    max_value = readings[0];

    for (size_t i = 0; i < count; i++) {
        int value = readings[i];

        if (value < min_allowed || value > max_allowed)
            return READING_SUMMARY_ERR_RANGE;

        if (value < min_value)
            min_value = value;
        if (value > max_value)
            max_value = value;
        sum += value;
    }

    out->min_value = min_value;
    out->max_value = max_value;
    out->average = (int)(sum / (int64_t)count);
    out->count = count;
    return READING_SUMMARY_OK;
}
```

Loop invariant:

> After processing `i` elements, `sum`, `min_value`, and `max_value` summarize the processed prefix.

### C: Filter With Caller-Owned Output

Filtering needs capacity policy. This implementation fails if the output buffer is too small and writes the completed count only on success.

```c
#include <stddef.h>

enum filter_status {
    FILTER_OK = 0,
    FILTER_ERR_NULL,
    FILTER_ERR_CAPACITY
};

enum filter_status filter_positive(const int *input,
                                   size_t input_count,
                                   int *output,
                                   size_t output_capacity,
                                   size_t *out_count)
{
    size_t written = 0;

    if (out_count == NULL)
        return FILTER_ERR_NULL;
    if (input == NULL && input_count > 0)
        return FILTER_ERR_NULL;
    if (output == NULL && output_capacity > 0)
        return FILTER_ERR_NULL;

    for (size_t i = 0; i < input_count; i++) {
        if (input[i] <= 0)
            continue;
        if (written == output_capacity)
            return FILTER_ERR_CAPACITY;
        output[written++] = input[i];
    }

    *out_count = written;
    return FILTER_OK;
}
```

This is O(n) time and O(1) extra memory beyond the caller-owned output buffer.

### Python: Reference Summary And Filter

```python
def summarize_readings(readings, minimum, maximum):
    if readings is None:
        raise ValueError("readings must not be None")
    if not readings:
        raise ValueError("readings must not be empty")
    for value in readings:
        if value < minimum or value > maximum:
            raise ValueError(f"out of range: {value}")
    return {
        "min_value": min(readings),
        "max_value": max(readings),
        "average": int(sum(readings) / len(readings)),
        "count": len(readings),
    }


def filter_positive(values):
    return [value for value in values if value > 0]
```

Python makes expected behavior compact. The C implementation still needs explicit capacity and pointer policy.

## Correctness Review

For each scheme, ask:

- What does the state variable mean after processing a prefix?
- What is the empty input behavior?
- Can the loop stop early?
- Is the output valid only on success?
- Are duplicate matches handled deliberately?
- Is the predicate pure and deterministic?
- Are range checks and overflow checks needed?
- Is the input mutated?

## Embedded And Systems Angle

- use simple schemes as the default for small bounded inputs
- avoid clever algorithms when input sizes are known and tiny
- use sentinels only when mutation and bounds are safe
- make failure returns explicit for searches and selections
- prefer caller-owned output buffers for filtering and selection results

## Pages In This Section

- [Linear Scan Patterns](linear-scan-patterns.md)
- [Binary Search](binary-search.md)
- [Practical Sequence Patterns](practical-sequence-patterns.md)

## Related Topics

- [Control Flow And Recursion](../control-flow-and-recursion/index.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
