---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# String And Protocol Parsing Algorithms

Parsing converts an untrusted sequence of bytes or characters into structured values. A parser is an algorithm with a state machine, a resource contract, and a failure policy. Correctness includes rejecting ambiguous or truncated input without reading beyond the available buffer.

## Identify The Input Model

Decide whether the input is:

- text with an encoding and character rules
- raw bytes with fixed-width fields
- a length-prefixed frame
- a delimiter-terminated record
- a stream containing multiple frames and partial reads

Do not use text assumptions for a binary protocol. A zero byte may be valid payload, and a byte with the high bit set may be part of a multi-byte encoding rather than a character.

## Tokenization

Tokenization separates an input into meaningful units. A tokenizer should define whitespace, delimiters, quoting, escaping, numeric grammar, and maximum token length. In-place tokenization can be efficient but changes the input and requires writable storage; slice-based tokens preserve the original buffer but require lifetime discipline.

For a bounded C tokenizer, represent a token as a pointer and length rather than returning a pointer to a temporary NUL-terminated string. A caller can copy it when a persistent value is needed.

## Finite-State Machines

An explicit state machine is often clearer than nested conditionals. Each state describes what the parser expects next and what data has already been accepted.

Useful parser states include:

```text
WAIT_SYNC       looking for a frame marker
READ_LENGTH     collecting a fixed-width length field
READ_PAYLOAD    collecting exactly length bytes
READ_CHECKSUM   validating trailing integrity data
EMIT_FRAME      delivering a complete frame
REJECT          reporting an invalid frame or resetting
```

The central invariant is:

> The parser state and counters describe exactly the bytes consumed so far; no unconsumed byte has been interpreted twice.

Every call should either consume at least one byte, complete a frame, reject input, or report that more input is needed. A parser that returns “need more data” without retaining why it needs it is difficult to resume safely.

## Framing Choices

### Length-Prefixed Frames

A length field allows payload bytes to contain any value. Validate the length before adding header size, allocating, or advancing a pointer. Reject lengths above the protocol maximum and check arithmetic such as `header + payload + trailer` for overflow.

### Delimiter-Based Frames

A delimiter is simple for human-readable records, but payload escaping or quoting must be defined. A maximum frame length is still required; otherwise a missing delimiter lets an attacker or faulty peer consume the entire stream buffer.

### Fixed-Length Frames

Fixed-length records are easy to bound and often suitable for device links. Versioning and optional fields still need explicit interpretation, especially when a newer sender transmits a larger record.

## Incremental Parsing

Reads from sockets, UARTs, files, and DMA descriptors can split any field. Keep parser state across calls instead of assuming one read contains one frame. A parser API commonly returns:

- `PARSER_FRAME` with a completed output
- `PARSER_NEED_MORE` when the current prefix is valid but incomplete
- `PARSER_REJECTED` for malformed input
- `PARSER_ERR_*` for API or resource failures

The caller must know whether rejected bytes were consumed and where scanning resumes. This is part of the interface, not an implementation detail.

## C: Incremental Length-Prefixed Parser

This example accepts a two-byte big-endian payload length, a one-byte type, and a bounded payload. The checksum is intentionally a simple byte sum; a real protocol should select a documented CRC or authenticated integrity mechanism as appropriate.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    PARSER_MAX_PAYLOAD = 64,
    PARSER_HEADER_BYTES = 3,
    PARSER_CHECKSUM_BYTES = 1
};

enum parser_status {
    PARSER_FRAME = 0,
    PARSER_NEED_MORE,
    PARSER_REJECTED,
    PARSER_ERR_NULL
};

struct parsed_frame {
    uint8_t type;
    uint8_t payload[PARSER_MAX_PAYLOAD];
    size_t payload_length;
};

struct frame_parser {
    uint8_t frame[PARSER_HEADER_BYTES + PARSER_MAX_PAYLOAD +
                  PARSER_CHECKSUM_BYTES];
    size_t used;
    size_t expected;
};

static void parser_reset(struct frame_parser *parser)
{
    parser->used = 0;
    parser->expected = 0;
}

void frame_parser_init(struct frame_parser *parser)
{
    if (parser != NULL)
        parser_reset(parser);
}

