---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Network Device Drivers

## What Problem Does This Solve?

Network hardware has no `/dev` node. A driver registers a `net_device` and exchanges packets with the networking stack; userspace sees an interface through `ip`, `ethtool`, and sockets. Performance dictates the structure: interrupts are suppressed under load, packets are processed in batches, and ownership of every buffer is explicit.

This page covers the driver side of an Ethernet-like device. [CAN](can-driver-integration.md) uses the same framework with its own helpers.

## Core Concepts

- `struct net_device`, `netdev_priv()`
- `struct net_device_ops`
- `struct sk_buff` (skb) and ownership
- transmit queue stop and wake
- NAPI and the poll budget
- descriptor rings and DMA
- carrier state and phylink
- RTNL lock
- ethtool operations, statistics

## Mental Model

```text
transmit                                receive
--------                                -------
stack owns skb                          hardware DMAs into posted buffer
-> ndo_start_xmit()  (atomic)           -> IRQ: mask device IRQ,
-> map, write descriptor, doorbell         napi_schedule()
   driver/hardware own skb              -> poll(budget)  (softirq)
-> completion in NAPI poll                 build skb, napi_gro_receive()
   unmap, free skb,                        refill ring
   wake the queue                       -> fewer than budget:
                                           napi_complete_done(),
                                           unmask IRQ
```

## Registration

```c
static const struct net_device_ops demo_netdev_ops = {
    .ndo_open = demo_open,
    .ndo_stop = demo_stop,
    .ndo_start_xmit = demo_xmit,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_set_mac_address = eth_mac_addr,
};

static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct net_device *ndev;
    struct demo_priv *priv;

    ndev = devm_alloc_etherdev(dev, sizeof(*priv));
    if (!ndev)
        return -ENOMEM;
    SET_NETDEV_DEV(ndev, dev);

    priv = netdev_priv(ndev);
    priv->ndev = ndev;

    ndev->netdev_ops = &demo_netdev_ops;
    ndev->ethtool_ops = &demo_ethtool_ops;
    ndev->pcpu_stat_type = NETDEV_PCPU_STAT_TSTATS;
    netif_napi_add(ndev, &priv->napi, demo_poll);

    if (device_get_ethdev_address(dev, ndev))
        eth_hw_addr_random(ndev);

    /* Everything must be ready: ndo_open may run immediately. */
    return devm_register_netdev(dev, ndev);
}
```

Private data lives in the memory behind `net_device`, and `free_netdev()` releases both. Never `kfree()` either one.

The MAC address comes from firmware: `device_get_ethdev_address()` reads `local-mac-address` and related properties, and nvmem cells can supply it as well.

## Open And Stop

```c
static int demo_open(struct net_device *ndev)
{
    struct demo_priv *priv = netdev_priv(ndev);
    int ret;

    ret = demo_alloc_rings(priv);
    if (ret)
        return ret;

    napi_enable(&priv->napi);
    demo_hw_start(priv);
    demo_irq_enable(priv);
    phylink_start(priv->phylink);
    netif_start_queue(ndev);
    return 0;
}

static int demo_stop(struct net_device *ndev)
{
    struct demo_priv *priv = netdev_priv(ndev);

    netif_stop_queue(ndev);
    phylink_stop(priv->phylink);
    demo_irq_disable(priv);
    napi_disable(&priv->napi);      /* waits for a running poll */
    demo_hw_stop(priv);             /* DMA off before freeing buffers */
    demo_free_rings(priv);
    return 0;
}
```

Both run in process context under the RTNL lock. Acquire interface-up resources here rather than in probe, so that a downed interface consumes no power and no buffers.

## Transmit

```c
static netdev_tx_t demo_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    struct demo_priv *priv = netdev_priv(ndev);
    dma_addr_t addr;

    addr = dma_map_single(priv->dev, skb->data, skb->len, DMA_TO_DEVICE);
    if (dma_mapping_error(priv->dev, addr)) {
        dev_core_stats_tx_dropped_inc(ndev);
        dev_kfree_skb_any(skb);
        return NETDEV_TX_OK;
    }

    demo_ring_put(priv, skb, addr);

    if (demo_ring_free(priv) < DEMO_STOP_THRESHOLD) {
        netif_stop_queue(ndev);
        /* Completion may have freed entries before the stop was visible. */
        smp_mb();
        if (demo_ring_free(priv) >= DEMO_WAKE_THRESHOLD)
            netif_start_queue(ndev);
    }

    demo_doorbell(priv);
    return NETDEV_TX_OK;
}
```

Ownership rules:

- Returning `NETDEV_TX_OK` means the driver consumed the skb, including when it dropped it.
- `NETDEV_TX_BUSY` means the skb was not touched. It should never happen: stop the queue *before* it can be full.
- After the skb is handed to the hardware, another CPU may complete and free it. Do not read `skb->len` afterwards.
- Free completed skbs with `napi_consume_skb(skb, budget)` in poll. In any other context use `dev_kfree_skb_any()` or `dev_consume_skb_any()`. The `kfree` forms count as drops in drop monitoring, the `consume` forms do not.

The function runs in atomic context, serialized per queue by the transmit lock. `include/net/netdev_queues.h` has `netif_txq_maybe_stop()` and `netif_txq_completed_wake()`, which implement the stop and wake races correctly.

## NAPI Receive

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    if (!demo_irq_pending(priv))
        return IRQ_NONE;

    if (napi_schedule_prep(&priv->napi)) {
        demo_irq_disable(priv);
        __napi_schedule(&priv->napi);
    }
    return IRQ_HANDLED;
}

