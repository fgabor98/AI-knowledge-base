---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# RPMsg, Mailboxes, Virtqueues, And Shared Memory

Remoteproc controls a processor lifecycle. Virtio provides transport devices. RPMsg carries messages over that transport. Mailboxes or doorbells notify the peer that shared queue state changed. These layers cooperate, but they are not interchangeable.

## Layer The Communication Path

```text
application protocol
  -> RPMsg endpoint and channel
  -> RPMsg virtio transport
  -> virtqueues/vrings and shared buffers
  -> mailbox, interrupt, or doorbell notification
  -> remote firmware transport implementation
```

A mailbox interrupt carries little or no payload in many designs; the actual descriptors and messages live in shared memory. A functioning mailbox test does not prove vring addresses or cache handling are correct.

## RPMsg Channels And Endpoints

RPMsg peers communicate using local and destination addresses. Firmware can announce named services, allowing matching RPMsg drivers to bind. The service name is a software ABI and should be versioned and constrained like any externally supplied identifier.

An endpoint is not a DT graph endpoint. It is a runtime messaging address managed by the RPMsg framework. Avoid adding child DT nodes for dynamically announced services unless the platform has a documented binding for static channels.

The host driver must validate every message:

- source and destination expectations
- exact minimum and maximum length
- version and feature fields
- integer offsets and counts
- object lifecycle and authorization

Remote firmware may be buggy or compromised. RPMsg can expose access to physical devices, memory, or privileged host services, so channel access is a security boundary.

## Virtqueues And Vrings

Virtqueues use shared descriptor rings. Their resource-table description includes device addresses, alignment, entry count, and notification identifiers. The host allocates or resolves backing memory and communicates usable addresses according to the remoteproc/virtio contract.

Check:

- each vring lies completely inside its assigned region
- alignment and entry count match both implementations
- address fields fit the remote processor
- descriptor and payload memory cache policy is compatible
- notification IDs map to the intended mailbox/doorbell
- queue indices survive wraparound and restart correctly

A mismatch can allow the first few messages and fail only after ring wrap or concurrent traffic.

## Mailboxes And Interrupts

A remoteproc binding may use `mboxes` and `mbox-names` for transmit, receive, kick, or control channels:

```dts
remoteproc@5d00000 {
        mboxes = <&mailbox0 4>, <&mailbox0 5>;
        mbox-names = "tx", "rx";
};
```

This is illustrative. The remoteproc and mailbox-provider schemas define the count, names, and specifier. Some SoCs use dedicated interrupts, system-controller calls, or interrupt-router events instead.

Trace both directions:

```text
host writes descriptor -> barrier/cache sync -> host kick -> remote ISR
remote updates used ring -> barrier/cache sync -> remote kick -> host ISR
```

If ordering is wrong, extra logging can accidentally make the problem disappear.

## Shared-Memory Ownership

For every buffer, define one of these states:

```text
host-owned -> published to remote -> remote-owned -> returned to host
```

Only the owner writes data and metadata unless the protocol defines finer-grained ownership. Memory barriers order metadata against payload. Cache maintenance makes noncoherent copies visible. They solve different problems.

Do not place ordinary shared buffers in a `reusable` region while firmware can retain pointers indefinitely. Do not let automatic crash recovery reuse buffers still accessible by an uncontained DMA engine.

## Zero Copy And Large Payloads

RPMsg has finite transport buffers and is best suited to control messages and bounded payloads. Large data paths often exchange handles, offsets, or descriptors referring to separately managed DMA buffers.

That design needs:

- a trusted allocator
- IOMMU mapping and access permissions
- lifetime and reference rules
- cache synchronization
- bounds validation
- cancellation and crash cleanup
- an ABI for address-space interpretation

Sending a Linux virtual or physical pointer is never a portable protocol.

## Diagnose From The Bottom Up

1. remote processor is running
2. resource table produced a virtio device
3. both vrings resolved to valid memory
4. notification interrupts increment in both directions
5. RPMsg name service announced the expected channel
6. host driver matched the service
7. endpoint addresses and protocol version agree
8. sustained bidirectional traffic preserves data and ownership

Useful evidence may include:

```sh
ls -l /sys/bus/virtio/devices
ls -l /sys/bus/rpmsg/devices
cat /proc/interrupts
dmesg | grep -Ei 'rpmsg|virtio|vring|mailbox|remoteproc'
```

Enable dynamic debug selectively for the relevant drivers. Excessive tracing changes timing and can obscure an ordering defect.

## Stress And Recovery Tests

- ring wrap under bidirectional maximum-rate traffic
- variable message sizes including invalid lengths
- delayed or lost notification where hardware permits simulation
- remote crash with every buffer-ownership state
- host service unbind/rebind
- repeated remote start/stop
- suspend/resume during idle and traffic
- incompatible firmware service version

After recovery, no stale channel should remain bound and no old notification should be mistaken for a new queue event.

## Authoritative References

- [Linux RPMsg framework](https://docs.kernel.org/staging/rpmsg.html)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)
- [Linux mailbox framework](https://docs.kernel.org/driver-api/mailbox.html)
- [Linux generic mailbox binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/mailbox/mailbox.txt)
- [Virtio specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html)

## Continue

Proceed to [PRU, R5/M4, DSP, And Cluster Modeling](pru-r5-m4-dsp-and-cluster-modeling.md).