enum parser_status frame_parser_feed(struct frame_parser *parser,
                                     const uint8_t *input,
                                     size_t input_length,
                                     struct parsed_frame *out_frame,
                                     size_t *out_consumed)
{
    size_t consumed = 0;

    if (parser == NULL || out_frame == NULL || out_consumed == NULL)
        return PARSER_ERR_NULL;
    if (input == NULL && input_length > 0)
        return PARSER_ERR_NULL;

    while (consumed < input_length) {
        if (parser->used == sizeof(parser->frame)) {
            parser_reset(parser);
            *out_consumed = consumed;
            return PARSER_REJECTED;
        }

        parser->frame[parser->used++] = input[consumed++];
        if (parser->used == 2) {
            size_t payload_length =
                ((size_t)parser->frame[0] << 8) | parser->frame[1];

            if (payload_length > PARSER_MAX_PAYLOAD) {
                parser_reset(parser);
                *out_consumed = consumed;
                return PARSER_REJECTED;
            }
            parser->expected = PARSER_HEADER_BYTES + payload_length +
                               PARSER_CHECKSUM_BYTES;
        }

        if (parser->expected == 0 || parser->used < parser->expected)
            continue;

        {
            uint8_t checksum = 0;
            size_t payload_length = parser->expected -
                                    PARSER_HEADER_BYTES -
                                    PARSER_CHECKSUM_BYTES;

            for (size_t i = 0; i < parser->expected - 1; i++)
                checksum = (uint8_t)(checksum + parser->frame[i]);
            if (checksum != parser->frame[parser->expected - 1]) {
                parser_reset(parser);
                *out_consumed = consumed;
                return PARSER_REJECTED;
            }

            out_frame->type = parser->frame[2];
            out_frame->payload_length = payload_length;
            for (size_t i = 0; i < payload_length; i++)
                out_frame->payload[i] = parser->frame[3 + i];
            parser_reset(parser);
            *out_consumed = consumed;
            return PARSER_FRAME;
        }
    }

    *out_consumed = consumed;
    return PARSER_NEED_MORE;
}
```

The example consumes a complete frame before returning it and retains partial bytes in caller-owned storage. A stream with a sync marker needs an additional resynchronization policy after checksum failure; blindly treating every subsequent byte as a new length field may lose alignment.

## Endianness And Alignment

Decode multi-byte fields from bytes rather than casting an unaligned pointer:

```c
static uint32_t read_be32(const uint8_t bytes[4])
{
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) |
           (uint32_t)bytes[3];
}
```

This avoids alignment faults, host-endian dependence, and strict-aliasing violations. Similar helpers should define signed decoding, maximum values, and whether non-canonical encodings are accepted.

## Python: Reference Frame Parser

```python
def parse_frames(data, maximum=64):
    offset = 0
    frames = []
    while offset + 2 <= len(data):
        length = int.from_bytes(data[offset:offset + 2], "big")
        total = 3 + length + 1
        if length > maximum:
            raise ValueError("payload too large")
        if offset + total > len(data):
            break

        frame = data[offset:offset + total]
        if sum(frame[:-1]) & 0xFF != frame[-1]:
            raise ValueError("checksum mismatch")
        frames.append((frame[2], frame[3:3 + length]))
        offset += total
    return frames, data[offset:]
```

The returned remainder is the incomplete suffix. It lets a streaming caller append the next read without silently discarding a partial header or payload.

## Malformed Input And Resynchronization

Define what happens after:

- an impossible length
- an invalid type or version
- a checksum failure
- a missing delimiter
- an incomplete frame at timeout or end-of-stream

Possible policies are reset, scan for a sync marker, discard until delimiter, or close the connection. Resynchronization must be bounded; scanning forever for a marker is another unbounded algorithm.

## Common Mistakes

- Calling `strlen`, `strcpy`, or similar functions on untrusted non-terminated data.
- Adding a length to a pointer before checking the length and the addition.
- Casting a byte buffer to a packed struct and assuming alignment and endianness.
- Losing parser state when a read splits a multi-byte field.
- Accepting trailing bytes without a version or frame-boundary policy.
- Treating a checksum as authentication.
- Letting a missing delimiter grow a frame without a maximum.

## Embedded And Systems Angle

- use caller-owned buffers with explicit maximum frame and token sizes
- parse bytes with endian helpers and alignment-safe loads
- make progress and `need more` behavior observable
- bound resynchronization scans, retries, and timeout-driven partial state
- validate before copying into application structures
- keep integrity checks, authentication, and error recovery as separate concerns

## Review Checklist

- Is the accepted byte grammar written down?
- What state is retained across reads?
- Does every length check precede arithmetic, indexing, and copying?
- Can the parser reject malformed input without consuming unrelated frames?
- Are output ownership and lifetime explicit?
- Are endianness, alignment, encoding, and maximum lengths defined?

## Related Topics

- [Data Modeling And Abstract Data Types](../algorithmic-foundations/data-modeling-and-abstract-data-types.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Bitsets And Bitmaps](../data-structures-for-algorithms/bitsets-and-bitmaps.md)
- [Algorithm Testing Fuzzing And Reference Models](algorithm-testing-fuzzing-and-reference-models.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
