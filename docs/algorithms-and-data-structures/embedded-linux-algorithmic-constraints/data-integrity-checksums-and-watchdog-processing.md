---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Data Integrity Checksums And Watchdog-Friendly Processing

Embedded algorithms often process data that can be corrupted, truncated, reordered, or interrupted by reset. They also run under watchdog and deadline constraints. Integrity checking and long-running work therefore need an explicit algorithmic contract: what is covered, when it is validated, how progress is measured, and what happens after failure.

## Checksum Versus CRC

A checksum combines input bytes into a smaller value, often by addition or one's-complement arithmetic. It is cheap and useful for detecting many accidental errors, but simple sums have blind spots: reordering two bytes or compensating one change with another can preserve the result.

A cyclic redundancy check (CRC) treats the message as a polynomial over a finite field. The selected polynomial, initial value, input reflection, output reflection, and final XOR are all part of the CRC definition. “CRC-32” without those parameters is incomplete.

Integrity codes detect accidental corruption; they do not authenticate an attacker. If an adversary can change both payload and checksum, use an authenticated construction with a key and a defined key-management policy.

## Coverage And Placement

Define exactly what the integrity value covers:

- payload only
- header and payload
- version and length fields plus payload
- a whole storage image excluding the integrity field itself
- each frame independently, or a stream-wide sequence

Validate framing and lengths before hashing or copying. A checksum over the wrong byte range can be perfectly implemented and still fail to protect the data model.

The integrity field's byte order must be documented. A CRC value computed correctly but serialized in host endianness is not a portable wire format.

## Incremental CRC

An incremental CRC allows a frame to be processed in chunks. The update function must preserve the exact algorithm state between chunks; resetting the CRC for every chunk produces per-chunk checks rather than one frame check.

### C: Bitwise CRC-32 Update

```c
#include <stddef.h>
#include <stdint.h>

uint32_t crc32_ieee_update(uint32_t crc,
                           const uint8_t *data,
                           size_t count)
{
    if (data == NULL && count > 0)
        return crc;

    for (size_t i = 0; i < count; i++) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; bit++) {
            if (crc & 1u)
                crc = (crc >> 1) ^ UINT32_C(0xEDB88320);
            else
                crc >>= 1;
        }
    }
    return crc;
}

uint32_t crc32_ieee(const uint8_t *data, size_t count)
{
    return crc32_ieee_update(UINT32_C(0xFFFFFFFF), data, count) ^
           UINT32_C(0xFFFFFFFF);
}
```

This is the reflected CRC-32/ISO-HDLC form. It is easy to audit but slower than a table-driven or hardware-accelerated implementation. A table version must be tested against this bitwise reference for empty input, one-byte input, known vectors, and split updates.

The null-data behavior in this small helper returns the input state rather than an error because its return type is only a CRC value. A public API should normally return a status so a null pointer cannot be mistaken for a valid checksum of empty data.

## Chunked Validation

Long validation should process bounded chunks. The chunk size determines memory traffic, cache behavior, cancellation frequency, and maximum time between progress checkpoints. It does not change the integrity result when the CRC state is carried across chunks.

### C: Bounded Validation With Progress Callback

```c
#include <stddef.h>
#include <stdint.h>

enum validation_status {
    VALIDATION_OK = 0,
    VALIDATION_ERR_NULL,
    VALIDATION_ERR_ARGUMENT,
    VALIDATION_ERR_CORRUPT,
    VALIDATION_ERR_CANCELED,
    VALIDATION_ERR_PROGRESS
};

typedef int (*validation_progress_fn)(size_t completed,
                                      size_t total,
                                      void *context);

enum validation_status validate_crc32_chunks(
    const uint8_t *data,
    size_t count,
    size_t chunk_size,
    uint32_t expected_crc,
    validation_progress_fn progress,
    void *context)
{
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    size_t completed = 0;

    if (data == NULL && count > 0)
        return VALIDATION_ERR_NULL;
    if (chunk_size == 0)
        return VALIDATION_ERR_ARGUMENT;

    while (completed < count) {
        size_t remaining = count - completed;
        size_t current = remaining < chunk_size ? remaining : chunk_size;

        crc = crc32_ieee_update(crc, data + completed, current);
        completed += current;
        if (progress != NULL) {
            int result = progress(completed, count, context);

            if (result < 0)
                return VALIDATION_ERR_CANCELED;
            if (result > 0)
                return VALIDATION_ERR_PROGRESS;
        }
    }

    return (crc ^ UINT32_C(0xFFFFFFFF)) == expected_crc
               ? VALIDATION_OK
               : VALIDATION_ERR_CORRUPT;
}
```

The callback runs only after a bounded chunk. It can check cancellation, record progress, or service a watchdog through a policy-owned layer. It should not make a corrupt image appear valid: completion and integrity comparison remain separate.

For empty input, the loop performs no callback and compares the CRC of the empty message. If the system requires a progress event for an empty object, make that a separate documented policy rather than relying on an accidental loop iteration.

## Watchdog-Friendly Work

Watchdog servicing should be tied to evidence that the algorithm is making valid progress. An unconditional kick inside an infinite or stuck loop defeats the watchdog's purpose.

Define:

