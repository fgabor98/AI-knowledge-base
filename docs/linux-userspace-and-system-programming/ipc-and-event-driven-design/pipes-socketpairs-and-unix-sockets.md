---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Pipes, socketpairs, And Unix Sockets

## What problem does this solve?

Local processes need a transport that provides the desired directionality, message
boundaries, peer lifecycle, and backpressure. Pipes are excellent streams; Unix
sockets add independent endpoints, credentials, reconnect, and datagrams.

## Pipes and socketpairs

Use a pipe for one-way byte flow and `socketpair(AF_UNIX, SOCK_STREAM, 0, fds)` for a
related bidirectional stream. Both require framing and exact closure ownership.

```text
pipe:       writer --> kernel buffer --> reader
socketpair: endpoint A <--> kernel buffers <--> endpoint B
```

`socketpair` endpoints are already connected and do not have a filesystem pathname.
They are useful for parent/child control channels and event-loop wakeups.

## Unix stream sockets

A server creates a socket, binds a pathname or abstract address, listens, and accepts:

```text
socket -> bind -> listen -> accept
client: socket -> connect
both sides: read/write framed stream -> shutdown/close
```

The pathname is a filesystem object with ownership and stale-socket cleanup policy.
Unlink a stale socket only after verifying that it belongs to the service; blindly
removing a live pathname can disrupt another instance. Abstract sockets avoid a
filesystem path but have different namespace and discoverability semantics.

## Framing

Streams do not preserve writes. Use a fixed header:

```text
magic | version | type | flags | request_id | payload_length | payload
```

Read exactly the header, validate magic/version/length against a fixed maximum, then
read the payload through a bounded loop. Keep partial bytes in a connection-owned
buffer. Never allocate `payload_length` before checking its range.

## Datagrams

Unix datagrams preserve message boundaries, but messages can be rejected, truncated,
or lost according to queue and socket behavior. Design each datagram to stand alone
or include a request ID and retry policy. Do not assume a datagram is authenticated
just because it is local.

## Passing descriptors

Unix sockets can pass FDs with `SCM_RIGHTS`. Receiving an FD transfers a kernel handle,
not necessarily trust. Validate object type, flags, peer authority, and lifetime.
Set close-on-exec on received descriptors and close them on every rejection path.

## Shutdown

EOF means the peer performed an orderly stream close. `ECONNRESET`, `EPIPE`, and
`SIGPIPE` indicate other lifecycle paths. `shutdown(SHUT_WR)` can half-close a socket;
define whether the protocol permits it. Close only after queued output and pending
responses are resolved by policy.

## Common mistakes

- Assuming one `send` equals one `recv`.
- Leaving stale Unix socket paths or deleting a live one.
- Treating local transport as authentication.
- Accepting unbounded frame lengths or FD counts.
- Forgetting close-on-exec on passed/received descriptors.
- Ignoring half-close, EOF, reset, and SIGPIPE semantics.
- Using one stream for unrelated messages without request IDs or framing.

## Debugging checklist

- Inspect `ss -x -a`, socket path, owner, mode, namespace, and peer credentials.
- Trace connect, accept, send, receive, shutdown, EOF, and reset events.
- Test fragmented headers/payloads, oversized frames, stale paths, and peer crash.
- Check FD passing and close-on-exec with `/proc/<pid>/fd`.
- Verify backpressure, output queues, cancellation, and restart generations.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [IPC Selection And Failure Models](ipc-selection-and-failure-models.md)
- [IPC Protocols And Versioning](ipc-protocols-and-versioning.md)
- [Descriptor Inheritance And Redirection](../system-calls-files-and-file-descriptors/descriptor-inheritance-and-redirection.md)

## References

- [`unix(7)`](https://man7.org/linux/man-pages/man7/unix.7.html)
- [`socketpair(2)`](https://man7.org/linux/man-pages/man2/socketpair.2.html)
- [`unix(4)`](https://man7.org/linux/man-pages/man4/unix.4.html)
- [`cmsg(3)`](https://man7.org/linux/man-pages/man3/cmsg.3.html)
