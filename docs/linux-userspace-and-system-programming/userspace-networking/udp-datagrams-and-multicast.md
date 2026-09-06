---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# UDP Datagrams And Multicast

## What problem does this solve?

UDP preserves datagram boundaries but does not provide reliable delivery, ordering,
deduplication, congestion control, or a connected peer contract. It is useful for
bounded telemetry, discovery, and protocols that implement their own loss policy.

## Datagram contract

```text
send one bounded datagram -> receive one datagram or loss/truncation/error
```

Keep each message below the application and path MTU policy. Fragmentation can make
loss more likely; an application protocol should prefer bounded messages or provide
its own fragmentation/reassembly with expiry and memory limits. `recvmsg` can report
truncation; do not process a truncated payload as complete.

Include sequence, timestamp/domain, source identity, and checksum/authentication as
needed. Define duplicate, reorder, loss, and stale-data behavior. A telemetry stream
may drop old samples; a command protocol should usually use a reliable transport or
an explicit acknowledgement/retry design.

## Connected UDP

`connect` on a UDP socket selects a default peer and filters received packets by peer;
it does not create a TCP-style connection or reliability. It can also make asynchronous
error reporting and write calls more convenient. Treat peer absence and route changes
as normal failures.

## Multicast

Multicast requires group address, port, interface, TTL/hop limit, loopback, and group
join/leave policy. A group membership is interface-specific and can disappear when
the link or namespace changes. Receivers must authenticate and validate datagrams;
network locality is not authorization.

```text
sender -> multicast group -> zero or more receivers
```

Do not assume every network, router, Wi-Fi link, or embedded kernel forwards multicast.
Test IGMP/MLD, firewall, route, interface, and power-mode behavior on the target.

## Overload and rate limits

Bound receive buffers and application queues. A fast sender can fill kernel buffers,
cause drops, and consume CPU in parsing. Report drops and sequence gaps. Use sampling,
coalescing, rate limits, or backpressure at a higher layer; UDP itself will not slow
the sender for you.

## Common mistakes

- Treating UDP as reliable because a send succeeded.
- Ignoring datagram truncation, sequence gaps, and duplicate packets.
- Sending oversized messages and relying on IP fragmentation.
- Assuming multicast is available on every interface/path.
- Authorizing packets by source address alone.
- Leaving unbounded reassembly buffers or retry state.

## Debugging checklist

- Log family, local/remote/group endpoint, interface, TTL, sequence, and length.
- Inspect `ip addr`, `ip route`, multicast memberships, firewall, and namespace.
- Capture packets with `tcpdump` and compare application drop counters.
- Test loss, reorder, duplication, truncation, interface change, and receiver restart.
- Verify authentication and stale-data expiry.

## Related topics

- [Stage 9: Userspace Networking](index.md)
- [Socket Lifecycle And Addresses](socket-lifecycle-and-addresses.md)
- [IPv4, IPv6, DNS, And Interface Binding](ipv4-ipv6-dns-and-interface-binding.md)
- [Socket Options, TLS, And Network Diagnostics](socket-options-tls-and-network-diagnostics.md)

## References

- [`udp(7)`](https://man7.org/linux/man-pages/man7/udp.7.html)
- [`ip(7)`](https://man7.org/linux/man-pages/man7/ip.7.html)
- [`ip-maddress(8)`](https://man7.org/linux/man-pages/man8/ip-maddress.8.html)
- [`recvmsg(2)`](https://man7.org/linux/man-pages/man2/recvmsg.2.html)
