---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# MTD, SPI NOR, And Fixed Partitions

MTD represents raw flash characteristics rather than block-device semantics. A partition table divides that raw address space; it does not provide transactional updates, wear leveling, authentication, or a boot-selection policy.

## SPI NOR Device

```dts
flash@0 {
        compatible = "jedec,spi-nor";
        reg = <0>;
        spi-max-frequency = <50000000>;

        partitions {
                compatible = "fixed-partitions";
                #address-cells = <1>;
                #size-cells = <1>;

                partition@0 {
                        label = "bootloader";
                        reg = <0x000000 0x100000>;
                        read-only;
                };

                partition@100000 {
                        label = "firmware-a";
                        reg = <0x100000 0x700000>;
                };
        };
};
```

The flash node is an SPI target, so its `reg` selects chip select. Inside `fixed-partitions`, each `reg` is flash offset and size. Address meaning changes at the partition-container boundary.

Modern SPI NOR devices often expose discoverable parameters through SFDP, making `jedec,spi-nor` appropriate. Use a part-specific compatible or fixup only when the binding/framework requires it; do not encode a guessed geometry that contradicts discoverable data.

## Partition Geometry

For every region, prove:

- offset and size fit within the physical flash
- partitions do not overlap or leave accidental gaps
- erase boundaries satisfy bootloader and update operations
- redundant metadata has independent erase blocks where required
- boot ROM and bootloader offsets agree with immutable platform behavior
- kernel command line, UBI, filesystem, and updater use the same layout

Schema validation can catch some structural errors but cannot know the boot ROM contract or whether two product variants install different flash capacities.

## `read-only` And `lock`

`read-only` prevents normal MTD writes through that partition in Linux; it is not a security boundary against privileged code, another OS, boot firmware, or direct controller access. Hardware write-protect pins, flash lock bits, secure boot, and lifecycle controls provide different guarantees.

Some partition bindings support `lock`, requesting the MTD layer to lock the region where hardware permits. Confirm unlock/recovery implications before relying on it.

## NAND And UBI

Raw NAND adds bad blocks, ECC, OOB layout, and controller-specific timing. Fixed byte offsets alone do not solve reliable data placement. UBI manages eraseblocks and bad blocks; UBIFS provides a flash-aware filesystem. Their volume/update design is separate from the physical DT partition boundary.

Do not copy a NOR layout to NAND or mount raw NAND as if it were a conventional block device. ECC configuration must match the controller, NAND requirements, boot ROM, and existing on-flash data.

## Update Architecture

An A/B scheme needs more than partitions named `firmware-a` and `firmware-b`. Define:

- authenticated image format
- atomic boot-selection metadata
- rollback and retry counters
- power-loss behavior at every write boundary
- compatibility between bootloader, kernel, rootfs, and DTB
- recovery when one or both slots are invalid

DT should describe the physical layout and binding-defined metadata. Update policy belongs in the boot/update architecture.

## Runtime Diagnosis

```sh
cat /proc/mtd
mtdinfo -a
dmesg | grep -Ei 'mtd|spi-nor|nand|ubi|ecc'
```

For SPI NOR, framework debugfs/sysfs may expose JEDEC ID, SFDP, opcodes, erase types, and protocol widths. Read-only identification is safe; erase/write tests are destructive and require explicit test partitions, verified backups, and a recovery path.

Compare the compiled partition offsets with bootloader environment and manufacturing tools. A board that boots once does not prove update safety.

## Authoritative References

- [Linux SPI NOR framework](https://docs.kernel.org/driver-api/mtd/spi-nor.html)
- [Linux MTD NAND driver API](https://docs.kernel.org/driver-api/mtdnand.html)
- [Linux SPI NOR binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/mtd/jedec,spi-nor.yaml)
- [Linux fixed-partitions schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/mtd/partitions/fixed-partitions.yaml)

## Continue

Proceed to the [Peripheral Integration And Diagnosis Lab](peripheral-integration-and-diagnosis-lab.md).
