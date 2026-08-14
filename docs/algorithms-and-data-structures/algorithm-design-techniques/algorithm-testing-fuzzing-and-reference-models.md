---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Algorithm Testing Fuzzing And Reference Models

Algorithm tests should check more than a few expected examples. A useful strategy combines exact examples, structural invariants, an independent reference model, malformed-input cases, and generated sequences that exercise limits.

The reference implementation does not need to be fast. Its value comes from being simple enough that it fails differently from the optimized implementation.

## Test Layers

### Example Tests

Use named examples for requirements that need a readable explanation:

- empty and one-element inputs
- duplicate keys and equal priorities
- already sorted and reverse-sorted input
- disconnected and single-vertex graphs
- full and empty queues
- minimum and maximum representable values

Example tests document expected policy, such as whether equal intervals may touch or whether an invalid length is rejected.

### Invariant Tests

An invariant checks a property that remains true across many inputs. Examples:

- a sorted output is nondecreasing and is a permutation of the input
- a heap root has the best priority and every parent dominates its children
- a BFS distance increases by at most one across every edge
- a disjoint-set union never reports two different roots for the same component
- a parser never consumes bytes beyond the declared frame

Invariants catch broad classes of defects without requiring one expected output for every generated input.

### Differential Tests

Run the optimized implementation and a simple oracle on the same input, then compare normalized results. Normalize details that are intentionally unspecified, such as the order of equal-priority items, before comparing.

Good oracles include:

- exhaustive search for small combinatorial inputs
- a straightforward O(n²) algorithm for a fast O(n log n) algorithm
- a dictionary or list model for a fixed-capacity queue
- a Python parser for a lower-level C parser
- a trusted library only when its semantics match the contract exactly

Do not reuse the same helper, comparator, or state transition in the oracle; shared bugs can make a differential test falsely pass.

## Properties By Algorithm Family

| Family | Useful property |
| --- | --- |
| sorting | output is ordered and preserves input multiplicities |
| binary search | returned index satisfies the predicate and boundary neighbors do not |
| shortest path | every reported predecessor edge exists and distances satisfy relaxation bounds |
| traversal | every reachable vertex is visited once and unreachable vertices are untouched |
| interval selection | selected intervals are compatible and objective matches the oracle |
| ring buffer | FIFO order is preserved until overwrite policy explicitly discards data |
| parser | accepted frames round-trip through an encoder; malformed frames fail safely |

## Boundary And Failure Cases

Exercise resource and control boundaries deliberately:

- count zero and count one
- capacity zero, capacity one, and capacity full
- integer addition at `MAX - operand`, exactly at the limit, and beyond it
- null pointers with zero and nonzero lengths
- duplicate and aliased buffers when aliasing is possible
- malformed indexes, impossible graph vertices, and cycles in predecessor chains
- allocation failure, timeout, cancellation, and retry exhaustion

A test that only uses valid mid-range values will not validate the error contract.

## Fuzzing A Bounded API

A fuzz target should make the input domain finite and the expected termination behavior explicit. For a byte-oriented parser, cap the generated input length. For a graph, cap vertices and edges. For a queue, generate an operation sequence with a bounded number of operations.

The target should:

1. decode the generated input without unchecked arithmetic
2. invoke the implementation
3. check memory safety and invariants
4. compare with a reference model where possible
5. record the seed and the exact bytes or operations

Fuzzing is not a substitute for a specification. It explores the contract you wrote, including any accidental acceptance of malformed data.

## Shrinking And Reproduction

When a generated case fails, reduce it while preserving the failure:

- remove operations from a sequence
- delete graph edges or vertices
- shorten byte strings and lengths
- reduce numeric values toward zero or boundary values
- remove duplicate records

Persist the minimized case in a regression corpus. A failure report should include the seed, generator version, build configuration, input bytes, and expected/actual result. A random failure that cannot be replayed is an unfinished test result.

## C: Invariant-Oriented Harness

```c
#include <assert.h>
#include <stddef.h>

static int nondecreasing(const int *values, size_t count)
{
    if (values == NULL && count > 0)
        return 0;
    for (size_t i = 1; i < count; i++) {
        if (values[i - 1] > values[i])
            return 0;
    }
    return 1;
}

static size_t count_value(const int *values, size_t count, int target)
{
    size_t result = 0;
    for (size_t i = 0; i < count; i++)
        if (values[i] == target)
            result++;
    return result;
}

static void assert_same_multiset(const int *before,
                                 const int *after,
                                 size_t count)
{
    for (size_t i = 0; i < count; i++) {
        assert(count_value(before, count, before[i]) ==
               count_value(after, count, before[i]));
    }
}

void check_sort_result(const int *before,
                       const int *after,
                       size_t count)
{
    assert(nondecreasing(after, count));
    assert_same_multiset(before, after, count);
}
```

The multiset helper is intentionally simple and O(n²), which is appropriate for a test oracle on small generated arrays. A test helper should not silently impose the production algorithm's assumptions.

## Python: Differential Test Skeleton

```python
import random


def reference_first_at_or_above(values, threshold):
    for index, value in enumerate(values):
        if value >= threshold:
            return index
    return None


def test_search_against_reference(search):
    generator = random.Random(0x5EED)
    for _ in range(500):
        values = sorted(generator.randrange(-8, 9) for _ in range(
            generator.randrange(0, 20)))
        threshold = generator.randrange(-10, 11)
        assert search(values, threshold) == \
            reference_first_at_or_above(values, threshold)
```

Use a local generator instead of global random state. That makes the test deterministic and prevents unrelated tests from changing the sequence. Add explicit examples for cases the generator may rarely produce.

## C Tooling And Sanitizers

For C algorithms, combine logical tests with tools that observe undefined behavior:

- AddressSanitizer for out-of-bounds and use-after-free
- UndefinedBehaviorSanitizer for invalid shifts, overflow categories, and other undefined operations
- integer or bounds instrumentation where available
- compiler warnings at a strict level
- fault injection for allocation, I/O, and queue-full paths

Sanitizers do not prove correctness. They complement invariants and differential checks by finding memory and language-level failures.

## Common Mistakes

- Testing only the optimized output with hand-picked examples.
- Comparing unspecified ordering as if it were part of the contract.
- Building the oracle from the same buggy helper as the implementation.
- Generating inputs too small to reach capacity, overflow, or recursion limits.
- Using a random seed that is not recorded on failure.
- Fuzzing a parser without a maximum input size or timeout.
- Treating sanitizer-clean execution as proof of algorithmic correctness.

## Embedded And Systems Angle

- fuzz bounded inputs and operation sequences so test runs have predictable cost
- retain minimized failures in the target's regression corpus
- test full, empty, timeout, cancellation, and allocation-failure states
- include byte order, alignment, and corrupted-length cases in parser tests
- run optimized C against a simple host-side reference model
- capture build flags and target configuration with reproducible failures

## Review Checklist

- What invariant is checked for every generated result?
- What independent oracle covers small cases?
- Are invalid inputs and resource limits generated deliberately?
- Can every failure be replayed from a seed and serialized input?
- Are ties and unspecified output order normalized correctly?
- Does the test suite exercise the maximum queue, table, stack, and frame sizes?

## Related Topics

- [Invariants And Correctness](../algorithmic-foundations/invariants-and-correctness.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [String And Protocol Parsing Algorithms](string-and-protocol-parsing-algorithms.md)
- [Sorting Fundamentals](../sorting-and-ordering/sorting-fundamentals.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
