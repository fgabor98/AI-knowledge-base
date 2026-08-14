---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# String And Protocol Parsing Algorithms

Roadmap for scanning text and binary protocols as bounded state machines rather than ad hoc string manipulation.

## Coverage

- tokenization and delimiter scanning
- finite-state parsers
- length-delimited versus sentinel-terminated fields
- escaping and quoting
- incremental parsing across buffers
- framing, validation, and resynchronization
- prefix and substring search
- endianness and integer decoding
- malformed-input and truncation policy

## Core Model

A parser state should identify:

- the current phase
- bytes or tokens consumed
- partial field data and remaining capacity
- the next expected delimiter or length
- the error or recovery state

The parser must make progress on every call or return a status that explains why it needs more input.

## Programming Examples

- C: add a length-bounded tokenizer and an incremental frame parser.
- Python: use a clear reference parser and malformed-input generator.

## Embedded And Systems Angle

- never infer binary length from a missing terminator
- validate lengths before arithmetic or buffer access
- support partial input without losing parser state
- define resynchronization and corruption reporting
- keep protocol decoding separate from device ownership and I/O

## Future Material

- finite-state parser skeleton
- checked little- and big-endian decoding
- incremental parser across ring-buffer segments
- delimiter escaping and malformed-frame recovery

## Related Topics

- [Arrays Buffers And Records](../data-structures-for-algorithms/arrays-buffers-and-records.md)
- [Algorithm Testing Fuzzing And Reference Models](algorithm-testing-fuzzing-and-reference-models.md)
- [Cache-Aware And DMA-Friendly Layouts](../embedded-linux-algorithmic-constraints/cache-aware-and-dma-friendly-layouts.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
