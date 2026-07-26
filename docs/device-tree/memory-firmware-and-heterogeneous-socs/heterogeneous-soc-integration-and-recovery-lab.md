---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Heterogeneous SoC Integration And Recovery Lab

This lab integrates a fictional Cortex-R5 remote processor with fixed firmware memory, RPMsg vrings and buffers, a mailbox pair, a system IOMMU, and a secure-firmware-mediated lifecycle. It focuses on proving ownership and address contracts, then recovering safely from faults.

## Objectives

By the end, you should be able to:

- audit physical memory reservations and consumers
- distinguish host physical, DMA/IOMMU, and remote device addresses
- reconcile DT regions with ELF segments and a resource table
- derive the remoteproc and RPMsg lifecycle
- identify secure firmware as an authority rather than another register provider
- gather evidence for boot, transport, coherency, and recovery failures

## Hardware Contract

Assume:

- 2 GiB of DRAM begins at CPU physical address `0x80000000`
- the R5 has a 32-bit device-address view
- system firmware owns the R5 reset and authenticates its image
- Linux stages and manages nonsecure memory, then requests start/stop
- R5 device address `0x9d000000` maps directly to the same CPU physical address
- firmware code/data uses 8 MiB from `0x9d000000`
- two vrings and their guard/alignment allowance use 1 MiB from `0x9d800000`
- RPMsg buffers use 4 MiB from `0x9d900000`
- an SMMU stream context constrains R5 DMA to approved mappings
- mailbox channel 4 kicks the remote, and channel 5 notifies the host

The compatible strings and property names below are illustrative. A real board must use its platform remoteproc, system-firmware, IOMMU, and mailbox schemas.

## Step 1: Read The Description

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;

        memory@80000000 {
                device_type = "memory";
                reg = <0x0 0x80000000 0x0 0x80000000>;
        };

        reserved-memory {
                #address-cells = <2>;
                #size-cells = <2>;
                ranges;

                r5f_fw: r5f-firmware@9d000000 {
                        reg = <0x0 0x9d000000 0x0 0x00800000>;
                        no-map;
                };

                r5f_vrings: r5f-vrings@9d800000 {
                        reg = <0x0 0x9d800000 0x0 0x00100000>;
                        no-map;
                };

                r5f_buffers: r5f-buffers@9d900000 {
                        compatible = "shared-dma-pool";
                        reg = <0x0 0x9d900000 0x0 0x00400000>;
                        no-map;
                };
        };
};

r5f@5d00000 {
        compatible = "example,soc-r5f-secure-rproc";
        reg = <0x0 0x05d00000 0x0 0x10000>;
        firmware-name = "product/r5f-control.elf";
        memory-region = <&r5f_fw>, <&r5f_vrings>, <&r5f_buffers>;
        memory-region-names = "firmware", "vrings", "buffers";
        mboxes = <&mailbox0 4>, <&mailbox0 5>;
        mbox-names = "tx", "rx";
        iommus = <&smmu 0x42>;
        power-domains = <&system_firmware 27>;
        status = "okay";
};
```

In a real secure-lifecycle binding, the node might not expose direct registers or might use a firmware-specific handle. Never infer those details from this example.

## Step 2: Prove The Physical Layout

Calculate each half-open interval:

```text
DRAM        [0x80000000, 0x100000000)
firmware    [0x9d000000, 0x9d800000)
vrings      [0x9d800000, 0x9d900000)
buffers     [0x9d900000, 0x9dd00000)
```

All carveouts lie within DRAM, are adjacent but do not overlap, and consume 13 MiB total. Confirm that boot firmware does not reserve a conflicting interval and that Linux reports all three exclusions.

Explain why the `/memory` node still spans the complete installed bank: `/reserved-memory` records exclusions independently.

## Step 3: Build The Ownership Matrix

| Resource | Before load | During load | Running | After contained stop |
|---|---|---|---|---|
| firmware carveout | Linux reserved | host loader writes | R5 executes/uses | host may reload, not page allocator |
| vrings | remoteproc reserved | host initializes | virtio shared | reinitialize before restart |
| RPMsg buffers | pool reserved | transport prepares | ownership alternates | reclaim after DMA containment |
| reset/power | system firmware | system firmware | system firmware policy | system firmware |
| SMMU context | host/secure cooperation | mapped | enforces aperture | revoke after quiescence |

The static carveouts remain excluded after stop; “host owns for reload” does not mean ordinary pages may use them.

## Step 4: Reconcile Firmware Metadata

Assume `readelf -l r5f-control.elf` reports:

```text
LOAD device 0x9d000000 filesz 0x00180000 memsz 0x00200000
LOAD device 0x9d300000 filesz 0x00080000 memsz 0x00100000
```

Both segments fit within the 8 MiB firmware region, with a gap. Verify the entry point also lies in executable remote memory.

Then inspect the resource table and record:

- two vring device addresses and sizes within the vring interval
- alignment and descriptor count
- RPMsg virtio device and feature bits
- buffer-pool expectations
- trace entry, if any

Reject an image whose resource entry overlaps an ELF segment or extends beyond its DT region before requesting secure authentication.

## Step 5: Trace One Message

Write the full path:

```text
host RPMsg payload
  -> transport buffer allocated from r5f_buffers
  -> host writes payload
  -> DMA/cache ownership transition
  -> host publishes descriptor in r5f_vrings
  -> memory barrier
  -> mailbox channel 4 kick
  -> R5 reads descriptor using its device address
  -> R5 consumes payload and publishes completion
  -> mailbox channel 5 interrupt
  -> host regains buffer ownership
