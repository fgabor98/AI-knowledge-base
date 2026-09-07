---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Unit, Integration, Sanitizer, And Fuzz Testing

## Test layers

Unit tests should make one decision or invariant fail clearly. Integration tests should cross a real boundary such as a file descriptor, socket, process, loader, or service unit. End-to-end tests should assert observable behavior and recovery, not implementation details.

Run AddressSanitizer and UndefinedBehaviorSanitizer regularly; use ThreadSanitizer where its platform and workload support it. Treat sanitizer findings as defects even when production has not reproduced them. Keep symbolized stack traces, compiler flags, and the exact input.

Fuzz parsers and state machines with structure-aware generators, bounded inputs, and persistent corpora. Define a useful oracle: no crash, no sanitizer report, no hang beyond a budget, and invariants preserved. Seed with real protocol samples only after removing secrets. Minimize and retain every reproducer.

Coverage is a guide to unexplored code, not a correctness proof. Add targeted cases for overflow, truncation, duplicate fields, unknown versions, cancellation, reconnects, resource limits, and power-loss recovery. Run tests under multiple optimization levels and architectures when ABI-sensitive code is involved.

## Build distinct instrumented configurations

For a project with `parser.c` and a standalone `parser_test.c`, these
illustrative commands show separate host test builds:

```sh
clang -std=c11 -g -O1 -fno-omit-frame-pointer -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    parser.c parser_test.c -o parser-asan
./parser-asan

clang -std=c11 -g -O1 -fno-omit-frame-pointer -pthread \
    -fsanitize=thread queue.c queue_test.c -o queue-tsan
./queue-tsan
```

These filenames describe a project fixture, not files shipped in this topic.
Instrument relevant dependencies too. ASan and TSan are separate configurations;
neither proves all code paths safe, and TSan does not establish device/DMA
ordering. Preserve the runtime report and exact build.
See [ASan](https://clang.llvm.org/docs/AddressSanitizer.html) and
[TSan](https://clang.llvm.org/docs/ThreadSanitizer.html).

## Fuzz the production parser boundary

A libFuzzer entry receives arbitrary bytes and length. Reset parser state for
each input, impose allocation/work limits, and call the same parser used by
production. Do not discard every input without a valid checksum before any
interesting logic is reached; include structured seeds or a format-aware
mutation strategy while retaining malformed-input coverage.

For streaming protocols, test that feeding a valid frame in every possible
two-part split yields the same result as feeding it all at once. Test two
coalesced frames, truncated EOF, maximum length, length overflow and unknown
required flags. Assert semantic properties: invalid input must not dispatch
a command, and consumed bytes must not exceed supplied bytes.

Keep minimized regressions from fuzzing in the ordinary test suite. Record seed,
corpus, toolchain and limits. Coverage helps choose the next test; a high
percentage does not prove timeout, authorization or recovery semantics.
See [libFuzzer](https://llvm.org/docs/LibFuzzer.html).

## Related topics

- [Stage 16 overview](index.md)
- [Evidence and failure classification](../diagnostics-debugging-and-performance/userspace-failure-taxonomy-and-evidence.md)
