---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Protocols And Serialization

Protocol code turns bytes from another time, machine, process, or trust domain into C
objects and decisions. The input may be fragmented, truncated, duplicated, reordered,
malicious, or produced by a different compiler and endian order. A robust parser does
not cast a buffer to a structure and hope that the producer made the same assumptions.
It validates length and state before every read, decodes fields explicitly, and keeps
ownership and version policy visible.

## Learning Objectives

- Design binary formats with explicit widths, endian order, alignment, framing, and
  version rules.
- Parse fragmented and malformed input without out-of-bounds access or unbounded work.
- Choose between copying, views, streaming parsers, and zero-copy representations.
- Add checksums, sequence numbers, authentication boundaries, and recovery behavior.
- Build protocol tests, fuzz targets, reference models, and compatibility matrices.

## Define The Wire Contract

For every field, specify:

- offset or encoding order;
- width and signedness;
- endian order or byte encoding;
- valid range and reserved values;
- alignment requirement, if any;
- presence/length relationship;
- ownership and lifetime of referenced data;
- error behavior and recovery point;
- version and backward/forward compatibility.

Also specify what is not on the wire. C padding, pointer values, enum representation,
`long`, bit-field layout, floating-point representation, and native structure alignment
must not leak into a portable protocol unless the protocol explicitly fixes them and
all participants enforce the same ABI.

## Explicit Endian Conversion

Use byte operations or well-defined conversion functions. A byte sequence such as
`12 34` means `0x1234` only after the protocol says it is big-endian. Do not rely on a
host's endian order, and do not use a packed structure as a substitute for decoding.

```c
#include <stdint.h>

static uint16_t read_be16(const unsigned char bytes[2])
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | (uint16_t)bytes[1]);
}

static void write_be32(unsigned char bytes[4], uint32_t value)
{
    bytes[0] = (unsigned char)(value >> 24);
    bytes[1] = (unsigned char)(value >> 16);
    bytes[2] = (unsigned char)(value >> 8);
    bytes[3] = (unsigned char)value;
}
```

The cast before shifting matters: it makes the intended unsigned width explicit. For
large integer encodings, use a loop and check the destination length. For signed wire
values, decode an unsigned representation and apply the protocol's range/sign rule
before converting to a C signed type.

## Length Validation And Integer Overflow

Length fields are untrusted. Before adding a length to an offset, check that the offset
is within the input and that the addition cannot overflow. Before multiplying a count
by an element size, check the multiplication. Validate a declared length against both
the remaining bytes and the product's configured maximum.

A safe parser often follows this order:

1. confirm enough bytes for the fixed prefix;
2. decode the length using an explicit endian rule;
3. reject values above a protocol and implementation maximum;
4. confirm the complete field/frame is present;
5. only then create a view or allocate storage;
6. validate nested counts and offsets relative to the containing object.

Never let an attacker choose an allocation size, loop count, recursion depth, or retry
count without a bound. A length that is numerically valid can still exceed the product's
memory or timing budget.

## A Bounds-Checked Reader

An advancing reader centralizes bounds checks and makes it difficult for one field
decoder to forget the remaining-length test:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct reader {
    const unsigned char *data;
    size_t length;
    size_t position;
};

static bool take(struct reader *reader, size_t count,
                 const unsigned char **result)
{
    if (reader == NULL || result == NULL || reader->position > reader->length ||
        count > reader->length - reader->position) {
        return false;
    }
    *result = reader->data + reader->position;
    reader->position += count;
    return true;
}

static bool reader_be16(struct reader *reader, uint16_t *result)
{
    const unsigned char *bytes;
    if (!take(reader, 2u, &bytes) || result == NULL) {
        return false;
    }
    *result = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    return true;
}