```

For this direct-mapped example, the remote device and CPU physical numbers match. The host must still use the DMA/remoteproc APIs because the SMMU and cache policy can make the operational DMA address different.

## Step 6: Derive Boot And Stop

A plausible boot is:

1. remoteproc driver binds after mailbox, IOMMU, and firmware providers
2. request and parse the ELF and resource table
3. validate segments and resource entries against carveouts
4. create required SMMU mappings
5. load or stage segments and initialize vrings
6. request system firmware to authenticate and start the R5
7. wait for readiness and virtio/RPMsg announcement
8. bind the version-compatible RPMsg service

A safe stop is:

1. reject new application requests
2. request cooperative remote shutdown
3. wait for or force containment through system firmware
4. stop independent DMA and mask notifications
5. resolve or revoke outstanding shared buffers
6. tear down transport and SMMU mappings
7. retain diagnostic evidence and return offline

Actual order is platform-specific, but no buffer can return to another owner while an active master can still write it.

## Step 7: Collect Runtime Evidence

```sh
cat /proc/iomem
dmesg | grep -Ei 'reserved memory|remoteproc|rproc|firmware|iommu|rpmsg|virtio|mailbox'
ls -l /sys/class/remoteproc
ls -l /sys/bus/virtio/devices
ls -l /sys/bus/rpmsg/devices
cat /proc/interrupts
```

Also capture firmware build identity, remote trace where safe, IOMMU mappings/faults, and the live DT properties. Use platform-approved tools for secure-firmware state; do not bypass it through raw register writes.

## Step 8: Test The Lifecycle

Exercise:

1. cold boot with the R5 initially off
2. repeated start/stop and service bind/unbind
3. maximum-rate bidirectional messages through ring wrap
4. system suspend/resume with the R5 stopped
5. supported low-power behavior with the R5 running
6. remote watchdog crash at each buffer-ownership state
7. incompatible and invalidly signed firmware
8. warm reset and kexec according to product policy

Verify memory remains protected during every failure and recovery is bounded.

## Step 9: Diagnose Deliberate Faults

### Fault A: Firmware Segment Ends At `0x9d900000`

The segment crosses the firmware carveout and consumes the vring region. Reject the image. Enlarging only the firmware DTS node would then overlap the next static region; memory layout and firmware must change together.

### Fault B: Resource Table Uses R5 Address `0x9e800000` For A Vring

The entry is outside the DT vring region. Determine whether the firmware used another product's linker/resource configuration or whether a documented remote address translation was omitted. Do not truncate the address.

### Fault C: Only Mailbox TX Interrupts Increase

The host can kick the remote, but there is no receive notification. Check remote boot progress, RX channel mapping, remote interrupt routing, queue address, and memory ordering. An RPMsg driver change is premature.

### Fault D: Messages Corrupt Only Under Load

Audit buffer ownership, barriers, cache policy, vring alignment, and ring wrap. Adding sleeps is not a fix. Capture the first corrupt descriptor and both peers' indices.

### Fault E: An SMMU Fault Reports Stream `0x42`

Correlate the faulting IOVA and access type with the active RPMsg buffer and mappings. Check address domain, length, permissions, and stale DMA after stop. Disabling the SMMU removes evidence and isolation.

### Fault F: Restart Works Once, Then Times Out

Inspect stale mailbox notifications, remote watchdog state, incomplete reset, retained vring indices, power references, and unrevoked mappings. Reinitialize transport only after the remote and every DMA master are contained.

### Fault G: Secure Firmware Rejects The Image

Preserve its status code and firmware identity. Verify signature, key policy, rollback counter, target core, and staging limits. Do not fall back to an unauthenticated direct-load path.

## Exit Review

The lab is complete when you can provide:

- a nonoverlapping physical memory map
- ownership state for every region and lifecycle phase
- ELF/resource-table/DT consistency report
- one end-to-end address and cache-ownership trace
- boot, stop, crash, and recovery sequences
- runtime evidence for remoteproc, virtio, RPMsg, mailbox, and IOMMU layers
- root-cause reasoning for every deliberate fault
- security justification for refusing unsafe workarounds

## Authoritative References

- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)
- [Linux RPMsg framework](https://docs.kernel.org/staging/rpmsg.html)
- [Linux DMA API guide](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux generic IOMMU binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/iommu/iommu.txt)

## Continue

Proceed to [U-Boot And Bootloader Device Tree](../u-boot-and-bootloader-device-tree.md).
