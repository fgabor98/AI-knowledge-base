# Unit, Integration, Sanitizer, And Fuzz Testing

## Test layers

Unit tests should make one decision or invariant fail clearly. Integration tests should cross a real boundary such as a file descriptor, socket, process, loader, or service unit. End-to-end tests should assert observable behavior and recovery, not implementation details.

Run AddressSanitizer and UndefinedBehaviorSanitizer regularly; use ThreadSanitizer where its platform and workload support it. Treat sanitizer findings as defects even when production has not reproduced them. Keep symbolized stack traces, compiler flags, and the exact input.

Fuzz parsers and state machines with structure-aware generators, bounded inputs, and persistent corpora. Define a useful oracle: no crash, no sanitizer report, no hang beyond a budget, and invariants preserved. Seed with real protocol samples only after removing secrets. Minimize and retain every reproducer.

Coverage is a guide to unexplored code, not a correctness proof. Add targeted cases for overflow, truncation, duplicate fields, unknown versions, cancellation, reconnects, resource limits, and power-loss recovery. Run tests under multiple optimization levels and architectures when ABI-sensitive code is involved.
