---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Control, Working, SPL, And Linux Device Trees

U-Boot can hold more than one FDT in memory. The control FDT drives U-Boot itself; the working FDT is prepared for the operating system. Earlier phases can have their own reduced control trees. Debugging starts by identifying the consumer, address, and lifetime of each blob.

## The Control FDT

With driver-model Devicetree control enabled, U-Boot binds devices from its control FDT. It can obtain that tree through a built-in or appended build artifact, a board-supplied tree, an earlier-stage handoff, or another configured mechanism.

The control tree can describe:

- UART, timer, clock, reset, pinctrl, storage, network, USB, and other bootloader devices
- U-Boot-specific boot policy and configuration
- verified-boot public keys
- binman image-layout descriptions used during the build
- properties that decide availability in early phases

Not every Linux-enabled device needs a U-Boot driver. Conversely, U-Boot may need devices that Linux later leaves disabled or assigns to secure firmware. The shared hardware facts should agree; software enablement and phase policy can differ deliberately.

U-Boot exposes the current control FDT address, commonly through `fdtcontroladdr` after relocation. Its precise origin and relocation behavior depend on the configured control-FDT mechanism.

## The Working FDT

The working FDT is the blob that boot commands prepare for the next-stage OS. It can be:

- loaded as a standalone file
- selected from a FIT configuration
- discovered through bootstd/extlinux
- supplied by an earlier boot stage
- copied from another tree on a platform that intentionally shares one

U-Boot may add boot arguments, initrd boundaries, memory reservations, firmware data, or board fixups before handoff. Those mutations are covered in the next module; here the key fact is that the working FDT is distinct from U-Boot's internal device model.

The `fdt` command operates on the working FDT by default and uses `-c` for the control FDT:

```text
=> fdt addr
Working FDT set to ...
=> fdt addr -c
Control fdt: ...
```

Do not infer that two equal addresses early in boot remain equal after relocation, loading, or resizing.

## Why Live Control-FDT Mutation Is Dangerous

After driver-model binding, devices and uclasses can retain offsets, pointers, phandles, or derived platform data. Replacing, moving, or structurally editing the live control FDT can invalidate those relationships. U-Boot's `fdt` documentation explicitly distinguishes freely mutable working state from sensitive control state.

If runtime selection requires a different control tree, use the supported phase/board-selection mechanism before dependent devices bind. Do not use interactive `fdt set -c` commands as a product configuration system.

## TPL And SPL Trees

TPL and SPL execute before U-Boot proper, often from small on-chip SRAM. They can use phase-specific DTBs filtered from a broader control description. The filtered tree contains only nodes marked or required for that phase, plus mandatory infrastructure selected by the build tooling.

This creates several possible artifacts:

```text
tpl/u-boot-tpl.dtb
spl/u-boot-spl.dtb
u-boot.dtb
board-linux.dtb
```

Their node sets, properties, addresses in the firmware image, and runtime locations can all differ.

An SPL DTB describes devices SPL needs to reach the next boot phase—perhaps SRAM, clocks, pinctrl, DRAM initialization, MMC, SPI flash, or a serial console. It is not automatically the DTB SPL passes to Linux.

## The Linux DTB

The Linux DTB must follow the Linux/Devicetree binding ABI and describe the hardware Linux is allowed to manage. U-Boot-specific build and phase properties should not be used as Linux driver configuration. Projects often share the main hardware DTS sources and merge bootloader-only fragments during the U-Boot build.

Linux receives an FDT pointer according to its architecture boot protocol. Before jumping, U-Boot must ensure:

- the blob is structurally valid
- it lies in safe, accessible memory
- kernel, initrd, decompression, and relocation cannot overwrite it
- required fixups and overlays completed
- reservations and `/chosen` data are coherent
- the selected DTB matches the kernel and hardware

Linux does not know which environment variable or FIT configuration produced the blob unless the system records provenance explicitly.

## Other Tree-Shaped Artifacts

Do not confuse hardware DTBs with:

- **FIT image tree**: container metadata under `/images` and `/configurations`
- **ITS source**: source input to `mkimage` for a FIT
- **binman description**: placement of components in a firmware image
- **FDT map**: compact binman image-content description embedded in an image
- **overlay DTBO**: a fragment applied to a base hardware tree

They share FDT encoding and libfdt tooling but have different schemas and consumers.

## Build An Artifact Ledger

For each shipped blob, record:

| Field | Example question |
|---|---|
| filename/build output | Is it `u-boot.dtb`, `spl/u-boot-spl.dtb`, or an OS DTB? |
| source closure | Which DTS, includes, and generated fragments contributed? |
| package location | Appended, in FIT, filesystem, flash region, or bloblist? |
| selector | Kconfig, board ID, FIT config, filename, bootflow, environment? |
| runtime address | Where is it before and after relocation? |
| consumer | TPL, SPL, U-Boot proper, Linux, or build tool? |
| trust | What authenticates it and its selector? |

This ledger prevents “the DTB” from concealing five separate artifacts.

## Runtime Inspection

Useful commands vary by configuration:

```text
=> bdinfo
=> fdt addr -c
=> fdt header -c
=> fdt print -c /model
=> fdt addr
=> fdt header
=> fdt print /model
=> printenv fdtcontroladdr fdt_addr_r fdtfile
```

Capture command output and memory addresses before `bootm`, `booti`, or `bootz` changes or relocates data. On the host, preserve and hash every packaged DT artifact.

## Authoritative References

- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot generic SPL framework](https://docs.u-boot.org/en/latest/develop/spl.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)

## Continue

Proceed to [U-Boot DT Sources, Upstream Sync, And Build Artifacts](u-boot-dt-sources-upstream-sync-and-build-artifacts.md).