static int demo_poll(struct napi_struct *napi, int budget)
{
    struct demo_priv *priv = container_of(napi, struct demo_priv, napi);
    int done = 0;

    demo_tx_complete(priv, budget);

    while (done < budget && demo_rx_ready(priv)) {
        struct sk_buff *skb = demo_rx_build_skb(priv);

        if (skb) {
            skb->protocol = eth_type_trans(skb, priv->ndev);
            napi_gro_receive(napi, skb);    /* stack owns it now */
        }
        done++;
    }

    if (done < budget && napi_complete_done(napi, done))
        demo_irq_enable(priv);

    return done;
}
```

The contract:

- Process at most `budget` received packets and return how many were processed.
- Returning `budget` means "more work": do not complete, do not unmask. The core polls again.
- Returning less requires `napi_complete_done()` first, and interrupts are unmasked only if it returned true.
- When `budget` is 0, process only transmit completions and receive nothing. Netpoll does this.
- Transmit completion is not limited by the budget.

A device that raises an edge or pulse for a new packet needs a recheck after unmasking; otherwise a packet that arrived in between waits for the next one.

Allocate receive buffers with `napi_alloc_skb()` for copies or with the page pool API for zero-copy rings.

## Link State

Do not write PHY code by hand. Describe the PHY or fixed link in Device Tree and use phylink (or the older phylib), which calls the driver when speed, duplex, or link changes:

```text
phylink_create() in probe -> phylink_of_phy_connect() and phylink_start() in open
-> mac_link_up()/mac_link_down() callbacks -> carrier state handled for you
```

A device without PHY reports link with `netif_carrier_on()` and `netif_carrier_off()`. Without a correct carrier state, userspace network managers will not configure the interface.

## Configuration And Statistics

| Setting | Interface |
| --- | --- |
| MTU | `ndo_change_mtu`, plus `min_mtu`/`max_mtu` |
| link modes, pause, EEE | ethtool via phylink |
| ring sizes, coalescing | ethtool |
| offloads | `hw_features`, `features`, `ndo_set_features` |
| hardware counters | `ndo_get_stats64`, ethtool standard stats |
| timestamping, PTP | `ndo_hwtstamp_get`/`set`, PTP clock |
| driver-private | devlink; not a private ioctl |

Advertise an offload only if the hardware implements it for all the cases the stack may hand over. A wrong checksum claim corrupts data silently.

## Contexts And Locks

| Callback | Context | Protection |
| --- | --- | --- |
| `ndo_open`, `ndo_stop` | process, may sleep | RTNL |
| `ndo_start_xmit` | softirq or BH disabled, atomic | transmit queue lock |
| NAPI poll | softirq | one CPU at a time per NAPI instance |
| `ndo_set_rx_mode` | atomic | address list lock; defer sleeping bus access to work |
| `ndo_tx_timeout` | softirq | schedule work for the reset |
| ethtool | process | RTNL |

Devices behind I2C, SPI, or USB cannot transmit from `ndo_start_xmit` directly. They queue the skb and use work or URBs.

## Teardown

`unregister_netdev()` closes the interface and waits until no one uses it. After that the driver may free resources. Do not hold a lock across it that `ndo_stop` also takes.

## Inspecting

```sh
ip -s -d link show eth0
ethtool eth0
ethtool -S eth0
ethtool -i eth0
cat /proc/interrupts | grep eth0
sudo perf trace -e 'napi:*' -a sleep 2
sudo dropwatch -l kas
```

Trace events under `net:` and `napi:` show each transmit, receive, and poll. `skb:kfree_skb` shows drops with a reason.

## Runnable Example

`examples/linux-kernel/small-labs/19-netdev` implements two virtual interfaces joined back to back, with NAPI receive, per-CPU statistics, and ethtool.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `NETDEV WATCHDOG: transmit queue timed out` | queue never woken, or lost completion interrupt | wake logic, IRQ unmask |
| receive stalls until the next packet | unmask race, or poll completed with work left | poll completion path |
| one CPU at 100 % in `ksoftirqd` | poll returns `budget` without doing work | poll return value |
| warning about exceeded weight | poll processed more than `budget` | loop limit |
| memory grows | skb not freed on an error path | kmemleak, drop paths |
| use-after-free | skb touched after the doorbell | order in xmit |
| interface up but no traffic | carrier off, MAC address filter | `ip link`, `ethtool` |
| works at low rate, corrupts at high rate | DMA sync or descriptor ordering | barriers, DMA debug |

## Practice Exercises

1. Run Lab 19, then break it by completing NAPI without the recheck. Use `ping -f` to show the difference.
2. Read a small in-tree driver such as `drivers/net/ethernet/davicom/dm9000.c` and mark context and lock for every callback.
3. Follow one packet from `ndo_start_xmit` to completion in your board's Ethernet driver (`am65-cpsw-nuss.c` or `cpsw_new.c` on TI).

## Debugging Checklist

- Who owns each skb at each line of the transmit path?
- Can the ring become full while the queue is running?
- Does poll respect `budget`, including 0?
- Are interrupts unmasked only after a successful completion?
- Is DMA stopped before buffers are freed?
- Is the carrier state correct?

## Related Topics

- [CAN Driver Integration](can-driver-integration.md)
- [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [Bottom Halves, Softirqs, And Tasklets](../execution-and-concurrency/bottom-halves-softirqs-and-tasklets.md)
- [RCU](../execution-and-concurrency/rcu.md)
- [PCI And PCIe Device Drivers](pci-drivers.md)

## Official References

- [Network devices, the kernel, and you](https://docs.kernel.org/networking/netdevices.html)
- [NAPI](https://docs.kernel.org/networking/napi.html)
- [Softnet driver issues](https://docs.kernel.org/networking/driver.html)
- [phylink](https://docs.kernel.org/networking/sfp-phylink.html)
- [struct sk_buff](https://docs.kernel.org/networking/skbuff.html)
