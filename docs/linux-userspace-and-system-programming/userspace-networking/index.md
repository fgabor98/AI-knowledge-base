---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 9: Userspace Networking

Userspace networking is the design of byte and datagram protocols over sockets, not
the design of Ethernet hardware or a MAC/PHY driver. A socket program must define
address selection, connection and peer lifetime, framing, timeouts, retries,
backpressure, security, and behavior when the network is absent or changes.

## The network path

```text
application protocol
  -> TLS or other security layer
  -> socket API
  -> kernel protocol stack / routing / namespaces
  -> interface / driver / link
  -> peer stack and application
```

Successful `connect`, `send`, or `sendto` only establishes the guarantee documented
by that layer. It does not prove that the remote application consumed the message or
that the link will remain available.

## Learning materials

1. [Socket Lifecycle And Addresses](socket-lifecycle-and-addresses.md)
2. [TCP Streams And Reconnect](tcp-streams-and-reconnect.md)
3. [UDP Datagrams And Multicast](udp-datagrams-and-multicast.md)
4. [IPv4, IPv6, DNS, And Interface Binding](ipv4-ipv6-dns-and-interface-binding.md)
5. [Socket Options, TLS, And Network Diagnostics](socket-options-tls-and-network-diagnostics.md)

## Protocol contract

| Area | Decision |
| --- | --- |
| Addressing | IPv4/IPv6, hostname resolution, interface/route policy |
| Transport | Stream, datagram, multicast, local fallback |
| Framing | Length, delimiter, maximum message, encoding |
| Timing | Connect, read, write, transaction, keepalive deadlines |
| Reliability | Ordering, loss, duplicate, retry, idempotence |
| Backpressure | Send queue, receive queue, application queue, overload response |
| Security | TLS/authentication, certificate/time policy, peer identity |
| Lifecycle | Startup without network, disconnect, DNS change, reconnect, shutdown |
| Observability | Endpoint, family, interface, state, sequence, errno, latency |

## Stage lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-network-loopback.c -o /tmp/network-loopback
/tmp/network-loopback
```

The probe uses TCP on loopback. Extend the lab with a delayed peer, fragmented frame,
connection reset, IPv6 loopback, and a bounded reconnect loop.

## Completion criteria

You can complete this stage when you can:

- create and close sockets with explicit address-family and ownership policy;
- frame TCP streams and handle short I/O, EOF, reset, and reconnect;
- choose UDP only when loss/duplication/order and size limits are acceptable;
- resolve and select IPv4/IPv6 endpoints without blocking the wrong thread;
- bind to an interface deliberately and diagnose routes/firewalls/namespaces;
- configure socket queues/timeouts/keepalive and integrate TLS safely;
- test network absence, delay, loss, peer restart, DNS change, and certificate failure.

## Related topics

- [Stage 7: IPC And Event-Driven Design](../ipc-and-event-driven-design/index.md)
- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](../hardware-facing-userspace-and-kernel-uapi/index.md)
- [Socket Lifecycle And Addresses](socket-lifecycle-and-addresses.md)
- [Network Diagnostics](socket-options-tls-and-network-diagnostics.md)

## References

- [`socket(7)`](https://man7.org/linux/man-pages/man7/socket.7.html)
- [`ip(7)`](https://man7.org/linux/man-pages/man7/ip.7.html)
- [`ipv6(7)`](https://man7.org/linux/man-pages/man7/ipv6.7.html)
- [Linux kernel networking documentation](https://www.kernel.org/doc/html/latest/networking/index.html)
