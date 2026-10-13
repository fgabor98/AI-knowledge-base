---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Heterogeneous SoC Integration And Recovery Lab

This lab audits the first Cortex-R5F core of the TI AM642 SK (`main_r5fss0_core0`) as Linux sees it: memory reservations, address views, the mailbox doorbell, firmware placement, the TI-SCI-mediated lifecycle, and recovery. The description is real: `k3-am64-main.dtsi`, `k3-am642-sk.dts`, and `k3-am64-ti-ipc-firmware.dtsi` from mainline Linux, with `ti_k3_r5_remoteproc` and `ti_k3_common`.

## Objectives

By the end, you should be able to:

- audit physical memory reservations and their consumers
- distinguish A53 physical, R5F device, and TCM addresses
- reconcile DT regions with ELF segments and the resource table
- derive the remoteproc and RPMsg lifecycle, including IPC-only attach
- identify system firmware as an authority rather than another register provider
- predict the failure signature of five description faults

## Inputs

- `k3-am642-sk.dtb` (it includes the IPC firmware `.dtsi`)
- for Steps 4–8, an AM642 SK with TI's R5F IPC example firmware installed as `/lib/firmware/am64-main-r5f0_0-fw`

```sh
D=k3-am642-sk.dtb
R=/bus@f4000/r5fss@78000000/r5f@78000000
```

## Step 1: Read The Description

The SoC half, from `k3-am64-main.dtsi` (trimmed):

```dts
main_r5fss0: r5fss@78000000 {
        compatible = "ti,am64-r5fss";
        ti,cluster-mode = <0>;
        ranges = <0x78000000 0x00 0x78000000 0x10000>,
                 <0x78100000 0x00 0x78100000 0x10000>,
                 /* core1 TCMs */;
        power-domains = <&k3_pds 119 TI_SCI_PD_EXCLUSIVE>;

        main_r5fss0_core0: r5f@78000000 {
                compatible = "ti,am64-r5f";
                reg = <0x78000000 0x00010000>,
                      <0x78100000 0x00010000>;
                reg-names = "atcm", "btcm";
                ti,sci = <&dmsc>;
                ti,sci-dev-id = <121>;
                ti,sci-proc-ids = <0x01 0xff>;
                resets = <&k3_reset 121 1>;
                firmware-name = "am64-main-r5f0_0-fw";
                ti,atcm-enable = <1>;
                ti,btcm-enable = <1>;
                ti,loczrama = <1>;
        };
};
```

The board half: `optee@9e800000` and the core's two regions in `k3-am642-sk.dts`; `mailbox0_cluster2`, the `mboxes`/`memory-region` amendment, and `main_timer8` marked `status = "reserved"` (“used by r5f0-0”) in `k3-am64-ti-ipc-firmware.dtsi`.

## Step 2: Audit The Memory Map

```sh
fdtget -tx $D /memory@80000000 reg
for n in $(fdtget -l $D /reserved-memory); do
        echo "$n $(fdtget -tx $D /reserved-memory/$n reg)"
done
fdtget -tx $D $R memory-region
```

