---
status: draft
reviewed: false
domain: networking
difficulty: beginner
last_reviewed: null
---

# Networking

Networking topics focused on Linux systems, embedded bring-up, diagnostics, and practical protocol understanding.

## Roadmap

### Embedded Ethernet Bring-Up

- MAC vs PHY
- MDIO
- PHY addresses
- link negotiation
- RGMII/RMII interface modes
- PHY reset GPIOs
- PHY interrupt lines
- fixed-link
- device tree networking nodes
- U-Boot Ethernet vs Linux Ethernet

### Linux Network Configuration

- static IP
- DHCP
- routes
- DNS
- systemd-networkd
- NetworkManager tradeoffs
- interface naming
- persistent configuration
- initramfs networking

### Network Diagnostics

- `ip addr`
- `ip link`
- `ip route`
- `ethtool`
- `tcpdump`
- `ss`
- `ping`
- ARP checks
- link-state checks
- packet capture workflow

### Product Networking Features

- VLANs
- bridges
- routing
- firewalling
- nftables overview
- time sync
- NTP
- PTP overview
- service discovery overview

## Related Topics

- [Embedded Linux](../embedded-linux/index.md)
- [Device Tree](../device-tree/index.md)
- [Topic Map](../topic-map.md)
