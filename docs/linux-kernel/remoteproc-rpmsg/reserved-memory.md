---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Reserved Memory

## What Problem Does This Solve?

Remote cores and DMA-capable devices often need memory that Linux must not use
as ordinary system RAM. Reserved memory creates a board-level contract for those
regions.

Remoteproc systems commonly reserve memory for:

- firmware text/data loaded by Linux
- remote-core private data
- shared buffers between Linux and firmware
- virtio vrings
- RPMsg payload buffers
- trace buffers
- coredump regions
- DMA pools

If reserved memory is wrong, failures can be severe:

```text
Linux page allocator uses memory that remote firmware also writes
  -> random kernel memory corruption
```

or:

```text
firmware is linked for a carveout larger than Device Tree reserves
  -> remoteproc load fails or overwrites adjacent memory
```

## Core Concepts

### `/reserved-memory`

Device Tree describes reserved memory under the `/reserved-memory` node.

Example:

```dts
/ {
    reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        r5f_dma_memory_region: r5f-dma-memory@9c000000 {
            compatible = "shared-dma-pool";
            reg = <0x0 0x9c000000 0x0 0x100000>;
            no-map;
        };

        r5f_resource_table: r5f-resource-table@9c100000 {
            reg = <0x0 0x9c100000 0x0 0x1000>;
            no-map;
        };
    };
};
```

A device references memory regions with `memory-region`:

```dts
r5f@41000000 {
    compatible = "example,r5f";
    memory-region = <&r5f_dma_memory_region>, <&r5f_resource_table>;
};
```

The exact number, order, names, and semantics of memory regions are defined by
the remoteproc driver's binding.

### Carveout

A carveout is a memory region set aside for remote firmware or a device.

Remoteproc may learn about carveouts from:

- Device Tree `memory-region` properties
- firmware resource tables
- platform driver defaults
- firmware or bootloader handoff

Carveout memory can back:

```text
remote firmware code
remote firmware data
heap
vrings
RPMsg buffers
trace buffers
```

Do not use the term carveout loosely. For debugging, always identify the exact
region, address, size, owner, and cacheability.

### `no-map`

The `no-map` property tells Linux not to create a normal linear kernel mapping
for the region.

Use it when:

- firmware or hardware owns the memory directly
- speculative CPU access could be unsafe
- cacheability must be controlled explicitly
- remoteproc or a platform driver maps the region deliberately

Example:

```dts
trace0: trace@9c200000 {
    reg = <0x0 0x9c200000 0x0 0x4000>;
    no-map;
};
```

`no-map` does not mean the kernel can never access the memory. It means the
memory is not part of the ordinary linear mapping; a driver must map it through
the appropriate API if access is needed.

### `reusable`

Some reserved memory can be marked reusable. This means Linux may use it under
specific conditions when the owner is not using it.

Example:

```dts
linux,cma {
    compatible = "shared-dma-pool";
    reusable;
    size = <0x0 0x4000000>;
    linux,cma-default;
};
```

Do not mark remote firmware memory `reusable` unless the binding and platform
design explicitly allow it. A remote core that can access the region while Linux
reuses it is a corruption bug.

### Shared DMA Pool

`compatible = "shared-dma-pool"` is commonly used for DMA pools and some
remoteproc memory regions.

Example:

```dts
vdev0buffer: vdev0buffer@9c300000 {
    compatible = "shared-dma-pool";
    reg = <0x0 0x9c300000 0x0 0x100000>;
    no-map;
};
```

The meaning depends on the consumer. For one device it may be a coherent DMA
pool. For remoteproc it may be vring or payload memory. Follow the binding.

## Address Spaces

Remoteproc memory debugging requires explicit address vocabulary.

| Address | Owner/View | Example |
| --- | --- | --- |
| physical address | SoC memory map | `0x9c000000` in `/reserved-memory` |
| kernel virtual address | Linux mapping | pointer returned by `memremap()` or DMA API |
| DMA address | bus address used by a device | may be affected by IOMMU or DMA ranges |
| device address | remote core firmware view | address in linker script/resource table |

A remote core might see memory at a different device address from its physical
address.

Example:

```text
Linux physical address:
  0x9c000000

remote core device address:
  0x80000000

translation:
  platform remoteproc driver maps da 0x80000000 to pa 0x9c000000
```

If that translation is wrong, firmware may load correctly from Linux's point of
view but execute from the wrong address on the remote core.

## Firmware Linker Script Contract

Remote firmware is usually linked for specific memory addresses.

Firmware linker script:

```ld
MEMORY
{
    DDR_CODE (rx)  : ORIGIN = 0x80000000, LENGTH = 512K
    DDR_DATA (rwx) : ORIGIN = 0x80080000, LENGTH = 512K
    SHMEM    (rw)  : ORIGIN = 0x80100000, LENGTH = 1M
}
```

Device Tree:

```dts
reserved-memory {
    r5f_code_data: r5f-code-data@9c000000 {
        reg = <0x0 0x9c000000 0x0 0x100000>;
        no-map;
    };

    r5f_shmem: r5f-shmem@9c100000 {
        reg = <0x0 0x9c100000 0x0 0x100000>;
        no-map;
    };
};
```

Compatibility requires:

- enough size for each segment
- correct alignment
- correct address translation
- no overlap with Linux memory or other carveouts
- matching cacheability assumptions
- agreement on which side owns each region

## Resource Table And Memory

Firmware resource tables may request carveouts dynamically or describe fixed
resources.

Conceptual resource table entries:

```text
RSC_CARVEOUT:
  remote firmware needs memory at device address X, size Y

RSC_TRACE:
  trace buffer address and size

RSC_VDEV:
  virtio device with vrings at given addresses or dynamically allocated
```

