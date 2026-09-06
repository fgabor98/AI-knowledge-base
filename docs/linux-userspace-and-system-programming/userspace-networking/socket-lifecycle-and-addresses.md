---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Socket Lifecycle And Addresses

## What problem does this solve?

Sockets combine a descriptor lifecycle with address families, protocol state, queues,
and peer behavior. Errors such as `EAFNOSUPPORT`, `EADDRINUSE`, `ECONNREFUSED`, and
`ENETUNREACH` are useful only when the program records which lifecycle step failed.

## Lifecycle

```text
socket -> bind (optional) -> listen -> accept      server stream
socket -> connect                              client stream
socket -> bind/sendto/recvfrom                 datagram
any endpoint -> shutdown -> close
```

Use `SOCK_CLOEXEC` and `SOCK_NONBLOCK` at creation when supported. The creator owns
the FD. `accept4` can apply the same flags to the accepted socket atomically.

## Address structures

Prefer `getaddrinfo` and `getnameinfo` over hand-built family-specific assumptions.
Use `sockaddr_storage` for storage and inspect the returned family and length. Never
cast an IPv4 structure to an IPv6 structure or assume `sizeof(sockaddr_in)` applies
to every call.

```c
struct addrinfo hints = {
    .ai_socktype = SOCK_STREAM,
    .ai_family = AF_UNSPEC,
};
struct addrinfo *results;
int error = getaddrinfo(host, service, &hints, &results);
if (error != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
}
```

`getaddrinfo` errors are not necessarily in `errno`. Free results with
`freeaddrinfo` on every path. Numeric conversion functions such as `inet_pton` and
`inet_ntop` have their own return contracts.

## Server binding

A server should bind an explicit address and port policy. `INADDR_ANY`/`in6addr_any`
accepts traffic on all eligible interfaces; that may be wrong for a management-only
service. Port `0` asks the kernel for an ephemeral port, useful in tests. `SO_REUSEADDR`
has platform-specific details and is not a universal permission to share a live port.

Check bind/listen backlog, namespace, interface, firewall, and service identity.

## Client connection

A client may try multiple `getaddrinfo` results. Bound each attempt and the overall
operation with a monotonic deadline. Do not retry an address forever before trying a
healthy alternative. For nonblocking connect, wait for writability and inspect
`SO_ERROR`; writable is not proof of connection success.

## Shutdown and ownership

`shutdown(SHUT_WR)` sends an orderly half-close; `shutdown(SHUT_RDWR)` prevents
further communication according to socket semantics. EOF from a stream is peer
half-close, not necessarily a protocol error. Close only after queued output and
outstanding request policy is settled.

## Common mistakes

- Assuming all endpoints are IPv4 or all hostnames resolve to one address.
- Using `sizeof(sockaddr_in)` for every family.
- Treating `getaddrinfo` errors as `errno` values.
- Binding all interfaces unintentionally.
- Treating nonblocking connect writability as success without `SO_ERROR`.
- Retrying indefinitely without a total deadline or backoff.
- Sharing one socket FD across components without ownership rules.

## Debugging checklist

- Record family, numeric/local/remote endpoint, interface, namespace, and FD.
- Capture socket/bind/connect/listen/accept errors and durations.
- Inspect `ss -lntup`, `ip addr`, `ip route`, firewall, and namespace context.
- Test absent route, refused port, delayed connect, dual-stack, and address change.
- Verify close/shutdown, accepted-FD flags, and descriptor inheritance.

## Related topics

- [Stage 9: Userspace Networking](index.md)
- [TCP Streams And Reconnect](tcp-streams-and-reconnect.md)
- [IPv4, IPv6, DNS, And Interface Binding](ipv4-ipv6-dns-and-interface-binding.md)
- [File Descriptors And Open-File Descriptions](../system-calls-files-and-file-descriptors/file-descriptors-and-open-file-descriptions.md)

## References

- [`socket(2)`](https://man7.org/linux/man-pages/man2/socket.2.html)
- [`bind(2)`](https://man7.org/linux/man-pages/man2/bind.2.html)
- [`accept4(2)`](https://man7.org/linux/man-pages/man2/accept4.2.html)
- [`getaddrinfo(3)`](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html)
