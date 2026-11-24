# Lab 19: Network Device With NAPI

This checkpoint registers two virtual Ethernet interfaces wired back to back.
Transmitting on one queues the packet on the other and schedules its NAPI poll,
which stands in for a receive interrupt.

Put one interface into a network namespace so that traffic really crosses the
pair:

```sh
make
sudo insmod ./lab_net.ko
sudo ip netns add labns
sudo ip link set labnet1 netns labns
sudo ip addr add 192.0.2.1/24 dev labnet0
sudo ip link set labnet0 up
sudo ip -n labns addr add 192.0.2.2/24 dev labnet1
sudo ip -n labns link set labnet1 up
ping -c 3 192.0.2.2
ip -s link show labnet0
ethtool -i labnet0
```

Clean up. Delete the namespace first; the interface returns to the initial
namespace:

```sh
sudo ip netns del labns
sudo rmmod lab_net
```

Expected evidence: three ping replies and matching RX/TX counters.

Read `lab_net_xmit()` for skb ownership and `lab_net_poll()` for the budget
rules. Experiment: return `done` without calling `napi_complete_done()` when
the queue is empty and watch the CPU load of `ksoftirqd`.