- maximum work between checkpoints
- the clock or iteration budget used to detect a stall
- what progress means: bytes validated, records committed, or state advanced
- whether a partial result is recoverable after reset
- how checkpoint state is persisted and validated

For flash or storage scans, a checkpoint may include the last fully validated block, its sequence number, and an integrity value for the checkpoint itself. On restart, resume from the last committed block rather than trusting an index that may have been torn by power loss.

## Endianness And Wire Validation

Decode fields from bytes with explicit helpers. Check the version and length before using them:

```c
static uint16_t read_be16(const uint8_t bytes[2])
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

static int valid_frame_length(const uint8_t *frame,
                              size_t available,
                              size_t maximum,
                              size_t *out_total)
{
    size_t payload;

    if (frame == NULL || out_total == NULL || available < 4)
        return 0;
    payload = read_be16(&frame[2]);
    if (payload > maximum || payload > SIZE_MAX - 4)
        return 0;
    *out_total = payload + 4;
    return *out_total <= available;
}
```

The example assumes a two-byte version/type field followed by a two-byte payload length and a four-byte trailer. Real formats must define the exact header layout and trailer coverage. Every addition used for total length must be checked before pointer arithmetic.

## Retry And Timeout Policy

Retries are an algorithmic multiplier. If one attempt can inspect `n` bytes and there are `r` retries, the worst-case work is roughly `(r + 1)n`, not merely `n`. Include backoff, I/O timeout, reset, and resynchronization scans in the budget.

Retry only errors that are plausibly transient. Corruption in immutable storage, an invalid version, or a failed authentication check usually will not improve after an immediate retry. Re-reading a DMA buffer may be reasonable if ownership has not been returned to the producer.

Report distinct statuses for:

- validated successfully
- corrupt data
- incomplete or truncated data
- timeout
- canceled operation
- retry limit reached
- resource or allocation failure

Collapsing them into “invalid” prevents recovery code from choosing the right action.

## Resynchronization

After a corrupted frame, a stream parser may scan for a sync marker or discard through a delimiter. Bound the scan and validate the candidate header before committing to it. A marker that can occur in payload needs escaping, length validation, or a stronger framing rule.

Resynchronization can lose valid data around the corruption boundary. Record the dropped byte count and sequence number when diagnostics or replay matter.

## Python: Reference And Fault Injection

```python
import zlib


def crc32_chunks(data, chunk_size, corrupt_at=None):
    if chunk_size <= 0:
        raise ValueError("chunk_size must be positive")
    crc = 0
    for start in range(0, len(data), chunk_size):
        chunk = bytearray(data[start:start + chunk_size])
        if corrupt_at is not None and start <= corrupt_at < start + len(chunk):
            chunk[corrupt_at - start] ^= 0x01
        crc = zlib.crc32(chunk, crc)
    return crc & 0xFFFFFFFF


def retry(operation, attempts):
    if attempts <= 0:
        raise ValueError("attempts must be positive")
    for attempt in range(attempts):
        result = operation(attempt)
        if result == "ok":
            return result
        if result == "corrupt":
            return result
    return "retry_exhausted"
```

The fault-injection hook changes one byte before validation and makes the corruption case reproducible. Tests should also split data at every header and payload boundary to prove that incremental and one-shot updates agree.

## Common Mistakes

- Treating a checksum as authentication or tamper detection.
- Computing a CRC with the wrong polynomial parameters or byte coverage.
- Resetting the CRC state at each chunk.
- Kicking a watchdog without checking that meaningful progress occurred.
- Retrying permanent corruption until the watchdog resets the system.
- Trusting a length or endian-dependent struct before validating it.
- Accepting partially validated output as committed state.
- Using an unbounded resynchronization scan after frame corruption.

## Embedded And Systems Angle

- validate lengths before checksums, pointer arithmetic, or copies
- choose a chunk size from watchdog, cache, DMA, and cancellation budgets
- keep checkpoints atomic and integrity-protected where reset recovery matters
- distinguish corruption, timeout, cancellation, retry exhaustion, and allocation failure
- use a bitwise reference CRC to validate table or hardware implementations
- make data-loss and resynchronization behavior observable in diagnostics

## Review Checklist

- What bytes are covered by the integrity value?
- Are CRC parameters and serialized byte order documented?
- Is incremental state preserved across every chunk and buffer boundary?
- What proves progress before a watchdog checkpoint?
- How many retries and resynchronization bytes are permitted?
- Can restart distinguish committed, incomplete, and corrupt data?
- Is the integrity mechanism appropriate for accidental errors or authentication needs?

## Related Topics

- [Embedded Linux Algorithmic Constraints](index.md)
- [String And Protocol Parsing Algorithms](../algorithm-design-techniques/string-and-protocol-parsing-algorithms.md)
- [Cache-Aware And DMA-Friendly Layouts](cache-aware-and-dma-friendly-layouts.md)
- [Deterministic Runtime And Real-Time Tradeoffs](deterministic-runtime-and-real-time-tradeoffs.md)
- [Bounded Memory And Allocation Failure](bounded-memory-and-allocation-failure.md)
- [Interrupt-Safe Queues And Buffers](interrupt-safe-queues-and-buffers.md)