Remoteproc may allocate memory for some resources or match them to reserved
memory depending on platform support and firmware expectations.

Debug question:

```text
Did the resource table request memory that Device Tree did not reserve?
Did Device Tree reserve memory that the firmware does not use?
Are vring addresses fixed or allocated?
```

## Virtio/RPMsg Memory

RPMsg over virtio uses shared memory:

```text
vring0: Linux -> remote descriptors
vring1: remote -> Linux descriptors
buffers: message payload storage
```

The memory must be:

- visible to both Linux and remote core
- correctly aligned for vrings
- large enough for the selected number and size of buffers
- coherent or explicitly synchronized according to platform rules
- not reused by Linux page allocator

If RPMsg channels appear but messages corrupt or hang, inspect vring memory,
mailbox interrupts, and cache coherency before changing application protocol.

## Cacheability And Coherency

Shared memory correctness depends on cache behavior.

Possible models:

| Model | Implication |
| --- | --- |
| hardware coherent | Linux and remote see updates through coherent interconnect |
| non-coherent with DMA API | Linux must use DMA mapping/sync APIs correctly |
| uncached mapping | easier visibility, often slower |
| manual firmware cache maintenance | remote firmware must clean/invalidate at ownership boundaries |

Bad pattern:

```text
Linux writes shared buffer through cached mapping
Linux kicks remote core
remote reads stale data
```

or:

```text
remote writes response
Linux reads cached old value
```

Use the subsystem's transport APIs where possible. If you design raw shared
memory, define ownership, barriers, and cache maintenance explicitly.

## Overlap And Alignment

Reserved memory regions must not overlap:

```text
region A: 0x9c000000 - 0x9c0fffff
region B: 0x9c080000 - 0x9c17ffff
          overlap
```

They must also fit inside actual RAM and respect hardware alignment:

- page alignment for Linux mappings
- vring alignment required by virtio
- MPU/MMU region alignment on the remote core
- cacheline alignment for shared data
- firmware loader alignment requirements

Review DTS ranges manually. Do not rely only on whether the system boots once.

## Bootloader Reservations

Bootloaders can reserve, load, or protect memory before Linux boots.

Check:

- U-Boot environment and boot scripts
- firmware-loaded remote core memory
- `/memreserve/` entries in the DTB
- `/reserved-memory` nodes
- secure monitor reserved regions
- crash log or ramoops regions

Problem:

```text
bootloader loads remote firmware into DDR
Linux DTB does not reserve that DDR
Linux allocator reuses it
remote firmware eventually corrupts Linux memory
```

When a core is bootloader-started, Linux must still know which memory the remote
core owns.

## Runtime Inspection

Inspect reserved memory in the running DTB:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg -n 'reserved-memory|memory-region|no-map|shared-dma-pool' /tmp/running.dts
```

Check boot logs:

```sh
dmesg | grep -Ei 'reserved|carveout|remoteproc|rproc|cma'
```

Check system memory map:

```sh
cat /proc/iomem
```

Remoteproc logs often identify carveouts:

```sh
dmesg | grep -Ei 'carveout|vring|vdev|trace'
```

## Debugging Memory Corruption

Symptoms:

- random kernel crashes after remote firmware starts
- crashes only under RPMsg traffic
- remote firmware crashes when Linux allocates memory
- corrupted messages or stale shared-memory values
- remoteproc load failure around segment placement

Workflow:

1. Stop the remote core and see whether corruption disappears.
2. Compare firmware map file with `/reserved-memory`.
3. Check for overlaps between all carveouts.
4. Verify `no-map` and `reusable` policy.
5. Confirm address translation between device address and physical address.
6. Review cacheability and ownership transitions.
7. Enable DMA/API/debug tooling where relevant.
8. Use hardware tracing or memory watchpoints if available.

## Common Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| carveout too small | firmware load failure or overwrite | increase region and update firmware map |
| missing `no-map` | speculative/cached Linux access to remote-owned memory | add `no-map` when required by binding/design |
| accidental `reusable` | Linux reuses remote-owned memory | remove `reusable` |
| wrong memory-region order | platform driver assigns regions incorrectly | follow binding order/names |
| address translation mismatch | remote executes wrong memory | fix driver mapping or linker script |
| vring memory not aligned | RPMsg/virtio failure | align per virtio/platform requirements |
| shared buffer cache bug | stale or corrupted messages | use proper cache maintenance/API |
| bootloader memory not reserved | random corruption after boot | reserve all bootloader-started remote memory |
| overlapping carveouts | nondeterministic failures | fix DTS memory map |

## Practice Exercises

1. Dump the running Device Tree and list every `/reserved-memory` child.
2. For one remoteproc node, map each `memory-region` phandle to address and
   size.
3. Compare a firmware linker map against the reserved-memory layout.
4. Identify which regions are code/data, vrings, buffers, trace, or coredump.
5. Deliberately reduce a lab carveout size and capture the remoteproc failure
   message.

## Review Checklist

- Are all remote-owned memory regions reserved from Linux?
- Do firmware linker addresses match the platform address translation?
- Are carveout sizes and alignments sufficient?
- Are `no-map` and `reusable` used correctly?
- Are vring and RPMsg buffer regions visible to both sides?
- Is cacheability/coherency defined for shared memory?
- Are bootloader-started remote-core regions reserved too?
- Does the binding define memory-region order or names, and does DTS follow it?

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [Remoteproc Framework](remoteproc-framework.md)
- [Virtio And RPMsg](virtio-rpmsg.md)

## Official References

- [Reserved Memory Binding](https://docs.kernel.org/devicetree/bindings/reserved-memory/reserved-memory.yaml)
- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