static bool reader_be32(struct reader *reader, uint32_t *result)
{
    const unsigned char *bytes;
    if (!take(reader, 4u, &bytes) || result == NULL) {
        return false;
    }
    *result = ((uint32_t)bytes[0] << 24) |
              ((uint32_t)bytes[1] << 16) |
              ((uint32_t)bytes[2] << 8) |
              (uint32_t)bytes[3];
    return true;
}
```

`take` returns a view valid only while the backing buffer remains unchanged and owned
by the parser. A production reader may add an error code, absolute offset, nesting
depth, or maximum operation count. Keep the reader's `position` monotonic and expose
it in diagnostics so a malformed packet can be explained precisely.

## Framing And Streaming

A byte stream does not preserve message boundaries. Framing strategies include:

- fixed-size frames;
- length-prefixed frames;
- delimiter-terminated frames with escaping or COBS-like encoding;
- bit/byte stuffing;
- transport-provided records;
- self-synchronizing formats with magic and sequence fields.

A streaming parser is a state machine, not a loop that assumes `read()` returns one
complete frame. It must handle partial prefix, partial payload, extra bytes, invalid
length, timeout, cancellation, noise, and resynchronization. Decide whether a bad
frame causes the parser to discard until a delimiter, scan for a validated magic, or
reset the link. Bound resynchronization work to avoid a denial-of-service path.

For a length-prefixed format, do not allocate the declared payload until the prefix has
been validated against the configured maximum. For a delimiter format, cap the frame
length even if no delimiter arrives.

## Checksums And Integrity

A checksum detects accidental corruption; it is not authentication. Choose a checksum
or CRC from the channel's error model and document polynomial, initial value, reflection,
final XOR, byte order, and coverage. Test known vectors and verify the implementation
against a small reference model.

Place the integrity field so that a receiver can reject corrupted lengths and metadata,
not only payload. Validate checksum before exposing a parsed command to an actuator or
state machine. For hostile links, add a cryptographic MAC/signature with key
management, nonce/replay rules, and constant-time verification as required; do not
rename a CRC to “secure checksum.”

## Versioning And Compatibility

A protocol evolves through added fields, changed constraints, new messages, and removed
features. Use an explicit version or capability negotiation. A robust extension usually
has a length-delimited message so an older reader can skip fields it does not know.

Document:

- minimum and maximum supported versions;
- whether unknown fields are ignored, rejected, or preserved;
- required versus optional fields;
- default values and whether zero is a valid value;
- feature negotiation and downgrade behavior;
- message and error compatibility;
- deprecation and migration timelines.

Never infer a version from structure size or compiler layout. If a structure is used as
an in-memory ABI, version it separately from the wire format and validate its `size`
field before accessing later members.

## No-Allocation Parsing

No-allocation parsing is valuable in ISRs, bootloaders, small MCUs, and failure paths.
Use a caller-owned output structure, bounded scratch arena, or views into an input
buffer. The output contract must say which fields are copied and which borrow input
storage.

Borrowed strings are not C strings unless the parser verified a terminator inside the
view. Represent a string as pointer plus length, or copy to a bounded destination with
explicit truncation/error behavior. Do not call an unbounded string function on a wire
field merely because it happened to contain a zero byte in a test.

## Zero-Copy Views

A zero-copy field can be represented as:

```c
struct byte_view {
    const unsigned char *data;
    size_t length;
};
```

Its invariants are that `data` points into a live input buffer when `length` is nonzero,
the range has passed bounds checks, and the owner will not modify/recycle the buffer
until all consumers release the view. Across DMA, processes, or processors, add cache,
address-space, and ownership rules. Across a language boundary, add ABI and lifetime
rules.

Copy small control fields when it makes lifetime easier to prove. Zero-copy is a
performance option, not a security property.

## Nested Formats And TLV

Type-length-value formats permit extension and skipping, but the parser must validate
that the value length stays within its containing message. Nested TLVs need a depth
limit and a total-work budget. Duplicate fields need a policy: first wins, last wins,
reject, or combine. Unknown critical fields should not be silently ignored.

For offset-based formats, validate that each offset is within the message, aligned if
required, and that offset plus length cannot overflow. Reject overlapping regions unless
overlap is explicitly part of the format.

## Error Taxonomy And Recovery

Separate errors that affect recovery:

- incomplete input: retain state and request more bytes;
- malformed syntax: discard or resynchronize;
- unsupported version/feature: negotiate, report, or reject;
- integrity failure: discard and record telemetry;
- semantic invalidity: report a validly framed but unacceptable command;
- resource exhaustion: apply backpressure or bounded rejection;
- transport failure: reset link/session state.

Do not collapse all failures into `-1` when the caller needs to decide whether to retry,
drop, reset, or alert. Error paths must be bounded and must not expose partially updated
application state.

## Fuzzing And Differential Testing

Protocol parsers are excellent fuzz targets. Define a harness that accepts arbitrary
bytes and guarantees:

- no crash, UB, leak, or unbounded memory/time;
- no external I/O or irreversible action;
- deterministic result for deterministic input and configuration;
- clean cleanup of all temporary state.

Use sanitizers, coverage guidance, mutation, corpus minimization, and structured
generators. Add seed cases for every field boundary, nesting limit, checksum mismatch,
fragmentation point, version, duplicate field, and integer overflow edge.

Differential testing compares a target parser with a simple reference decoder or a
second implementation. Property tests can assert that encoding then decoding preserves
valid values, while malformed bytes are rejected without side effects.

## Exercises And Diagnostics

1. Specify a versioned binary frame with magic, length, sequence, payload, and CRC;
   define every byte and error transition.
2. Implement a streaming parser that accepts one byte at a time and test every split
   point of valid and malformed frames.
3. Add nested TLV parsing with maximum depth, total-byte, duplicate, and unknown-field
   policies.
4. Build a zero-copy view API and a copied API; test buffer reuse, truncation, and
   lifetime after the input transport advances.
5. Fuzz the parser with sanitizers and add a regression seed for every discovered bug.

## Common Mistakes

- Casting wire bytes to a native structure and trusting padding, alignment, or endian.
- Adding a length to an offset before checking for overflow.
- Treating a stream read as a complete message.
- Calling `strlen` or another unbounded API on untrusted bytes.
- Using a CRC as authentication or validating integrity after side effects.
- Ignoring unknown fields without a critical/optional policy.
- Returning borrowed views after the input buffer has been recycled.
- Letting malformed input trigger unbounded allocation, recursion, scanning, or retries.

## Related Topics

- [Advanced C overview](./index.md)
- [Advanced Data Structures](./advanced-data-structures.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Security](../correctness-quality-and-security/security.md)
- [Protocols And Serialization Capstone](../professional-and-capstone/capstone-protocol-parser.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [IETF RFC 9293: Transmission Control Protocol](https://www.rfc-editor.org/rfc/rfc9293)
- [IETF RFC 1952: GZIP file format](https://www.rfc-editor.org/rfc/rfc1952)
- [American National Institute of Standards and Technology cryptographic standards](https://csrc.nist.gov/publications)
- The exact wire-format specification, transport framing contract, checksum/CRC
  parameters, security protocol, and compatibility test matrix
