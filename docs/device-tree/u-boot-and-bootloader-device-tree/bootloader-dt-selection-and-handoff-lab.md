---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Bootloader DT Selection And Handoff Lab

This lab traces a fictional dual-board product from ROM through SPL and U-Boot into Linux. A shared firmware image contains two control DTBs, a signed FIT with one kernel and two Linux DTBs, and an optional authenticated mezzanine overlay. The goal is to prove selection, memory safety, verification, and final handoff.

## Objectives

By the end, you should be able to:

- inventory every tree-shaped artifact and its consumer
- derive an early-phase dependency closure
- calculate SRAM and DRAM image ranges
- reproduce SPL and FIT configuration selection
- verify that configuration, DTB, and overlay policy share one trust chain
- distinguish control and working FDT runtime addresses
- capture the final Linux-bound DTB and its provenance
- diagnose deliberate selection, sizing, and trust faults

## Product Contract

Assume:

- ROM loads at most 256 KiB into SRAM at `0x00100000`
- SRAM ends at `0x00180000`; the remainder is available for SPL runtime
- SPL chooses Board A or B using a CRC-protected EEPROM identifier
- generic I²C pins and timing are identical on both boards
- Board A and B require different PMIC/DRAM configuration
- SPL initializes DRAM and loads U-Boot proper plus a signed FIT
- U-Boot proper uses the control DTB selected by SPL
- the FIT contains one kernel and two Linux DTBs
- an optional mezzanine is identified by a trusted product manifest and uses one DTBO
- U-Boot proper is authenticated by the previous stage
- FIT verification keys are embedded in its control DTB

Compatible strings, addresses, and commands are illustrative. A real product must use its SoC's ROM format, U-Boot version, linker scripts, and verified-boot design.

## Step 1: Create The Artifact Ledger

| Artifact | Consumer | Selector | Trust |
|---|---|---|---|
| `spl/u-boot-spl.bin` | ROM/CPU | ROM boot policy | ROM authentication |
| generic SPL DTB | SPL before EEPROM read | built with SPL | covered by SPL image |
| Board A/B control DTB | SPL then U-Boot | validated EEPROM ID | authenticated U-Boot package |
| binman description | build tooling | board build config | release build provenance |
| FIT image tree | SPL/U-Boot loader | FIT parser | signed configuration |
| Board A/B Linux DTB | Linux after selection | FIT configuration | config signature |
| mezzanine DTBO | U-Boot working FDT | signed manifest | authenticated overlay |

Explain why the FIT image tree and binman description are not hardware DTBs.

## Step 2: Audit The SPL Dependency Closure

SPL needs:

```text
SRAM and timer
  -> generic pinctrl
  -> I2C controller
  -> EEPROM
  -> selected PMIC/regulators
  -> selected DRAM controller/configuration
  -> boot storage
  -> hash/signature implementation
```

The generic SPL tree must contain enough I²C dependencies to read identity before switching to the board-specific selection. Both selected trees must contain their complete PMIC, DRAM, and storage dependency closures, with appropriate `bootph-*` properties and phase drivers.

Decompile all phase DTBs and verify the closure rather than reading only source files.

## Step 3: Prove The SRAM Budget

Assume the map reports:

```text
SPL text+rodata+data  176 KiB
SPL BSS                28 KiB
generic SPL DTB         9 KiB
stack reservation      16 KiB
malloc/global data     20 KiB
handoff/bloblist        4 KiB
```

The ROM-loaded bytes are 185 KiB before format/header/padding, under its 256 KiB limit. Runtime demand is:

```text
176 + 28 + 9 + 16 + 20 + 4 = 253 KiB
```

The 512 KiB SRAM window leaves room, but placement still matters. Draw actual intervals from linker and binman maps and prove BSS clearing cannot overwrite the DTB. Preserve warning headroom for key rotation and recovery fixes.

## Step 4: Define Selection

EEPROM record:

```text
magic | format-version | board-id | revision | feature-bits | CRC
```

Policy:

1. reject invalid magic, format, length, or CRC
2. map known Board A revisions to `board-a`
3. map known Board B revisions to `board-b`
4. reject unsupported IDs into authenticated recovery
5. record raw identity source and normalized selection in the handoff

Do not default an unknown ID to Board A: incompatible PMIC or DRAM settings can damage hardware or corrupt memory.

## Step 5: Inspect FIT Pairing

A simplified configuration section:

```dts
configurations {
        default = "conf-a";

        conf-a {
                description = "Product Board A";
                kernel = "kernel";
                fdt = "fdt-a";
                signature {
                        algo = "sha256,rsa2048";
                        key-name-hint = "prod";
                        sign-images = "kernel", "fdt";
                };
        };

        conf-b {
                description = "Product Board B";
                kernel = "kernel";
                fdt = "fdt-b";
                signature {
                        algo = "sha256,rsa2048";
                        key-name-hint = "prod";
                        sign-images = "kernel", "fdt";
                };
        };
};
```

Use current FIT syntax and tooling for a real build. Prove that explicit selection overrides an unsuitable default and that the configuration signature protects the kernel/DTB association. Confirm the production key is required by the authenticated control DTB.

