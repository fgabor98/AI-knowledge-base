---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Capstone: Bounded Protocol Parser

Build a streaming binary protocol parser that can run on a small MCU without dynamic
allocation and can be fuzzed on a host. The project demonstrates that a parser is a
state machine with resource budgets, not a collection of casts and length checks.

## Project Brief

Define a framed telemetry/command protocol with:

- a magic or delimiter strategy;
- version and message type;
- flags/reserved bits;
- sequence number;
- payload length;
- payload fields or nested TLV;
- integrity check;
- explicit acknowledgment/error messages.

The parser shall accept input in arbitrary fragments, reject malformed or unsupported
frames, avoid out-of-bounds access and unbounded work, and expose only validated
messages to application code. It shall not allocate from the heap in the core path.

Choose a maximum frame size, nesting depth, resynchronization scan, and processing time.
If a message can be larger than the input buffer, define whether the parser streams it,
copies it into bounded storage, or rejects it.

## Wire Specification

Write the format byte by byte. One possible format is:

| Field | Width | Encoding | Rule |
| --- | ---: | --- | --- |
| Magic | 2 | big-endian | fixed value; used for resynchronization |
| Version | 1 | unsigned | supported range |
| Type | 1 | unsigned | known message or reject/skip policy |
| Flags | 1 | bit mask | reserved bits must be zero |
| Sequence | 4 | big-endian | duplicate/replay policy |
| Payload length | 2 | big-endian | bounded by `MAX_PAYLOAD` |
| Payload | variable | message-specific | nested lengths validated |
| CRC | 4 | defined byte order | covers header and payload |

Specify whether the CRC includes magic, whether frames may be concatenated, how a
partial frame is retained, and whether a bad frame causes immediate reset or scanning
for the next valid magic. Define maximum total bytes, not only maximum payload.

## Parser API

Design an API that makes partial progress and ownership explicit:

```c
#include <stddef.h>
#include <stdint.h>

enum parser_status {
    PARSER_NEED_MORE = 0,
    PARSER_MESSAGE_READY = 1,
    PARSER_BAD_FRAME = 2,
    PARSER_UNSUPPORTED = 3,
    PARSER_LIMIT = 4,
    PARSER_INTERNAL_ERROR = 5
};

struct parser_message {
    uint8_t type;
    uint8_t version;
    uint32_t sequence;
    const unsigned char *payload;
    size_t payload_length;
};

enum parser_status parser_feed(const unsigned char *data,
                               size_t length,
                               struct parser_message *message);
```

The final implementation should use project-approved fixed-width types and a parser
context; this sketch emphasizes the states. Decide whether `parser_feed` consumes all
input, leaves unconsumed bytes, reports one message per call, or can report multiple
messages through a callback. Document whether `payload` borrows the feed buffer or a
parser-owned storage region and when it becomes invalid.

## State Machine

Use explicit states such as:

1. `SEARCH_MAGIC_0`
2. `SEARCH_MAGIC_1`
3. `READ_FIXED_HEADER`
4. `VALIDATE_LENGTH`
5. `READ_PAYLOAD`
6. `READ_CRC`
7. `VALIDATE_CRC`
8. `DISPATCH`
9. `RESYNC` or `RESET`

For every byte and event, define the state transition, bytes consumed, buffer effect,
timeout action, and error status. A table or switch is acceptable; hidden transitions
in helper side effects are not. Make reset idempotent and ensure a malformed length
cannot move the parser into a state that waits forever for impossible bytes.

## Bounds And Arithmetic

For each read:

- verify the parser buffer contains the requested bytes;
- decode the value into a fixed-width unsigned type;
- check it against protocol and implementation maxima;
- check `header + payload + crc` without overflow;
- check nested offsets/counts relative to their containing length;
- cap loops, recursion, and resynchronization scans.

Test values at zero, one, maximum, maximum plus one, and the largest representable
integer that can reach the calculation. Never rely on a 16-bit wire length fitting in a
host `int`; the host may have a smaller `int` or the subsequent multiplication may not
fit.

## No-Allocation Architecture

Provide one of these bounded strategies:

- parser-owned fixed frame storage;
- caller-owned scratch storage supplied at initialization;
- a streaming callback for payload chunks;
- zero-copy views into a transport buffer with an explicit lifetime;
- per-message fixed records from a pool.

If the parser stores a frame, size the buffer for the complete header/payload/CRC and
reject larger frames before writing. If it streams payload, do not dispatch an action
until integrity and semantic validation complete, unless the protocol explicitly allows
incremental side effects and has rollback semantics.

## Malformed Input Policy

Create a malformed-input matrix:

- bad magic and noise before magic;
- truncated fixed header, payload, or CRC;
- unsupported version/type;
- reserved flags set;
- length too large or inconsistent with nested content;
- duplicate/unknown/critical TLV;
- bad checksum;
- invalid enum/range/sequence;
- valid frame followed immediately by another frame;
- input that never terminates a frame;
- repeated bad frames intended to exhaust CPU or logs.

