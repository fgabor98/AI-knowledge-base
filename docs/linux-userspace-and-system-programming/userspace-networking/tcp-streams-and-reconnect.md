---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# TCP Streams And Reconnect

## What problem does this solve?

TCP provides an ordered reliable byte stream between connected endpoints. It does
not preserve application messages, guarantee prompt failure detection, or make a
reconnect safe for side effects. A client must frame data and define connection
generation and retry semantics.

## Stream framing

```text
send(frame header + payload)
recv partial header -> retain
validate length/version
recv partial payload -> retain
validate checksum/semantics
dispatch complete frame
```

One `send` may be split over many `recv` calls, and several sends may arrive in one
read. Use a maximum frame length, bounded buffers, and a parser state that survives
short I/O.

## Connect and I/O deadlines

Use separate connect, request, response, idle, and shutdown deadlines. A blocking
socket call can outlive a product watchdog if no timeout or event-loop policy exists.
For nonblocking sockets, wait for readiness and handle `EAGAIN`, `EINTR`, EOF,
`ECONNRESET`, `ETIMEDOUT`, and `EPIPE`.

`SO_RCVTIMEO`/`SO_SNDTIMEO` are useful but do not replace an end-to-end transaction
deadline. The behavior and error shape of timeout socket options differs from an
event-loop timeout; document which one is authoritative.

## Reconnect generations

```text
DISCONNECTED --connect--> CONNECTED(generation 4)
CONNECTED --request--> WAITING
WAITING --reply--> CONNECTED
any state --EOF/reset/timeout--> DISCONNECTED(generation 5)
```

Increment a generation on reconnect. Replies from an old connection must not satisfy
new requests. Outstanding side effects become unknown on disconnect; retry only if
the operation is idempotent or can be reconciled.

Use bounded exponential backoff with jitter to avoid a fleet reconnect storm. Reset
the backoff only after meaningful healthy operation, not immediately after TCP
handshake.

## Keepalive and liveness

TCP may not promptly detect a failed peer during an idle connection. Keepalive can
help but its timers and platform defaults may be too slow for a product deadline.
Application heartbeats can verify protocol health, but they consume traffic and need
an overload/timeout policy. A heartbeat reply is not proof that the requested device
operation succeeded.

## Shutdown

Stop new requests, resolve or mark in-flight operations, flush bounded output if
required, half-close/close, and wake waiting threads/event loops. Do not let a reconnect
thread reopen a socket after shutdown begins.

## Common mistakes

- Assuming TCP preserves message boundaries.
- Treating a successful send as remote application acknowledgement.
- Retrying a non-idempotent request after reset.
- Accepting late replies from an old connection generation.
- Reconnecting in a tight loop or synchronously on the event-loop thread.
- Relying on keepalive defaults for product-level deadlines.
- Forgetting EOF and half-close behavior.

## Debugging checklist

- Log connection generation, local/remote endpoint, request ID, bytes, and deadlines.
- Capture connect/accept, short reads/writes, EOF/reset, retransmissions, and retries.
- Test fragmented/coalesced frames, delayed response, reset after send, and peer
  restart.
- Verify backoff, jitter, queue limits, cancellation, and shutdown races.
- Reconcile ambiguous side effects before retrying.

## Related topics

- [Stage 9: Userspace Networking](index.md)
- [Socket Lifecycle And Addresses](socket-lifecycle-and-addresses.md)
- [Socket Options, TLS, And Network Diagnostics](socket-options-tls-and-network-diagnostics.md)
- [IPC Protocols And Versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)

## References

- [`tcp(7)`](https://man7.org/linux/man-pages/man7/tcp.7.html)
- [`connect(2)`](https://man7.org/linux/man-pages/man2/connect.2.html)
- [`shutdown(2)`](https://man7.org/linux/man-pages/man2/shutdown.2.html)
- [`socket(7)`](https://man7.org/linux/man-pages/man7/socket.7.html)
