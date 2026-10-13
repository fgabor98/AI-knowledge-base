---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Memory Ownership, RAM, And Reserved Regions

A memory address is not an ownership policy. The `/memory` nodes tell the OS which physical RAM exists; `/reserved-memory` removes or constrains selected regions for a documented purpose. Drivers, firmware, and security policy still need a lifecycle contract for every region.

## Start With A Physical Memory Map

Build the map from the SoC reference manual, board population, boot firmware, linker maps, and security configuration before reading DTS labels. For the AM642 SK, `k3-am642-sk.dts` declares `memory@80000000` with 2 GiB and, together with `k3-am64-ti-ipc-firmware.dtsi`, reserves:

| Physical interval | Size | Owner | Purpose |
|---|---:|---|---|
| `0x9e800000`–`0x9fffffff` | 24 MiB | OP-TEE | secure OS (`optee@9e800000`) |
| `0xa0000000`–`0xa00fffff` | 1 MiB | Linux + R5F0_0 | vring/virtio DMA pool |
| `0xa0100000`–`0xa0ffffff` | 15 MiB | R5F0_0 | firmware code and data |
| `0xa1000000`–`0xa3ffffff` | 3 × 16 MiB | R5F0_1, R5F1_0, R5F1_1 | same 1 + 15 MiB split per core |
| `0xa4000000`–`0xa4ffffff` | 16 MiB | Linux + M4F | same split for the MCU M4F |
| `0xa5000000`–`0xa57fffff` | 8 MiB | remote cores only | RTOS-to-RTOS IPC |

The 64-bit TF-A BL31 is absent from this table. TI U-Boot's `am64x_evm_a53_defconfig` sets `CONFIG_K3_ATF_LOAD_ADDR=0x701c0000`, which is on-chip SRAM, not DDR. OP-TEE's address, `CONFIG_K3_OPTEE_LOAD_ADDR`, defaults to `0x9e800000`, matching the `optee` reservation. The DT and the bootloader configuration must agree, and nothing checks that automatically.

Check for overlap, alignment, address-cell width, and boot-stage changes. A 64-bit root typically requires two cells for an address and size; copying a four-cell `reg` tuple into a subtree with different cell counts changes its meaning.

## `/memory` And `/reserved-memory` Stay Separate

`/memory` and the `/reserved-memory` container are described in [CPUs, Topology, And Memory](../standard-nodes-and-properties/cpus-topology-and-memory.md#the-memory-node) and [Reserved Memory](../standard-nodes-and-properties/reserved-memory.md). Do not subtract every carveout manually from `/memory`: the reserved-memory mechanism exists so RAM topology and exclusions remain independently auditable, and firmware may adjust installed size before Linux boots. Name each child by purpose with its unit address; labels such as `r5f_fw` are source conveniences, not ABI or proof of ownership.

## `memory-region` Connects A Device To A Region

A device-specific binding can reference one or more reserved-memory children. From `k3-am64-ti-ipc-firmware.dtsi`:

```dts
&main_r5fss0_core0 {
        mboxes = <&mailbox0_cluster2 &mbox_main_r5fss0_core0>;
        memory-region = <&main_r5fss0_core0_dma_memory_region>,
                        <&main_r5fss0_core0_memory_region>;
        status = "okay";
};
```

There is no `memory-region-names`; order carries the meaning. `k3_reserved_mem_init()` in `drivers/remoteproc/ti_k3_common.c` requires at least two regions, turns index 0 into the device's DMA pool for vrings, and maps every later index as a static carveout. It also uses the carveout's physical address as the remote core's device address, so the R5F must see DDR at the same address the A53 does. A firmware that expects its resource table elsewhere, or a board file that swaps the two entries, breaks this contract while still passing the schema.

The consumer binding defines count, order, names, and semantics; generic reserved-memory syntax does not. Never transplant a vendor example without reading that exact schema and driver.

One phandle also does not prove exclusive ownership. Audit every consumer and every firmware image. Accidental aliasing can pass schema and cause intermittent corruption.

## `no-map` Is Narrower Than “Private”

[`no-map`](../standard-nodes-and-properties/reserved-memory.md#static-reserved-memory-regions) only removes the OS's normal linear mapping; a driver may still deliberately map a permitted region. Use it when the binding, security, or hardware contract requires it, not as decoration on every carveout. It does not:

- configure a firewall or memory protection controller
- stop another DMA master
- authenticate firmware
- encrypt or erase contents
- transfer ownership between boot stages

## Fixed Placement Versus Allocated Placement

Use a fixed `reg` only when firmware link addresses, hardware windows, secure configuration, or inter-processor ABI require exact placement; otherwise a [dynamically placed region](../standard-nodes-and-properties/reserved-memory.md#dynamically-placed-regions) avoids hard-coded maps.

Ask whether the address is truly ABI:

- Does an ELF segment contain a fixed device address?
- Does the remote core lack address translation?
- Does ROM or secure firmware validate a fixed range?
- Does a hardware accelerator expose only a narrow window?
- Can the resource table or host communicate the chosen address?

Do not freeze placement merely because the first prototype happened to work there.

## Ownership Transitions

Record the expected state at:

1. reset and ROM execution
2. first-stage bootloader
3. trusted firmware initialization
4. U-Boot or another rich bootloader
5. Linux early memory initialization
6. remoteproc load and start
7. crash and recovery
8. suspend, warm reset, and kexec

For each transition, define who quiesces writers, flushes or invalidates caches, changes firewalls, initializes contents, and erases secrets. A reserved-memory node is static; ownership can be dynamic.

## Runtime Evidence

Collect the views listed in [Reserved Memory: Runtime Diagnosis](../standard-nodes-and-properties/reserved-memory.md#runtime-diagnosis) and compare them with the decompiled live DTB and bootloader memory map. `/proc/iomem` is an OS view, not a complete security audit.

## Review Checklist

- every installed bank is described once and with correct cell widths
- reserved regions are entirely inside actual memory or documented device memory
- no two regions overlap unless an explicit shared contract says so
- every carveout has a named owner and purpose outside its source label
- `no-map`, fixed placement, alignment, and allocation constraints are justified
- every `memory-region` tuple is decoded by the consumer binding
- boot, crash, suspend, kexec, and warm-reset ownership are defined

## Authoritative References

- [Devicetree Specification: memory node](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html#memory-node)
- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux boot-time memory management](https://docs.kernel.org/core-api/boot-time-mm.html)

## Continue

Proceed to [CMA, Shared DMA Pools, And Static Carveouts](cma-shared-dma-pools-and-static-carveouts.md).
