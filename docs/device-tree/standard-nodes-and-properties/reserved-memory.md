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

At DTS top level:

```dts
/memreserve/ 0x000000009f000000 0x0000000000100000;
```

This encodes a 64-bit address and size in the flattened tree's reservation block. It prevents the kernel from treating that interval as ordinary memory early in boot, but supplies no name, compatible string, allocation policy, or phandle for a device.

Inspect the block with tools that expose the DTB header, for example:

```sh
fdtdump board.dtb
dtc -I dtb -O dts board.dtb
```

Tool output for reservations varies, so keep the original DTB and tool version when collecting evidence.

## Static `/reserved-memory` Regions

A reserved-memory container mirrors the root address/size encoding and has an empty `ranges` property:

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;

        reserved-memory {
                #address-cells = <2>;
                #size-cells = <2>;
                ranges;

                video_pool: buffer@9e000000 {
                        compatible = "shared-dma-pool";
                        reg = <0x0 0x9e000000 0x0 0x02000000>;
                        reusable;
                };

                secure_fw: firmware@9f000000 {
                        reg = <0x0 0x9f000000 0x0 0x00100000>;
                        no-map;
                };
        };

        accelerator@40000000 {
                memory-region = <&video_pool>;
        };
};
```

`reg` defines a fixed physical range. `memory-region` links a consumer to the named reservation; its binding determines how many regions and names are valid.

`no-map` tells the operating system not to create its normal mapping of the region and not to allow speculative access through that mapping. It does not establish hardware security. Access controls still require firewalls, an IOMMU, privilege separation, or secure-world configuration as appropriate.

`reusable` permits the operating system to use the region temporarily when the owning device can reclaim it. It is inappropriate for firmware state or buffers whose contents must remain untouched. A region should not combine `no-map` and `reusable`, because their ownership models conflict.

## Dynamically Placed Regions

A child can request a size instead of declaring `reg`:

```dts
camera_pool: camera-pool {
        compatible = "shared-dma-pool";
        size = <0x0 0x04000000>;
        alignment = <0x0 0x00200000>;
        alloc-ranges = <0x0 0x80000000 0x0 0x20000000>;
};
```

Firmware or the operating system allocates a suitable range subject to `alignment` and `alloc-ranges`. If both `reg` and `size` appear, the fixed `reg` definition takes precedence. Dynamic placement reduces hard-coded maps but adds allocation-order and fragmentation considerations; validate the actual assigned address on every supported boot path.

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
