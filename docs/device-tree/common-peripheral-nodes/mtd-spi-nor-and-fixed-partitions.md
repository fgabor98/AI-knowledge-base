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

The AM642 EVM boots from a 64 MiB octal SPI NOR on `ospi0`. From `k3-am642-evm.dts`:

```dts
&ospi0 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&ospi0_pins_default>;

        flash@0 {
                compatible = "jedec,spi-nor";
                reg = <0x0>;
                spi-tx-bus-width = <8>;
                spi-rx-bus-width = <8>;
                spi-max-frequency = <25000000>;
                cdns,tshsl-ns = <60>;
                cdns,tsd2d-ns = <60>;
                cdns,tchsh-ns = <60>;
                cdns,tslch-ns = <60>;
                cdns,read-delay = <4>;

                partitions {
                        compatible = "fixed-partitions";
                        #address-cells = <1>;
                        #size-cells = <1>;

                        partition@0 {
                                label = "ospi.tiboot3";
                                reg = <0x0 0x100000>;
                        };

                        partition@100000 {
                                label = "ospi.tispl";
                                reg = <0x100000 0x200000>;
                        };

                        partition@300000 {
                                label = "ospi.u-boot";
                                reg = <0x300000 0x400000>;
                        };

                        partition@700000 {
                                label = "ospi.env";
                                reg = <0x700000 0x40000>;
                        };

                        partition@740000 {
                                label = "ospi.env.backup";
                                reg = <0x740000 0x40000>;
                        };

                        partition@800000 {
                                label = "ospi.rootfs";
                                reg = <0x800000 0x37c0000>;
                        };

                        partition@3fc0000 {
                                label = "ospi.phypattern";
                                reg = <0x3fc0000 0x40000>;
                        };
                };
        };
};
```

The flash node is an SPI target, so its `reg` selects chip select 0. Inside `fixed-partitions`, each `reg` is flash offset and size. Address meaning changes at the partition-container boundary. The `cdns,*` properties are per-device timing for the Cadence OSPI controller (chip-select setup and hold times, read capture delay), so they sit on the flash child, not the controller.

`jedec,spi-nor` is enough because the framework reads the JEDEC ID and SFDP tables to learn size, erase types, and octal opcodes. Use a part-specific compatible only when the binding or framework requires it; do not encode a guessed geometry that contradicts discoverable data.

## Partition Geometry

For every region, prove:

- offset and size fit within the physical flash
- partitions do not overlap or leave accidental gaps
- erase boundaries satisfy bootloader and update operations
- redundant metadata has independent erase blocks where required
- boot ROM and bootloader offsets agree with immutable platform behavior
- kernel command line, UBI, filesystem, and updater use the same layout

Apply this to the EVM layout. The boot ROM loads `tiboot3` from offset 0; TI U-Boot's `am64x_evm_r5_defconfig` and `am64x_evm_a53_defconfig` set `CONFIG_SYS_SPI_U_BOOT_OFFS` to `0x100000` and `0x300000`, matching `ospi.tispl` and `ospi.u-boot`. The last partition ends at `0x4000000`, exactly 64 MiB. `ospi.env.backup` ends at `0x780000` and `ospi.rootfs` starts at `0x800000`, leaving a 512 KiB hole. Decide whether a hole like this is reserved space or an accident, and record that decision.

None of the partitions is `read-only`, so Linux can rewrite the boot chain through `/dev/mtdN`. That is deliberate on an EVM and a decision to revisit for a product.

Schema validation can catch some structural errors but cannot know the boot ROM contract or whether two product variants install different flash capacities.

## `read-only` And `lock`

`read-only` prevents normal MTD writes through that partition in Linux; it is not a security boundary against privileged code, another OS, boot firmware, or direct controller access. Hardware write-protect pins, flash lock bits, secure boot, and lifecycle controls provide different guarantees.

Some partition bindings support `lock`, requesting the MTD layer to lock the region where hardware permits. Confirm unlock/recovery implications before relying on it.

## NAND And UBI

Raw NAND adds bad blocks, ECC, OOB layout, and controller-specific timing. Fixed byte offsets alone do not solve reliable data placement. UBI manages eraseblocks and bad blocks; UBIFS provides a flash-aware filesystem. Their volume/update design is separate from the physical DT partition boundary.

Do not copy a NOR layout to NAND or mount raw NAND as if it were a conventional block device. ECC configuration must match the controller, NAND requirements, boot ROM, and existing on-flash data.

## Update Architecture

An A/B scheme needs more than two partitions with `-a` and `-b` labels. Define:

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
