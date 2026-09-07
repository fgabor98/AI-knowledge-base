---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Serial Protocols, Timeouts, And Testing

## What problem does this solve?

Serial links deliver bytes, not requests. Bytes can be fragmented, delayed, duplicated
by a retrying peer, corrupted, or left over from a previous transaction. A reliable
protocol defines framing, validation, sequence, timeout, resynchronization, and
recovery independently from UART configuration.

## Frame design

```text
sync/magic | version | type | sequence | length | payload | checksum
```

Validate magic, version, type, flags, length, checksum, and semantic ranges before
acting. Bound length before allocating. Include a sequence or transaction ID so a
late response cannot satisfy a newer request. Decide whether unknown message types
are ignored, reported, or fatal.

## Timeout layers

Use separate budgets:

| Timeout | Meaning |
| --- | --- |
| Inter-byte | Maximum gap while a frame is arriving |
| Frame | Maximum time to receive a complete frame |
| Response | Maximum time from request to valid response |
| Transaction | End-to-end budget including retries and recovery |
| Reconnect | Time before reopening/reinitializing the link |

Compute all from a monotonic absolute deadline. Do not reset the transaction deadline
after every retry or partial read. A timeout does not prove the device stopped; late
responses must be discarded by sequence/generation or explicitly reconciled.

## Resynchronization

On bad checksum or length, scan for the next valid sync pattern within a bounded
buffer. Avoid accepting a magic value inside an unvalidated payload as a new frame
without a length/checksum strategy. If the parser cannot establish alignment, flush,
reset, or reopen according to the device protocol.

## Retry and idempotence

A read-only query can often be retried; a command that changes hardware state needs an
idempotency key, status query, or device transaction semantics. Record attempt count,
sequence, response, and whether the outcome is known. Use bounded exponential backoff
for reconnects and avoid reset storms.

## PTY and fake-device tests

A fake peer should generate:

- fragmented headers and payloads;
- delayed and empty responses;
- bad checksum, invalid length, old sequence, and unsupported version;
- response timeout, late response, disconnect, and reconnect;
- peer reset between command and acknowledgement;
- overload and repeated reset conditions.

PTYs test parser and process behavior. Loopback tests test some serial configuration.
Only a physical link or appropriate electrical test equipment proves baud, parity,
flow-control, voltage, timing, and cable behavior.

## Common mistakes

- Assuming one `read` returns one frame.
- Using a relative timeout afresh after every byte or retry.
- Retrying a non-idempotent command after an ambiguous timeout.
- Accepting payload before checksum and range validation.
- Treating a PTY as proof of UART electrical correctness.
- Resetting on every malformed byte and creating a reset storm.
- Allowing stale responses to satisfy new requests.

## Debugging checklist

- Log port, baud/mode, frame sequence/type/length, timestamps, deadline, and result.
- Preserve raw bytes with sensitive-data controls and bounded capture size.
- Test parser, timeout, retry, reconnect, reset, and power-cycle paths separately.
- Compare PTY/fake, loopback, logic-analyzer, and real-device evidence.
- Check queue ownership when serial I/O is separated from protocol workers.
- Verify behavior when the device is silent, chatty, slow, or partially initialized.

## Related topics

- [Stage 8: Terminals, TTYs, And Serial Userspace](index.md)
- [termios And Serial Configuration](termios-and-serial-configuration.md)
- [Blocking, Nonblocking, And Partial I/O](../system-calls-files-and-file-descriptors/blocking-nonblocking-and-partial-io.md)
- [IPC Protocols And Versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)

## References

- [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html)
- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`termios(3)`](https://man7.org/linux/man-pages/man3/termios.3.html)
- [Linux kernel TTY documentation](https://www.kernel.org/doc/html/latest/driver-api/tty/index.html)