For each case, define whether input is retained, discarded, resynchronized, counted,
logged, or escalated. Rate-limit diagnostics and make recovery bounded.

## Version Compatibility

Support at least one older or alternate version in the test model. Define whether:

- newer optional fields can be skipped;
- unknown message types are ignored or rejected;
- reserved bits are required to be zero;
- a receiver can negotiate capabilities;
- sequence numbers detect duplicates or replay;
- error messages are versioned;
- a frame can be forwarded without understanding its payload.

Do not treat a version field as permission to accept unknown semantics. Validate the
version before dispatch and test downgrade/unsupported behavior explicitly.

## Reference Model

Write a simple host reference encoder/decoder with clarity prioritized over speed. Use
it to:

- generate valid frames;
- verify round trips for valid messages;
- compare field values and CRCs;
- create boundary and malformed frames;
- serve as an oracle for the target parser.

Keep the reference independent enough that it does not reproduce the target parser's
same bug. An intentionally simple byte-at-a-time implementation is useful for this
purpose.

## Fuzzing Plan

Build a fuzz target that feeds arbitrary byte chunks and reset/timeout events to the
parser. The target must not perform hardware I/O, allocate without a strict bound, or
dispatch irreversible commands. Use ASan, UBSan, integer/implicit-conversion warnings,
coverage, and leak detection where supported.

Seed the corpus with:

- minimal valid frame for every message type/version;
- maximum valid frame;
- every truncation point;
- each malformed-input matrix row;
- frames split at every header/payload/CRC boundary;
- concatenated frames and noise/resynchronization cases.

When a failure is found, minimize the input, add it as a permanent regression seed,
record the root cause and violated invariant, and check that the fix does not weaken
the limit or recovery policy.

## Integration With A Transport

The transport adapter should own reads, timeouts, DMA buffers, and link reset. The parser
should receive bytes/events through a narrow interface. Test the adapter separately for
short reads, timeout, disconnect, buffer reuse, and error mapping.

For UART/DMA, define ownership while the parser borrows a buffer and when cache
maintenance occurs. For a socket/file, handle `EINTR`, short reads, EOF, and cancellation.
For an RTOS queue, bound message and queue sizes and define backpressure.

## Security And Side Effects

Treat every field as untrusted until it passes syntax, range, integrity, authorization,
and state validation. CRC is not authentication. If commands control hardware, separate
parse from authorize/execute and make replay, privilege, and rate limits explicit.

Do not log attacker-controlled strings with unbounded formatting. Avoid exposing raw
buffer pointers in error paths. Make parser resource limits part of the threat model:
CPU exhaustion, log flooding, memory exhaustion, state desynchronization, and replay.

## Milestones

1. Wire specification and malformed-input matrix.
2. Fixed-buffer reader and unit tests for endian/length arithmetic.
3. Fragment-aware state machine with valid-frame tests.
4. Nested payload/semantic validation and version policy.
5. Reference model, fuzz harness, sanitizer/coverage CI.
6. Transport adapter and target integration.
7. Timing/resource/security report and release-quality documentation.

## Assessment Rubric

- **Wire correctness:** format, endian, CRC, version, and field rules are unambiguous.
- **Memory safety:** no out-of-bounds, overflow, lifetime, alignment, or unbounded
  allocation/recursion behavior.
- **Streaming behavior:** arbitrary fragmentation, concatenation, timeout, and noise
  are handled according to the specification.
- **Resource bounds:** maximum bytes, states, CPU, storage, logs, and retries are known
  and tested.
- **Compatibility:** old/new/unknown versions and fields have explicit policy.
- **Evidence:** reference model, negative tests, fuzzing, sanitizers, and target timing
  reports are reproducible.
- **Separation:** parsing cannot accidentally trigger unauthorized irreversible action.

## Common Mistakes

- Casting a byte buffer to a packed struct and trusting its layout.
- Validating payload length after copying or allocating from it.
- Treating one transport read as one frame.
- Letting a malformed frame wait forever or scan without a work limit.
- Dispatching a command before checksum, semantic, and authorization checks.
- Treating CRC as authenticity or ignoring sequence/replay policy.
- Fuzzing only complete frames and not fragmentation, reset, and timeout events.
- Logging every malformed byte without rate limits.

## Related Topics

- [Professional Practice And Capstones overview](./index.md)
- [Protocols And Serialization](../advanced-c/protocols-and-serialization.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Security](../correctness-quality-and-security/security.md)
- [Testing Strategy](../correctness-quality-and-security/testing-strategy.md)
- [Advanced Data Structures](../advanced-c/advanced-data-structures.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [LLVM libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html)
- [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [IETF RFC 9293: Transmission Control Protocol](https://www.rfc-editor.org/rfc/rfc9293)
- The project wire specification, transport API, threat model, resource budget, and
  compatibility test matrix
