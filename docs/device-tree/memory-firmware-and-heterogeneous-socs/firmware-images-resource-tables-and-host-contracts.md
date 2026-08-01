---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Firmware Images, Resource Tables, And Host Contracts

DT, a firmware image, and its resource table describe different parts of one system. DT captures stable platform integration; the executable carries build-specific load segments and code; the resource table advertises resources and virtio devices expected by that firmware. Successful boot requires the three to agree.

## Firmware Naming And Delivery

A platform binding may define `firmware-name` or the driver may select a default from the compatible and core identity:

```dts
r5f@5d00000 {
        compatible = "vendor,soc-r5f";
        firmware-name = "product/r5f-control.elf";
};
```

The generic firmware loader searches configured firmware paths and may support platform-specific fallback or built-in firmware. The exact DT property is binding-specific; adding `firmware-name` to an arbitrary node does nothing.

Treat a firmware filename as release configuration:

- identify the hardware and ABI it targets
- version it with host drivers and userspace protocol
- authenticate it according to the product threat model
- make rollback policy explicit
- keep the deployed binary reproducible from a reviewed source and toolchain

Do not use a DTS edit to switch silently between incompatible application protocols.

## ELF Load Segments And Device Addresses

Remoteproc commonly supports ELF firmware. Program headers specify device addresses and sizes for loadable segments; the framework and platform driver translate and copy them into backing memory before start.

Review each segment:

| Field | Question |
|---|---|
| device address | What address does the remote core execute or access? |
| file size | How many initialized bytes are loaded? |
| memory size | How much space including zero-filled data is required? |
| flags | Does placement respect executable/writable policy? |
| alignment | Does it satisfy memory and loader restrictions? |

The highest segment end, not the ELF file size, determines carveout capacity. Sparse device addresses can require multiple regions. Validate integer overflow and containment during deployment, not only after a loader rejects the image.

## The Remoteproc Resource Table

A resource table is a firmware-defined data structure containing entries such as:

- carveout requests
- device-memory mappings
- trace buffers
- virtio devices
- virtqueues associated with virtio devices

The remoteproc core parses supported entries before starting the processor and can allocate or map resources. Platform implementations may also support vendor-specific behavior.

The resource table is not a replacement for DT. It cannot discover fixed clocks, resets, power domains, mailbox wiring, core operating mode, or secure ownership. Conversely, DT should not duplicate dynamic virtio feature negotiation merely because an early firmware build used one layout.

## Reconcile DT And Resource Tables

For every region requested or assumed by firmware, compare:

```text
DT reserved-memory physical base and size
DT consumer memory-region order/name
ELF segment device address and extent
resource-table device/physical address fields and length
platform driver's device-to-physical translation
remote linker script and MMU/MPU configuration
```

Some platform bindings supply fixed carveouts through DT while a resource table identifies how they are used. Some allow the host to allocate addresses. The authoritative rule is the binding plus the platform driver and firmware ABI—not a generic preference.

Duplicate declarations become dangerous when one changes independently. Add a machine-readable consistency check to the firmware packaging pipeline where possible.

## Firmware-Owned Versus Host-Owned Resources

Define who controls each object:

| Object | Common owner | Required agreement |
|---|---|---|
| load carveout | remoteproc during load, remote while running | no concurrent host reuse |
| vring memory | virtio/remoteproc | layout, alignment, addresses |
| RPMsg buffers | virtio transport | ownership transfer and cache policy |
| trace buffer | remote writer, host reader | wrap and synchronization |
| peripheral MMIO | remote or host driver | exclusive or mediated access |
| clock/reset | platform driver or secure firmware | ordered lifecycle |

If both Linux and remote firmware configure the same peripheral registers, the architecture needs an explicit arbitration layer. A resource-table device-memory entry is not permission to take a device away from an active Linux driver.

## ABI Compatibility

Remoteproc boot success proves only that the processor reached a runnable state. It does not prove that:

- the expected RPMsg service exists
- service names match host drivers
- message structures and endianness match
- feature bits are compatible
- shared structures have matching packing and alignment
- timeout and recovery semantics agree

Version application protocols explicitly. Prefer capability negotiation over inferring protocol from a filename. Reject incompatible peers safely before acting on untrusted lengths, offsets, or commands.

## Authentication And Measurement

On systems with secure boot, determine where auxiliary firmware authentication occurs:

- normal-world loader
- trusted firmware or secure monitor
- hardware/ROM authentication engine
- remote core's own boot ROM

DT can identify a firmware channel or platform device, but it is not a trust anchor. If secure firmware owns loading, Linux may only request a boot and observe status. Document anti-rollback, key rotation, failure reporting, and whether crash dumps may expose secrets.

## Diagnose By Phase

1. **request** — correct name, filesystem availability, loader policy
2. **parse** — supported format, ELF class, resource-table integrity
3. **place** — every segment fits and translates
4. **prepare** — power, clocks, reset, IOMMU, firewalls
5. **start** — boot vector and release sequence
6. **announce** — virtio/RPMsg services appear
7. **operate** — protocol and shared-memory behavior remain valid

Preserve the first failure. Repeatedly changing carveout addresses after a parse error only adds variables.

## Authoritative References

- [Linux firmware loader documentation](https://docs.kernel.org/driver-api/firmware/index.html)
- [Linux remoteproc framework and resource-table format](https://docs.kernel.org/staging/remoteproc.html)
- [Linux remoteproc sysfs ABI](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-remoteproc)

## Continue

Proceed to [Remoteproc Topology, Boot, Stop, And Recovery](remoteproc-topology-boot-stop-and-recovery.md).
