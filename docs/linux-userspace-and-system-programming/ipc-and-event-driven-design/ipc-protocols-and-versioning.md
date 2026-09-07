---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# IPC Protocols And Versioning

## What problem does this solve?

An IPC channel that works between matching binaries can fail during rolling updates,
partial deployment, restart, or malformed input. A versioned protocol makes messages,
errors, capabilities, retries, and compatibility explicit.

## Frame design

```text
magic | major | minor | message_type | flags | request_id | payload_length | payload
```

Define byte order, integer widths, alignment, maximum header/payload sizes, and
whether unknown fields are ignored or rejected. Validate magic, version, type, flags,
length, and authorization before allocating or acting. A stream parser must retain
partial headers and payloads across reads.

## Requests and replies

Each request should have a request ID and a defined result shape:

```text
accepted -> in progress -> succeeded/failed/cancelled/unknown
```

Asynchronous events need their own type and sequence. A reply may arrive after a
client timeout; the client must reject stale generations or reconcile the side effect.
Cancellation is a protocol message with its own acknowledgement and race semantics,
not just a local socket close.

## Compatibility

Use major versions for incompatible semantics and minor versions for compatible
extensions. Reserve fields, define default values, and prefer capability negotiation
over guessing from a version number. Old clients should ignore known-safe optional
fields; new clients must tolerate missing optional fields. Reject unknown required
flags.

Never serialize C structs by copying their in-memory representation across processes:
padding, alignment, endianness, ABI widths, and pointers are not stable wire format.

## Idempotence and retry

Commands that may be retried need an idempotency key or sequence. A client cannot know
whether a peer crashed before or after applying a side effect. Provide a query or
generation read that reconciles state. Do not retry “set once,” “increment,” or “erase”
blindly unless their semantics explicitly make that safe.

## Bounds and overload

Define maximum message size, in-flight requests per peer, output queue, retry count,
deadline, and error rate. On overload, reject early with a structured error, shed
optional work, or apply class-specific backpressure. Do not let malformed lengths or
unresponsive clients consume unbounded memory.

## Common mistakes

- Copying C structs as a wire format.
- Accepting unbounded lengths before validation.
- Treating version equality as capability negotiation.
- Retrying ambiguous non-idempotent commands.
- Reusing request IDs across live generations.
- Ignoring unknown flags, malformed frames, or partial messages.
- Providing no compatibility test between old and new clients.

## Debugging checklist

- Log protocol version, capabilities, type, request ID, generation, and lengths.
- Test fragmentation, coalescing, unknown fields, old/new peers, and malformed input.
- Test timeout before reply, peer crash after side effect, duplicate request, and
  cancellation races.
- Check queue bounds and overload responses.
- Preserve wire captures with sensitive-data redaction.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [IPC Selection And Failure Models](ipc-selection-and-failure-models.md)
- [Credentials, Authentication, And Peer Lifecycle](credentials-authentication-and-peer-lifecycle.md)
- [Serialization And Protocols](../../c/advanced-c/protocols-and-serialization.md)