Build the table from [Memory Ownership](memory-ownership-ram-and-reserved-regions.md#start-with-a-physical-memory-map) yourself and check:

1. every region lies inside the 2 GiB bank and none overlap
2. which phandles the core references, in which order (expected: `memory@a0000000`, then `memory@a0100000`)
3. which region has no consumer at all (`memory@a5000000`) and why it is still reserved
4. where TF-A is (not in DDR on AM64x; see [Secure Reserved Memory](trusted-firmware-op-tee-and-secure-world-boundaries.md#secure-reserved-memory))

## Step 3: Map The Address Views

Fill in one row per memory:

| Memory | A53 physical | R5F device address | Source of the mapping |
|---|---|---|---|
| ATCM (64 KiB) | `0x78000000` | | `ti,loczrama = <1>` |
| BTCM (64 KiB) | `0x78100000` | | `K3_R5_TCM_DEV_ADDR` |
| DMA pool | `0xa0000000` | | vring allocations |
| carveout | `0xa0100000` | | `k3_reserved_mem_init()` |

Expected: with `loczrama` set, ATCM is at R5F address `0x0` and BTCM at `0x41010000`. DDR regions use the same address on both sides, because the driver sets each carveout's device address to its physical address. Explain what breaks if a future SoC put an address translator between the R5F and DDR.

## Step 4: Reconcile The Firmware

```sh
readelf -lW /lib/firmware/am64-main-r5f0_0-fw
readelf -SW /lib/firmware/am64-main-r5f0_0-fw | grep -i resource
```

Every `LOAD` segment must fall inside ATCM, BTCM, or the carveout `[0xa0100000, 0xa1000000)`. `ti_k3_common` expects the resource table at the base of `memory-region` index 1, and reads at most 256 bytes of it in IPC-only mode. Confirm the `.resource_table` section address is `0xa0100000`, and that the vring addresses it declares are either allocated by Linux from the DMA pool or fall inside it.

## Step 5: Trace One Message

Write the path for one RPMsg message from Linux to the R5F, naming each DT element:

```text
rpmsg send -> vring in DMA pool (memory@a0000000)
           -> mailbox0_cluster2, TX FIFO 1, user 2 (mbox-main-r5fss0-core0)
           -> R5F ISR -> reads vring -> replies via FIFO 0 -> Linux RX callback
```

Explain why the R5F firmware's mailbox configuration must be the mirror image of `ti,mbox-rx = <0 0 2>` and `ti,mbox-tx = <1 0 2>`.

## Step 6: Derive Boot And Stop

Linux does not toggle R5F reset registers. It asks system firmware over TI-SCI: `ti,sci-proc-ids` names the processor to request, `ti,sci-dev-id` and `resets` the device and local reset, and the cluster's `power-domains` the power state. Write the cold-start sequence (request processor, power on, load segments, set boot vector, release reset), the stop sequence, and the IPC-only case: if a bootloader already started the core, the driver logs `configured R5F for IPC-only mode` and attaches instead of loading.

## Step 7: Collect Runtime Evidence

```sh
for r in /sys/class/remoteproc/remoteproc*; do
        echo "$r $(cat $r/name) $(cat $r/state) $(cat $r/firmware)"
done
grep -i -E 'mailbox|29020000' /proc/interrupts
ls /sys/bus/rpmsg/devices/
grep -E 'a0000000|a0100000' /proc/iomem
dmesg | grep -Ei 'r5f|remoteproc|rpmsg|virtio|reserved mem|mbox'
```

Map `remoteprocN` to `r5f@78000000` through its `device` link; numbering is probe-order dependent.

## Step 8: Test The Lifecycle

Test cold start from Linux, stop and restart through `/sys/class/remoteproc/remoteprocN/state`, a firmware crash with recovery enabled and disabled, and attach after a bootloader-started core. Record mailbox interrupt counts and RPMsg channel announcements at each step.

## Step 9: Diagnose Deliberate Faults

Apply each to a copy of `$D`. Confirm phandles first (`fdtget -tx $D /reserved-memory/memory@a0000000 phandle`); a mainline build gives `0x25` for the DMA pool and `0x26` for the carveout. `dtc -I dtb -O dts` reports none of these faults.

### Fault A: The Two Regions Are Swapped

```sh
cp $D a.dtb
fdtput -t x a.dtb $R memory-region 26 25
```

The 15 MiB carveout becomes the DMA pool, and the 1 MiB pool becomes the only carveout. Predict that loading fails in the ELF loader with `bad phdr da 0x... mem 0x...`, because the segments linked at `0xa0100000` no longer fall inside any carveout. In IPC-only mode, the driver instead reads the resource table from the wrong base.

### Fault B: Only One Region

```sh
cp $D b.dtb
fdtput -t x b.dtb $R memory-region 25
```

Predict `device needs at least two memory regions to be defined, num = 1` and no remoteproc for this core.

### Fault C: Two Cores' Regions Overlap

```sh
cp $D c.dtb
fdtput -t x c.dtb /reserved-memory/memory@a1000000 reg 0 a0800000 0 100000
```

R5F0_1's DMA pool now lies inside R5F0_0's carveout. Predict the early boot message `OVERLAP DETECTED!` naming both regions, followed by a system that may still boot. Explain why a warning is not protection, and why the unit address now disagrees with `reg` too.

### Fault D: The Mailbox Directions Are Swapped

```sh
cp $D d.dtb
M=/bus@f4000/mailbox@29020000/mbox-main-r5fss0-core0
fdtput -t u d.dtb $M ti,mbox-rx 1 0 2
fdtput -t u d.dtb $M ti,mbox-tx 0 0 2
```

The core loads and reaches `running`, but Linux now transmits on the FIFO the firmware also transmits on. Predict that no RPMsg channels appear under `/sys/bus/rpmsg/devices` and that the mailbox interrupt count stays flat. Explain why `state = running` proves nothing about IPC.

### Fault E: No Firmware Name

```sh
cp $D e.dtb
fdtput -d e.dtb $R firmware-name
```

Predict `failed to parse firmware-name property` and a failed cluster probe, so *both* R5F0 cores disappear, not just core 0.

## Exit Review

The lab is complete when you can present:

- the reserved-memory table with owners, consumers, and the unconsumed RTOS region explained
- the address-view table for TCMs and DDR
- the ELF segment and resource-table reconciliation
- the message path and mailbox mirror contract
- boot, stop, and IPC-only sequences with the TI-SCI identifiers used
- runtime evidence and lifecycle results
- predicted and observed outcomes for each fault

## Authoritative References

- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)
- [Linux RPMsg framework](https://docs.kernel.org/staging/rpmsg.html)
- [Linux DMA API guide](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux generic IOMMU binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/iommu/iommu.txt)

## Continue

Proceed to [Binding Design And Stable ABI](../binding-design-and-stable-abi.md).
