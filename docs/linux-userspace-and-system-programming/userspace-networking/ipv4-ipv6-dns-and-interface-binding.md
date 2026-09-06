---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# IPv4, IPv6, DNS, And Interface Binding

## What problem does this solve?

Embedded network environments change: interfaces appear late, addresses rotate,
DNS is unavailable at boot, and a hostname can resolve to several families. Code that
assumes one IPv4 address or one default interface becomes fragile during boot,
roaming, updates, and recovery.

## Address families

Use `AF_UNSPEC` with `getaddrinfo` when both IPv4 and IPv6 are valid. Store addresses
in `sockaddr_storage`, use the returned `ai_addrlen`, and format with `getnameinfo`
or `inet_ntop`. IPv6 link-local addresses require a scope/interface for correct use.

```c
struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
};
struct addrinfo *list;
int error = getaddrinfo(host, service, &hints, &list);
```

Try candidates with a bounded overall deadline. “Happy Eyeballs”-style parallel or
staggered attempts can reduce family/address delay, but add sockets and cancellation
complexity; use a target-supported implementation and measure it.

## DNS is a runtime dependency

Name resolution can block, fail due to missing configuration, return stale results,
or depend on a network that is not ready. Do not perform unbounded synchronous DNS
on an event loop. Cache with a defined TTL and refresh policy, but re-resolve after
network change or connection failure. Never treat a cached IP as permanent identity
without certificate/authentication validation.

## Interface and route selection

Binding a local address or using `SO_BINDTODEVICE`/`IP_UNICAST_IF` is a policy choice.
It can restrict a service to management, cellular, or a private interface, but may
fail when the interface is absent or renamed. Prefer routing policy and explicit
configuration where possible; interface binding often requires privilege and is
Linux-specific.

Inspect:

```sh
ip addr
ip route
ip -6 route
getent ahosts example.com
resolvectl status 2>/dev/null || true
```

Network namespaces can have different interfaces, routes, and DNS configuration.
Inspect from the service’s namespace.

## Link-local, dual-stack, and address changes

IPv6 link-local addresses are scoped. Dual-stack sockets have `IPV6_V6ONLY` policy;
do not assume one IPv6 listener accepts IPv4. A DHCP/RA renewal can invalidate local
addresses and existing connections. Rebind/reconnect through an explicit network
state machine.

## Common mistakes

- Assuming `getaddrinfo` returns one IPv4 address.
- Treating DNS resolution as instant, reliable, or nonblocking.
- Forgetting IPv6 scope IDs for link-local addresses.
- Binding to an interface name that is not stable on the target.
- Confusing address reachability with peer authentication.
- Ignoring network namespace and route differences.
- Failing to reconnect after DHCP/RA/address changes.

## Debugging checklist

- Record hostname, resolved addresses, family, scope/interface, route, and namespace.
- Capture DNS status, response timing, TTL/cache, and connection attempt order.
- Inspect addresses/routes/firewall from the service context.
- Test DNS absence, delayed readiness, dual-stack failure, address rotation, and
  interface disappearance.
- Verify TLS identity independent of DNS/IP address.

## Related topics

- [Stage 9: Userspace Networking](index.md)
- [Socket Lifecycle And Addresses](socket-lifecycle-and-addresses.md)
- [TCP Streams And Reconnect](tcp-streams-and-reconnect.md)
- [Socket Options, TLS, And Network Diagnostics](socket-options-tls-and-network-diagnostics.md)

## References

- [`getaddrinfo(3)`](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html)
- [`getnameinfo(3)`](https://man7.org/linux/man-pages/man3/getnameinfo.3.html)
- [`ipv6(7)`](https://man7.org/linux/man-pages/man7/ipv6.7.html)
- [`ip(8)`](https://man7.org/linux/man-pages/man8/ip.8.html)
