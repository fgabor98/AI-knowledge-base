---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Socket Options, TLS, And Network Diagnostics

## What problem does this solve?

Socket defaults are not a product protocol. Buffer sizes, timeouts, keepalive,
reuse, and error reporting affect failure behavior. TLS adds certificate, hostname,
clock, trust-store, and renegotiation/record-layer concerns. Diagnostics must identify
whether failure occurred before transport, in transport, or in the secure protocol.

## Useful socket options

| Option/facility | Purpose | Caveat |
| --- | --- | --- |
| `SO_RCVBUF` / `SO_SNDBUF` | Kernel queue sizing | Effective size may be adjusted; does not bound application queues |
| `SO_RCVTIMEO` / `SO_SNDTIMEO` | Socket call wait limit | Not an end-to-end transaction deadline |
| `SO_KEEPALIVE` and TCP keepalive options | Detect idle peer failure | Often too slow for application health |
| `SO_REUSEADDR` | Rebinding policy | Semantics vary; not a universal sharing permission |
| `SO_ERROR` | Read pending asynchronous socket error | Required after nonblocking connect completion |
| `IP_TOS`/traffic policy | Packet marking | Requires network policy and may be ignored |
| `TCP_NODELAY` | Disable Nagle coalescing | Can increase packet rate; measure |
| `SO_BINDTODEVICE` | Interface restriction | Linux-specific and privileged in many contexts |

Set options immediately after creating/accepting the socket and check every return.
Record effective policy in diagnostics. Application queues need independent bounds.

## TLS boundary

TLS authenticates and encrypts a byte stream; it does not define application framing,
retry idempotence, peer lifecycle, or device semantics. Use a maintained TLS library
and its documented API. Configure protocol versions, trust store, hostname/identity
verification, certificate rotation, client authentication, and entropy policy.

The system clock affects certificate validity. A device without a trusted time source
needs a bootstrap trust design that does not silently disable verification. Never ship
“accept any certificate” as a recovery shortcut.

TLS reads/writes can also be partial and can request the opposite direction due to
record processing. Integrate the library’s WANT_READ/WANT_WRITE and shutdown rules
with the event loop.

## Diagnostic layers

```text
name resolution -> route/interface -> TCP/UDP -> TLS -> application protocol -> peer action
```

Use evidence at the first failing layer:

```sh
getent ahosts host
ip route get address
ss -tanp
tcpdump -ni any host address
openssl s_client -connect host:443 -servername host  # controlled diagnostic
```

On production targets, tools may be absent and packet captures may expose secrets.
Use a support policy, redaction, and least-invasive diagnostics.

## Common mistakes

- Treating socket buffers as application queue bounds.
- Relying only on keepalive for request deadlines.
- Disabling TLS verification when the clock or trust store is wrong.
- Assuming TLS write/read calls map one-to-one to application messages.
- Diagnosing TLS before checking DNS, routes, interface, and namespace.
- Capturing credentials or payloads in unrestricted packet/log bundles.

## Debugging checklist

- Record family, endpoint, interface, options, queue sizes, and namespace.
- Capture DNS, route, socket state, TCP errors, TLS verification result, and protocol
  frame state separately.
- Test timeout, keepalive, peer reset, certificate expiry, trust-store update, and
  clock-not-set cases.
- Check partial TLS I/O and event-loop read/write interest.
- Redact packet captures and preserve exact library/certificate configuration.

## Related topics

- [Stage 9: Userspace Networking](index.md)
- [Socket Lifecycle And Addresses](socket-lifecycle-and-addresses.md)
- [TCP Streams And Reconnect](tcp-streams-and-reconnect.md)
- [IPv4, IPv6, DNS, And Interface Binding](ipv4-ipv6-dns-and-interface-binding.md)

## References

- [`socket(7)`](https://man7.org/linux/man-pages/man7/socket.7.html)
- [`tcp(7)`](https://man7.org/linux/man-pages/man7/tcp.7.html)
- [`ss(8)`](https://man7.org/linux/man-pages/man8/ss.8.html)
- [OpenSSL documentation](https://docs.openssl.org/)
