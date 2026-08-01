---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Minimal DTB, Boot Handoff, Memory, Console, And Boot Storage

The first custom DTB should answer only four questions: did firmware select the intended board artifact, does Linux understand CPU/RAM basics, can it print reliably, and can it reach the boot/root storage path? Every optional node adds dependencies and noise.

## Start From The SoC Description

Prefer the upstream/vendor SoC `.dtsi` and the narrowest proven package/module include. Create a new board DTS with:

- correct root `model` and ordered `compatible`
- required address/size cell setup inherited or explicit
- memory description only if firmware does not correctly provide/fix it
- `/chosen` console relationship as defined by platform policy
- boot-critical aliases only when consumers require stable numbering
- console UART and its pinctrl
- boot storage controller, pins, regulators, reset/detect properties
- verified fixed clocks or regulators needed by those paths

Keep other inherited controllers disabled.

## Minimal Source Skeleton

This schematic example is not a binding substitute:

```dts
/dts-v1/;

#include "acme-ax9.dtsi"
#include "acme-px901.dtsi"

/ {
    model = "Acme AXC300 Revision B";
    compatible = "acme,axc300-revb", "acme,ax9";

    aliases {
        serial0 = &uart2;
        mmc0 = &mmc1;
    };

    chosen {
        stdout-path = "serial0:115200n8";
    };
};

&uart2 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart2_default>;

    status = "okay";
};

&mmc1 {
    bus-width = <8>;
    non-removable;
    pinctrl-names = "default";
    pinctrl-0 = <&mmc1_default>;
    vmmc-supply = <&vcc_3v3>;
    vqmmc-supply = <&vcc_1v8>;

    status = "okay";
};
```

Verify every compatible, property, and provider against current schemas. Do not copy `aliases` solely to preserve reference-board numbering unless the boot/update contract needs it.

## Prove Artifact Selection Before Debugging Source

At build/package/boot checkpoints capture:

```bash
sha256sum path/to/acme/axc300-revb.dtb
fdtdump path/to/acme/axc300-revb.dtb | sed -n '1,100p'
```

In U-Boot, commands vary by configuration, but useful evidence includes:

```text
printenv fdtfile fdt_addr_r bootcmd
fdt addr ${fdt_addr_r}
fdt header
fdt print / model
fdt print / compatible
fdt print /chosen
fdt print /memory
```

Prove load address, size, configuration/filename, digest if available, post-fixup root identity, and the exact FDT address passed to the kernel. Never assume U-Boot's control FDT is the Linux working FDT.

## Validate The Handoff Contract

Architecture requirements differ. For AArch64, current kernel boot documentation defines DTB placement/alignment and register expectations and requires firmware to initialize RAM and provide the tree. Also verify:

- kernel/initrd/FDT load regions do not overlap
- FDT has room for bootloader fixups/overlays
- RAM passed to Linux excludes firmware/protected regions
- DMA-capable devices are quiesced before kernel handoff where required
- cache/MMU/exception-level handoff follows architecture rules

A malformed memory map may boot once and corrupt data later.

## Establish Console In Layers

```text
physical UART voltage and routing
  -> bootloader UART output/input
  -> UART clock/reset/pinctrl in working FDT
  -> /chosen stdout-path and kernel command line
  -> early console mechanism (platform/config-specific)
  -> normal serial driver and console handover
  -> userspace getty if desired
```

Capture where output stops. “No console” does not prove “no boot.” Use GPIO/LED, JTAG, bootloader logs, persistent logs, or network only as controlled secondary evidence.

## Keep Boot Arguments Diagnostic And Controlled

During lab bring-up, parameters such as increased log level or early console may help, but record their exact source:

- built-in kernel command line
- signed boot configuration
- bootloader environment
- `/chosen/bootargs`
- appended platform policy

Avoid shipping permissive debug, alternate root, or integrity-disabling arguments. Fix `stdout-path` and the normal console path rather than depending indefinitely on an early console.

## Prove Memory Before Stressing Peripherals

Check:

```bash
dmesg --color=never | grep -Ei 'Memory:|OF: fdt|reserved mem|cma'
cat /proc/iomem
cat /proc/meminfo
```

Compare with the approved memory ownership map. Test multiple banks, top-of-memory, and reservations. A reported total alone does not detect overlap with firmware or remote cores.

## Bring Up Boot Storage Next

For eMMC/SD/NAND/NVMe/SPI-NOR as applicable, prove:

1. parent controller clock/reset/power and pinctrl
2. device presence/detection and correct bus width/mode
3. voltage switching and regulator constraints
4. stable reads of known blocks/files
5. root filesystem discovery and mount
6. repeated cold/warm boots
7. write/read integrity in a disposable region
8. recovery path independent of the candidate root

Do not enable maximum frequency or high-speed modes first. Establish conservative operation, then add timing modes with signal-integrity evidence.

## Diagnose By Earliest Divergence

| Last proof | Next likely boundary |
|---|---|
| DTB load digest correct | bootloader selection/fixup/handoff |
| U-Boot working root correct | handoff address or kernel early boot |
| early console prints | normal serial driver/pinctrl/clock/command line |
| kernel mounts initramfs | boot-storage controller/device/root parameters |
| storage enumerates | partition/filesystem/integrity/userspace |

Change only the boundary under test.

## Stage Exit Gate

```text
[ ] packaged, loaded, handed-off, and live root identity are traceable
[ ] kernel console survives early-to-normal handover
[ ] memory map matches ownership and passes stress appropriate to stage
[ ] boot/root storage works conservatively and repeatedly
[ ] recovery remains available
[ ] optional devices remain disabled
[ ] logs, hashes, U-Boot evidence, live tree, and test results are preserved
[ ] minimal source has no unexplained copied nodes
```

## Further Reading

- [Booting AArch64 Linux](https://docs.kernel.org/arch/arm64/booting.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [U-Boot fdt command](https://docs.u-boot.org/en/stable/usage/cmd/fdt.html)
- [U-Boot And Bootloader Device Tree](../u-boot-and-bootloader-device-tree.md)
- [Power, Clock, Reset, Pinctrl, And GPIO Bring-Up](power-clock-reset-pinctrl-and-gpio-bring-up.md)
