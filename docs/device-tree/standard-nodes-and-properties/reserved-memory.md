---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Reserved Memory

Device Tree has two related reservation mechanisms. `/memreserve/` entries reserve physical ranges globally in the DTB header, while `/reserved-memory` children describe named regions that can carry policy and be referenced by consumers.

## Choose The Mechanism By Intent

| Mechanism | Representation | Best suited for | Consumer phandle |
|---|---|---|---|
| `/memreserve/` | DTB memory reservation block | coarse early reservation, often firmware-generated | no |
| `/reserved-memory` child | logical tree node | named carveout, shared DMA pool, reusable or unmapped policy | yes |

The mechanisms can coexist. Do not duplicate the same region in both without a documented reason and tested behavior.

## `/memreserve/`

At DTS top level, as in Arm's FVP and Juno-era models (`arch/arm64/boot/dts/arm/fvp-base-revc.dts`):

```dts
/memreserve/ 0x80000000 0x00010000;
```

This encodes a 64-bit address and size in the flattened tree's reservation block. It prevents the kernel from treating that interval as ordinary memory early in boot, but supplies no name, compatible string, allocation policy, or phandle for a device.

Inspect the block with tools that expose the DTB header, for example:

```sh
fdtdump board.dtb
dtc -I dtb -O dts board.dtb
```

Tool output for reservations varies, so keep the original DTB and tool version when collecting evidence.

## Static `/reserved-memory` Regions

A reserved-memory container mirrors the root address/size encoding and has an empty `ranges` property. The AM642 SK (`k3-am642-sk.dts`, plus the R5F consumer from `k3-am64-ti-ipc-firmware.dtsi`):

```dts
reserved_memory: reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        secure_ddr: optee@9e800000 {
                reg = <0x00 0x9e800000 0x00 0x01800000>; /* for OP-TEE */
                alignment = <0x1000>;
                no-map;
        };

        main_r5fss0_core0_dma_memory_region: memory@a0000000 {
                compatible = "shared-dma-pool";
                reg = <0x00 0xa0000000 0x00 0x100000>;
                no-map;
        };

        main_r5fss0_core0_memory_region: memory@a0100000 {
                compatible = "shared-dma-pool";
                reg = <0x00 0xa0100000 0x00 0xf00000>;
                no-map;
        };
};

&main_r5fss0_core0 {
        memory-region = <&main_r5fss0_core0_dma_memory_region>,
                        <&main_r5fss0_core0_memory_region>;
};
```

`reg` defines a fixed physical range. `memory-region` links a consumer to the named reservation; its binding determines how many regions and names are valid. The `ti,k3-r5f-rproc.yaml` binding requires at least two entries, in order: a region for vrings and DMA buffers, then a region for firmware sections. The 24 MiB OP-TEE carveout has no consumer phandle at all; it exists only to keep Linux away from secure-world memory.

`no-map` tells the operating system not to create its normal mapping of the region and not to allow speculative access through that mapping. It does not establish hardware security. Access controls still require firewalls, an IOMMU, privilege separation, or secure-world configuration as appropriate.

`reusable` permits the operating system to use the region temporarily when the owning device can reclaim it. It is inappropriate for firmware state or buffers whose contents must remain untouched. A region should not combine `no-map` and `reusable`, because their ownership models conflict.

## Dynamically Placed Regions

A child can request a size instead of declaring `reg`. The AM62A SK's default CMA region (`k3-am62a7-sk.dts`):

```dts
linux,cma {
        compatible = "shared-dma-pool";
        reusable;
        size = <0x00 0x24000000>;
        alloc-ranges = <0x00 0xc0000000 0x00 0x24000000>;
        linux,cma-default;
};
```

Firmware or the operating system allocates a suitable range subject to `alignment` and `alloc-ranges`. In this real example the `alloc-ranges` window is exactly as large as `size`, so the 576 MiB pool can only land at `0xc0000000`: dynamic in syntax, fixed in effect. If both `reg` and `size` appear, the fixed `reg` definition takes precedence. Dynamic placement reduces hard-coded maps but adds allocation-order and fragmentation considerations; validate the actual assigned address on every supported boot path.

## Reservation Is Not Ownership

A complete design answers four separate questions:

1. **Exclusion:** Which allocator must avoid the range?
2. **Mapping:** May the CPU map or speculate into it?
3. **Access:** Which CPU exception levels and bus masters can access it?
4. **Lifecycle:** Who initializes, lends, reclaims, and clears the contents?

Device Tree reservation primarily addresses the first two. It cannot, by itself, guarantee confidentiality, integrity, cache coherency, or synchronization with remote processors.

## Firmware And UEFI Interactions

Firmware may reserve memory through both Device Tree and its own memory-map protocol. The two descriptions must agree. Architecture boot rules may impose special requirements on how UEFI reservations are represented, especially for `no-map` and `reusable` regions.

For products with several firmware versions, treat the memory map as a versioned ABI. Test overlaps among:

- kernel and initrd placement
- secure firmware and trusted execution environments
- framebuffers and DMA pools
- crash kernels and persistent logs
- remote-processor firmware and shared buffers
- bootloader relocation and runtime services

## Runtime Diagnosis

Use multiple views because each answers a different question:

```sh
dmesg | grep -i -E 'reserved|cma|memory'
cat /proc/iomem
find /sys/firmware/devicetree/base/reserved-memory -maxdepth 2 -type f -print
```

Then inspect the final DTB with `fdtdump`, `fdtget`, or `dtc`. For every region, calculate the half-open interval `[base, base + size)` and check it against RAM banks and all other reservations. Half-open intervals make adjacency and overlap tests unambiguous.

## Review Checklist

- Does the container repeat root cell counts and contain `ranges;`?
- Is every static child named with a unit address matching its first `reg` address?
- Are sizes aligned to every relevant hardware and allocator requirement?
- Is `no-map` backed by actual access-control policy where confidentiality matters?
- Does each device-specific carveout have an explicit `memory-region` relationship?
- Can firmware mutate the reservation, and can support tools capture the result?
- Are stale contents cleared when ownership crosses a security boundary?

## Authoritative References

- [Devicetree Specification releases](https://github.com/devicetree-org/devicetree-specification/releases/tag/v0.4)
- [Linux DMA-BUF heaps userspace API](https://docs.kernel.org/userspace-api/dma-buf-heaps.html)
- [Linux RISC-V boot requirements: reserved memory](https://docs.kernel.org/arch/riscv/boot.html)
- [Linux Devicetree bindings index](https://docs.kernel.org/devicetree/bindings/index.html)

## Next Step

Continue with [Cross-Cutting Standard Relationships](cross-cutting-standard-relationships.md).
