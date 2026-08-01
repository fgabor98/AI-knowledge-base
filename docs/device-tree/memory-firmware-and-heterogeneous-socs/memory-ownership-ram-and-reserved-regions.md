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

Build the map from the SoC reference manual, board population, boot firmware, linker maps, and security configuration before reading DTS labels:

| Physical interval | Installed | Linux-managed | Other owner | Purpose |
|---|---:|---:|---|---|
| DRAM bank 0 | yes | mostly | secure firmware | secure runtime |
| DRAM bank 0 carveout | yes | no | R5 firmware | code/data |
| DRAM bank 0 vrings | yes | no or pool | host + R5 | transport |
| on-chip SRAM | yes | driver-specific | boot ROM/remote core | tightly coupled memory |

Check for overlap, alignment, address-cell width, and boot-stage changes. A 64-bit root typically requires two cells for an address and size; copying a four-cell `reg` tuple into a subtree with different cell counts changes its meaning.

## `/memory` Describes Usable Hardware Capacity

A normal RAM node uses `device_type = "memory"` and one or more `reg` ranges:

```dts
memory@80000000 {
        device_type = "memory";
        reg = <0x0 0x80000000 0x0 0x80000000>;
};
```

Platform firmware can adjust installed size before Linux boots. Confirm the final DTB and early boot log rather than assuming the checked-in DTS is the delivered map.

Do not subtract every carveout manually from the `/memory` node. The reserved-memory mechanism exists so RAM topology and exclusions remain independently auditable. Platform-specific firmware may impose additional reservations, but those should also be visible through an authoritative handoff mechanism.

## `/reserved-memory` Is A Container

The conventional shape is:

```dts
reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        r5f_fw: r5f-firmware@9d000000 {
                reg = <0x0 0x9d000000 0x0 0x00800000>;
                no-map;
        };
};
```

The empty `ranges` establishes direct translation into the parent address space. Child nodes describe either a statically placed region with `reg` or a dynamically allocated region with `size` and optional placement constraints. They do not use both.

The node name should describe purpose and include the unit address when `reg` is present. Labels such as `r5f_fw` are source conveniences, not ABI or proof of ownership.

## `memory-region` Connects A Device To A Region

A device-specific binding can reference one or more reserved-memory children:

```dts
remoteproc@5c00000 {
        memory-region = <&r5f_fw>, <&r5f_vring>, <&r5f_buf>;
        memory-region-names = "firmware", "vring", "buffer";
};
```

The consumer binding defines count, order, names, and semantics. Generic reserved-memory syntax does not. Some remoteproc bindings use multiple dedicated phandles or infer purposes from order; never transplant a vendor example without reading that exact schema and driver.

One phandle also does not prove exclusive ownership. Audit every consumer and every firmware image. Accidental aliasing can pass schema and cause intermittent corruption.

## `no-map` Is Narrower Than “Private”

`no-map` tells the OS not to create its normal linear mapping of the region and not to permit speculative access through that mapping. A driver may still deliberately map a permitted region using an appropriate API if the platform contract allows it.

Use `no-map` when the binding and security or hardware contract require it, not as decoration on every carveout. It does not:

- configure a firewall or memory protection controller
- stop another DMA master
- authenticate firmware
- encrypt or erase contents
- transfer ownership between boot stages

Those are separate mechanisms.

## Fixed Placement Versus Allocated Placement

A fixed `reg` region is appropriate when firmware link addresses, hardware windows, secure configuration, or inter-processor ABI require exact placement. A `size`-based reservation lets the early allocator choose a suitable address, optionally restricted by `alignment` and `alloc-ranges`.

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

Useful evidence includes:

```sh
cat /proc/iomem
cat /proc/meminfo
dmesg | grep -Ei 'reserved memory|cma|memblock|memory'
ls -l /sys/firmware/devicetree/base/reserved-memory
```

On a development system, compare these with the decompiled live DTB and bootloader memory map. `/proc/iomem` is an OS view, not a complete security audit.

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