## Step 6: Plan DRAM Addresses

Assume:

```text
U-Boot relocated    [0xbf000000, 0xbf200000)
malloc/stack reserve[0xbee00000, 0xbf000000)
FIT source          [0x90000000, 0x94000000)
kernel output       [0x80080000, 0x84080000)
initrd              [0x88000000, 0x8a000000)
working FDT         [0x87000000, 0x87040000)
overlay blob        [0x87100000, 0x87110000)
```

All intervals are illustrative and nonoverlapping. Confirm decompressor behavior, actual image sizes, reserved memory, and architecture constraints. The 256 KiB working-FDT window must cover base, overlay growth, and later fixups.

## Step 7: Execute And Capture Selection

Host-side:

```sh
mkimage -l product.itb
binman ls -i firmware.bin
fdtdump build/u-boot.dtb
fdtdump build/spl/u-boot-spl.dtb
sha256sum firmware.bin product.itb board-a.dtb board-b.dtb mezzanine.dtbo
```

U-Boot-side:

```text
=> bdinfo
=> fdt addr -c
=> fdt print -c /model
=> printenv fdtcontroladdr fdt_addr_r fdtfile
=> iminfo ${fit_addr}
=> fdt addr
=> fdt print /model
```

Capture SPL board-ID log, chosen control tree, FIT configuration, signature result, working-FDT address, overlay decision, and the final boot command.

## Step 8: Apply The Mezzanine Overlay

Only after the signed product manifest authorizes it:

1. load and authenticate `mezzanine.dtbo`
2. validate it against selected board/revision and kernel bundle
3. copy the verified base DTB to the working buffer
4. reserve sufficient space
5. apply the overlay once
6. inspect expected nodes and conflicting resources
7. on failure, discard the buffer and enter defined fallback

Record ordered input hashes and final DTB hash. Do not apply it to the control FDT; U-Boot does not need to bind the mezzanine merely because Linux will.

## Step 9: Confirm Linux Handoff

Before the final jump, capture the working DTB from memory using a platform-appropriate U-Boot command or debugger. After boot:

```sh
tr -d '\\0' </proc/device-tree/model
tr '\\0' '\\n' </proc/device-tree/compatible
cat /proc/cmdline
dtc -I fs -O dts -o linux-live.dts /sys/firmware/devicetree/base
```

Compare:

```text
packaged base DTB
  -> authorized overlay result
  -> U-Boot final captured DTB
  -> Linux live tree
```

Expected differences after unflattening or kernel interpretation should be understood. The U-Boot capture is the best boundary artifact for later mutation analysis.

## Step 10: Diagnose Deliberate Faults

### Fault A: EEPROM CRC Fails

Do not choose the FIT default or a board control tree by guess. Enter the defined authenticated recovery path and report the raw failure safely.

### Fault B: EEPROM I²C Pinctrl Is Missing From SPL

The full U-Boot tree contains it, but the filtered SPL tree does not. Inspect phase properties on the controller, pinctrl state, clock, bus parent, and GPIO/regulator dependencies. Do not hard-code Board A as a workaround.

### Fault C: SPL Binary Fits ROM, Then Crashes While Clearing BSS

The packaged DTB or handoff data overlaps runtime BSS. Use ELF/linker and binman interval maps; file-size success did not prove runtime layout.

### Fault D: FIT Signature Passes For Board A On Board B

Cryptography authenticated the selected configuration but selection chose the wrong valid one. Fix the trusted identity-to-configuration mapping. Signature validity does not prove hardware compatibility.

### Fault E: Base DTB Is Signed, Overlay Is Not

Reject the overlay. Applying it would create an unauthorized final hardware description even though the base and kernel are authentic.

### Fault F: `fdt apply` Returns An Error

Discard the working tree and overlay buffers according to U-Boot's documented invalidation behavior. Reload the pristine base or enter fallback; do not continue with partial state.

### Fault G: Linux Shows Board A But SPL Logged Board B

Trace explicit FIT configuration, environment/script overrides, `fdtfile`, bootflow metadata, and working-FDT address. The control tree selection and OS tree selection diverged.

### Fault H: Linux DTB Is Corrupted Only With A Large Initrd

Build the full DRAM interval map. The initrd relocation or kernel decompression overlaps the expanded working FDT. Moving an address without recalculating every range is not sufficient.

## Exit Review

The lab is complete when you can provide:

- a complete artifact ledger and source/build provenance
- filtered SPL dependency audit
- ROM and SRAM interval maps with headroom
- deterministic identity and fallback policy
- FIT configuration and required-key verification evidence
- nonoverlapping DRAM load/relocation map
- authenticated overlay compatibility and order record
- final U-Boot handoff DTB plus Linux live-tree comparison
- root-cause evidence for every deliberate fault

## Authoritative References

- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot generic SPL framework](https://docs.u-boot.org/en/latest/develop/spl.html)
- [U-Boot FIT signature verification](https://docs.u-boot.org/en/latest/usage/fit/signature.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [U-Boot binman documentation](https://docs.u-boot.org/en/latest/develop/package/binman.html)

## Continue

Proceed to [Boot-Time Mutation And Ownership](../boot-time-mutation-and-ownership.md).
